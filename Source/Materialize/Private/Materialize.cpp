#include "Materialize.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"
#include "MaterializeAssetActions.h"
#include "Editor/MaterializeGraphAssetActions.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "MaterializeEngine.h"
#include "MaterializePresets.h"
#include "MaterializeEditorContext.h"
#include "SMaterializeEditor.h"
#include "SMaterializeBatchWindow.h"
#include "Factories/MaterialFactoryNew.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/Material.h"
#include "UObject/SavePackage.h"
#include "MaterialEditingLibrary.h"
#include "MaterializeDeveloperSettings.h"
#include "Misc/MessageDialog.h"
#include "Misc/CoreDelegates.h"
#include "Editor/MaterializeGraphNodeFactory.h"
#include "EdGraphUtilities.h"
#include "MaterializeToolbarExtension.h"
#include "MaterializeStyle.h"
#include "MaterializeBackwardCompatibility.h"

#define LOCTEXT_NAMESPACE "Materialize"

static TSharedPtr<FMaterializeGraphNodeFactory> GraphNodeFactory;

void FMaterializeModule::StartupModule()
{
	// Initialize backward compatibility support for Materialize -> Materialize rename
	FMaterializeBackwardCompatibility::Initialize();
	
	// Register custom style set for icons
	FMaterializeStyle::Register();
	
	// Register virtual shader directory (must happen early for shader compilation)
	// Static guard prevents double-registration on hot reload / module reload cycles
	static bool bShaderDirRegistered = false;
	if (!bShaderDirRegistered)
	{
		bShaderDirRegistered = true;
		FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Materialize"))->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/Materialize"), PluginShaderDir);
	}
	
	// Defer editor registration until engine is fully initialized
	// AssetTools and other editor modules aren't available during PostConfigInit
	FCoreDelegates::OnPostEngineInit.AddLambda([this]()
	{
		RegisterAssetActions();
		RegisterMenuExtensions();
		RegisterToolbarExtension();
		
		// Register editor tab (without Window menu entry)
		// SetMenuType(ETabSpawnerMenuType::Hidden) prevents the tab from appearing in the Window menu
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner("KSampleEditor",
			FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& Args)
			{
				return SNew(SDockTab)
					.TabRole(ETabRole::NomadTab)
					.Label(LOCTEXT("KSampleEditorTabLabel", "Materialize Editor"))
					[
						SNew(SMaterializeEditor)
					];
			})
		).SetDisplayName(LOCTEXT("KSampleEditorDisplayName", "Materialize Editor"))
		 .SetMenuType(ETabSpawnerMenuType::Hidden); // Hidden from Window menu - accessible via toolbar only
		
		// TEMPORARILY DISABLED: Custom node factory causes SharedPointer crash
		// TODO: Fix SMaterializeGraphNode widget construction issues
		// GraphNodeFactory = MakeShareable(new FMaterializeGraphNodeFactory());
		// FEdGraphUtilities::RegisterVisualNodeFactory(GraphNodeFactory);
		
		UE_LOG(LogTemp, Log, TEXT("Materialize: Initialized. Access Materialize via toolbar button or right-click any Texture2D"));
	});
}

void FMaterializeModule::ShutdownModule()
{
	UnregisterAssetActions();
	UnregisterToolbarExtension();
	
	// Shutdown backward compatibility support
	FMaterializeBackwardCompatibility::Shutdown();
	
	// Unregister custom style set
	FMaterializeStyle::Unregister();
	
	// TEMPORARILY DISABLED: Custom node factory
	// if (GraphNodeFactory.IsValid())
	// {
	// 	FEdGraphUtilities::UnregisterVisualNodeFactory(GraphNodeFactory);
	// 	GraphNodeFactory.Reset();
	// }
	
	if (FGlobalTabmanager::Get()->HasTabSpawner("KSampleEditor"))
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("KSampleEditor");
	}
}

void FMaterializeModule::RegisterAssetActions()
{
	// Register asset actions for textures
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	
	// Register custom K-Studio category
	EAssetTypeCategories::Type KStudioCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("KStudio")), LOCTEXT("KStudioCategory", "K-Studio"));
	
	TSharedRef<FMaterializeTextureAssetActions> TextureActions = MakeShared<FMaterializeTextureAssetActions>(KStudioCategory);
	AssetTools.RegisterAssetTypeActions(TextureActions);

	TSharedRef<FAssetTypeActions_KSampleGraph> GraphActions = MakeShared<FAssetTypeActions_KSampleGraph>(KStudioCategory);
	AssetTools.RegisterAssetTypeActions(GraphActions);
}

