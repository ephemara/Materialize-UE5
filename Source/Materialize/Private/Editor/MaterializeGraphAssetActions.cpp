#include "Editor/MaterializeGraphAssetActions.h"
#include "Editor/MaterializeGraphEditorToolkit.h"
#include "Graph/MaterializeGraph.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FAssetTypeActions_KSampleGraph::FAssetTypeActions_KSampleGraph(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
{
}

FText FAssetTypeActions_KSampleGraph::GetName() const
{
	return LOCTEXT("AssetTypeActions_KSampleGraph", "Materialize Graph");
}

FColor FAssetTypeActions_KSampleGraph::GetTypeColor() const
{
	return FColor(120, 200, 255);
}

UClass* FAssetTypeActions_KSampleGraph::GetSupportedClass() const
{
	return UMaterializeGraph::StaticClass();
}

void FAssetTypeActions_KSampleGraph::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UMaterializeGraph* Graph = Cast<UMaterializeGraph>(*ObjIt))
		{
			TSharedRef<FMaterializeGraphEditorToolkit> EditorToolkit = MakeShareable(new FMaterializeGraphEditorToolkit());
			EditorToolkit->InitGraphEditor(Mode, EditWithinLevelEditor, Graph);
		}
	}
}

uint32 FAssetTypeActions_KSampleGraph::GetCategories()
{
	return MyAssetCategory;
}

#undef LOCTEXT_NAMESPACE
