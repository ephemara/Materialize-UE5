#include "MaterializeAssetActions.h"
#include "MaterializeEditorContext.h"
#include "ToolMenus.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "Materialize"

void FMaterializeTextureAssetActions::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	TArray<TWeakObjectPtr<UTexture2D>> Textures;
	for (UObject* Obj : InObjects)
	{
		if (UTexture2D* Tex = Cast<UTexture2D>(Obj))
		{
			Textures.Add(Tex);
		}
	}

	if (Textures.Num() == 0) return;

	Section.AddMenuEntry(
		"KSample_GeneratePBR",
		LOCTEXT("GeneratePBR", "Generate PBR Material"),
		LOCTEXT("GeneratePBRTooltip", "Open Materialize editor to generate PBR material from this texture"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FMaterializeTextureAssetActions::OpenKSampleEditor, Textures),
			FCanExecuteAction()
		)
	);
}

void FMaterializeTextureAssetActions::OpenKSampleEditor(TArray<TWeakObjectPtr<UTexture2D>> Textures)
{
	// Set the first texture as the current texture in the editor context
	if (Textures.Num() > 0 && Textures[0].IsValid())
	{
		FMaterializeEditorContext::SetCurrentTexture(Textures[0].Get());
	}
	
	// Open the Materialize editor tab
	FGlobalTabmanager::Get()->TryInvokeTab(FName("KSampleEditor"));
}

#undef LOCTEXT_NAMESPACE
