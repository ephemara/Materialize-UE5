#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Graph/MaterializeGraphExecutor.h"

class UMaterializeGraph;
class SMaterialize3DPreviewViewport;
class SMaterializeNodePalette;

class FMaterializeGraphEditorToolkit : public FAssetEditorToolkit
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;

	void InitGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UMaterializeGraph* InGraph);

	//~ Begin IToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	//~ End IToolkit Interface

	/** Execute the graph and update previews */
	void ExecuteGraph();

	/** Get the graph being edited */
	UMaterializeGraph* GetGraph() const { return KSampleGraph; }

protected:
	/** Build the toolbar */
	void ExtendToolbar();

	/** Toolbar button handlers */
	FReply OnCompileClicked();

	/** Graph editing commands */
	void BindGraphCommands();
	void DeleteSelectedNodes();
	bool CanDeleteNodes() const;
	void CopySelectedNodes();
	bool CanCopyNodes() const;
	void CutSelectedNodes();
	bool CanCutNodes() const;
	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes() const;
	void DuplicateNodes();
	bool CanDuplicateNodes() const;

private:
	UMaterializeGraph* KSampleGraph = nullptr;
	TSharedPtr<class SGraphEditor> GraphEditorWidget;
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<SMaterialize3DPreviewViewport> PreviewViewport;
	TSharedPtr<SMaterializeNodePalette> NodePalette;

	/** Graph executor for GPU dispatch */
	TUniquePtr<FMaterializeGraphExecutor> GraphExecutor;

	/** Debounce timer for auto-execution */
	double LastExecutionTime = 0.0;

	// Tab IDs
	static const FName GraphTabId;
	static const FName DetailsTabId;
	static const FName PreviewTabId;
	static const FName PaletteTabId;

	TSharedRef<SDockTab> SpawnTab_Graph(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Preview(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Palette(const FSpawnTabArgs& Args);
};
