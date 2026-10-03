#include "MaterializeToolbarExtension.h"
#include "MaterializeStyle.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "EditorStyleSet.h"
#include "Styling/SlateIconFinder.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "KSampleToolbarExtension"

// Static member initialization
TSharedPtr<FExtender> FMaterializeToolbarExtension::ToolbarExtender;
FDelegateHandle FMaterializeToolbarExtension::ExtensionHandle;

void FMaterializeToolbarExtension::RegisterToolbarExtension()
{
	// Get the Level Editor module
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	// Create a new toolbar extender
	ToolbarExtender = MakeShareable(new FExtender);
	
	// Add toolbar extension
	ToolbarExtender->AddToolBarExtension(
		"Content",  // Extension hook - position near Content Browser button
		EExtensionHook::After,
		nullptr,
		FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.AddWidget(FMaterializeToolbarExtension::GenerateToolbarButton());
		})
	);
	
	// Register the extender with the Level Editor
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
}

void FMaterializeToolbarExtension::UnregisterToolbarExtension()
{
	// Clean up the toolbar extender
	if (ToolbarExtender.IsValid())
	{
		FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor");
		if (LevelEditorModule)
		{
			LevelEditorModule->GetToolBarExtensibilityManager()->RemoveExtender(ToolbarExtender);
		}
		
		ToolbarExtender.Reset();
	}
}

TSharedRef<SWidget> FMaterializeToolbarExtension::GenerateToolbarButton()
{
	// Create the toolbar button with custom icon
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.OnClicked_Lambda([]() -> FReply
		{
			FMaterializeToolbarExtension::OnToolbarButtonClicked();
			return FReply::Handled();
		})
		.ToolTipText(LOCTEXT("KSampleToolbarButtonTooltip", "Open Materialize Editor\n\nGenerate PBR materials from photos using GPU-accelerated processing.\nSupports photo-to-PBR extraction and node-based procedural workflows."))
		.ContentPadding(FMargin(1, 0))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f)
			[
				SNew(SImage)
				.Image(FMaterializeStyle::Get().GetBrush("Materialize.ToolbarIcon"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("KSampleToolbarButtonLabel", "Materialize"))
				.TextStyle(FAppStyle::Get(), "NormalText")
			]
		];
}

void FMaterializeToolbarExtension::OnToolbarButtonClicked()
{
	// Open the Materialize editor tab
	FGlobalTabmanager::Get()->TryInvokeTab(FName("KSampleEditor"));
}

FSlateIcon FMaterializeToolbarExtension::GetToolbarIcon()
{
	// Return custom Materialize icon from our style set
	return FSlateIcon(FMaterializeStyle::Get().GetStyleSetName(), "Materialize.ToolbarIcon");
}

#undef LOCTEXT_NAMESPACE