void FMaterializeModule::UnregisterAssetActions()
{
	// AssetTools unregistration happens automatically on module shutdown
}

void FMaterializeModule::RegisterMenuExtensions()
{
	// Main menu: Add K-Studio section with Materialize entries
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
	{
		if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools"))
		{
			FToolMenuSection& Section = Menu->AddSection("KStudio", LOCTEXT("KStudioSection", "K-Studio"));
			
			Section.AddMenuEntry(
				"KSample_OpenEditor",
				LOCTEXT("OpenKSampleEditor", "Open Materialize Editor"),
				LOCTEXT("OpenKSampleEditorTip", "Open the Materialize Editor tab for material generation."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FName("KSampleEditor"));
				}))
			);
			
			Section.AddMenuEntry(
				"KSample_BatchProcessor",
				LOCTEXT("OpenBatchProcessor", "Batch Processor"),
				LOCTEXT("OpenBatchProcessorTip", "Open the Materialize Batch Processor for processing multiple textures at once."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					SMaterializeBatchWindow::OpenWindow();
				}))
			);
			
			Section.AddMenuEntry(
				"KSample_Feedback",
				LOCTEXT("KSampleFeedback", "Feedback & Support"),
				LOCTEXT("KSampleFeedbackTip", "Report bugs, request features, or join the community"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					const UMaterializeDeveloperSettings* Settings = GetDefault<UMaterializeDeveloperSettings>();
					auto Open = [](const FString& URL, const FString& Fallback)
					{
						if (!URL.IsEmpty())
						{
							FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
						}
						else
						{
							FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Fallback));
						}
					};
					Open(Settings->BugReportURL, TEXT("BugReportURL not configured. Please set it in Project Settings → Plugins → Materialize."));
				}))
			);
			
			Section.AddMenuEntry(
				"KSample_Feature",
				LOCTEXT("KSampleFeature", "Request a Feature"),
				LOCTEXT("KSampleFeatureTip", "Open feature request page"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					const UMaterializeDeveloperSettings* Settings = GetDefault<UMaterializeDeveloperSettings>();
					if (!Settings->FeatureRequestURL.IsEmpty())
					{
						FPlatformProcess::LaunchURL(*Settings->FeatureRequestURL, nullptr, nullptr);
					}
					else
					{
						FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("FeatureRequestURL not configured. Set in Project Settings → Plugins → Materialize.")));
					}
				}))
			);
			
			Section.AddMenuEntry(
				"KSample_Discord",
				LOCTEXT("KSampleDiscord", "Join Discord"),
				LOCTEXT("KSampleDiscordTip", "Open community Discord link"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					const UMaterializeDeveloperSettings* Settings = GetDefault<UMaterializeDeveloperSettings>();
					if (!Settings->DiscordURL.IsEmpty())
					{
						FPlatformProcess::LaunchURL(*Settings->DiscordURL, nullptr, nullptr);
					}
					else
					{
						FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("DiscordURL not configured. Set in Project Settings → Plugins → Materialize.")));
					}
				}))
			);
			
			Section.AddMenuEntry(
				"KSample_Docs",
				LOCTEXT("KSampleDocs", "Open Documentation"),
				LOCTEXT("KSampleDocsTip", "Open online docs"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					const UMaterializeDeveloperSettings* Settings = GetDefault<UMaterializeDeveloperSettings>();
					if (!Settings->DocsURL.IsEmpty())
					{
						FPlatformProcess::LaunchURL(*Settings->DocsURL, nullptr, nullptr);
					}
					else
					{
						FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("DocsURL not configured. Set in Project Settings → Plugins → Materialize.")));
					}
				}))
			);
		}
		
		// Try to extend a K-Studio top-level menu if provided by KStudioCore
		if (UToolMenu* KStudioMenu = UToolMenus::Get()->ExtendMenu("KStudio.MainMenu"))
		{
			FToolMenuSection& Section = KStudioMenu->AddSection("Materialize", LOCTEXT("KSampleSection", "Materialize"));
			Section.AddMenuEntry(
				"KSample_OpenEditor_Top",
				LOCTEXT("OpenKSampleEditorTop", "Open Materialize Editor"),
				LOCTEXT("OpenKSampleEditorTopTip", "Open the Materialize Editor tab."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FName("KSampleEditor"));
				}))
			);
		}
	}));
}

void FMaterializeModule::RegisterToolbarExtension()
{
	FMaterializeToolbarExtension::RegisterToolbarExtension();
}

void FMaterializeModule::UnregisterToolbarExtension()
{
	FMaterializeToolbarExtension::UnregisterToolbarExtension();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMaterializeModule, Materialize)
