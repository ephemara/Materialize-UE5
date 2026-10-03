#include "Editor/MaterializeGraphEditorToolkit.h"
#include "Editor/SMaterialize3DPreviewViewport.h"
#include "Editor/SMaterializeNodePalette.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Graph/Nodes/MaterializeGraphNode_Output.h"
#include "GraphEditor.h"
#include "EdGraphUtilities.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "KSampleGraphEditor"

const FName FMaterializeGraphEditorToolkit::GraphTabId(TEXT("KSampleGraphEditor_Graph"));
const FName FMaterializeGraphEditorToolkit::DetailsTabId(TEXT("KSampleGraphEditor_Details"));
const FName FMaterializeGraphEditorToolkit::PreviewTabId(TEXT("KSampleGraphEditor_Preview"));
const FName FMaterializeGraphEditorToolkit::PaletteTabId(TEXT("KSampleGraphEditor_Palette"));

void FMaterializeGraphEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceSettings", "Materialize Graph"));

	InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FMaterializeGraphEditorToolkit::SpawnTab_Graph))
		.SetDisplayName(LOCTEXT("GraphTab", "Graph"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FMaterializeGraphEditorToolkit::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(PreviewTabId, FOnSpawnTab::CreateSP(this, &FMaterializeGraphEditorToolkit::SpawnTab_Preview))
		.SetDisplayName(LOCTEXT("PreviewTab", "3D Preview"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(PaletteTabId, FOnSpawnTab::CreateSP(this, &FMaterializeGraphEditorToolkit::SpawnTab_Palette))
		.SetDisplayName(LOCTEXT("PaletteTab", "Node Palette"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Palette"));
		
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FMaterializeGraphEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(PreviewTabId);
	InTabManager->UnregisterTabSpawner(PaletteTabId);
}

void FMaterializeGraphEditorToolkit::InitGraphEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UMaterializeGraph* InGraph)
{
	KSampleGraph = InGraph;

	// Create graph executor
	GraphExecutor = MakeUnique<FMaterializeGraphExecutor>();

	// Create graph editor widget
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "K-SAMPLE");

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateLambda([this](UEdGraphNode* Node)
	{
		// Double-click on a node - execute the graph to update previews
		ExecuteGraph();
	});
	
	GraphEditorWidget = SNew(SGraphEditor)
		.AdditionalCommands(GetToolkitCommands())
		.Appearance(AppearanceInfo)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents);

	// Create details view
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = true;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(InGraph);

	// Create preview viewport
	PreviewViewport = SNew(SMaterialize3DPreviewViewport)
		.Graph(InGraph);

	// Create node palette
	NodePalette = SNew(SMaterializeNodePalette)
		.GraphSchema(Cast<UMaterializeGraphSchema>(InGraph->GetSchema()));

	// Define layout with vertical split for Preview below graph
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_KSampleGraphEditor_Layout_v4")
		->AddArea
		(
			FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter() ->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(PaletteTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter() ->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.6f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.6f)
						->AddTab(GraphTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.4f)
						->AddTab(PreviewTabId, ETabState::OpenedTab)
					)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(DetailsTabId, ETabState::OpenedTab)
				)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, "KSampleGraphEditor", StandaloneDefaultLayout, true, true, InGraph);

	// Bind graph editing commands
	BindGraphCommands();

	// Extend toolbar with compile button
	ExtendToolbar();

	// Initial execution to populate previews
	ExecuteGraph();
}

void FMaterializeGraphEditorToolkit::ExtendToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("Compile");
			{
				ToolbarBuilder.AddToolBarButton(
					FUIAction(
						FExecuteAction::CreateLambda([this]() { ExecuteGraph(); }),
						FCanExecuteAction::CreateLambda([]() { return true; })
					),
					NAME_None,
					LOCTEXT("Compile", "Compile"),
					LOCTEXT("CompileTooltip", "Execute the graph and update all node previews"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Build")
				);
			}
			ToolbarBuilder.EndSection();
		})
	);

	AddToolbarExtender(ToolbarExtender);
}

void FMaterializeGraphEditorToolkit::BindGraphCommands()
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	const FGraphEditorCommandsImpl& GraphCommands = FGraphEditorCommands::Get();

	TSharedPtr<FUICommandList> CommandList = GetToolkitCommands();

	// Delete
	CommandList->MapAction(GenericCommands.Delete,
		FExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::CanDeleteNodes));

	// Copy
	CommandList->MapAction(GenericCommands.Copy,
		FExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::CopySelectedNodes),
		FCanExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::CanCopyNodes));

	// Cut
	CommandList->MapAction(GenericCommands.Cut,
		FExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::CutSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::CanCutNodes));

	// Paste
	CommandList->MapAction(GenericCommands.Paste,
		FExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::PasteNodes),
		FCanExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::CanPasteNodes));

	// Duplicate
	CommandList->MapAction(GenericCommands.Duplicate,
		FExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::DuplicateNodes),
		FCanExecuteAction::CreateSP(this, &FMaterializeGraphEditorToolkit::CanDuplicateNodes));
}

void FMaterializeGraphEditorToolkit::DeleteSelectedNodes()
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteNodes", "Delete Selected Nodes"));
	
	GraphEditorWidget->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	GraphEditorWidget->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt);
		if (Node && Node->CanUserDeleteNode())
		{
			Node->Modify();
			Node->DestroyNode();
		}
	}

	// Refresh graph
	GraphEditorWidget->NotifyGraphChanged();
	
	// Re-execute graph to update previews
	ExecuteGraph();
}

bool FMaterializeGraphEditorToolkit::CanDeleteNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return false;
	}

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt);
		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void FMaterializeGraphEditorToolkit::CopySelectedNodes()
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	
	// Export selected nodes to text
	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	
	// Copy to clipboard
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FMaterializeGraphEditorToolkit::CanCopyNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return false;
	}

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	return SelectedNodes.Num() > 0;
}

void FMaterializeGraphEditorToolkit::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedNodes();
}

bool FMaterializeGraphEditorToolkit::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FMaterializeGraphEditorToolkit::PasteNodes()
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	PasteNodesHere(GraphEditorWidget->GetPasteLocation());
}

void FMaterializeGraphEditorToolkit::PasteNodesHere(const FVector2D& Location)
{
	if (!GraphEditorWidget.IsValid() || !KSampleGraph)
	{
		return;
	}

	// Get text from clipboard
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import nodes from text
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(KSampleGraph, TextToImport, PastedNodes);

	if (PastedNodes.Num() == 0)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("PasteNodes", "Paste Nodes"));
	KSampleGraph->Modify();

	// Calculate average position of pasted nodes
	FVector2D AvgNodePosition(0.0f, 0.0f);
	for (UEdGraphNode* Node : PastedNodes)
	{
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}
	if (PastedNodes.Num() > 0)
	{
		AvgNodePosition.X /= PastedNodes.Num();
		AvgNodePosition.Y /= PastedNodes.Num();
	}

	// Offset nodes to paste location
	for (UEdGraphNode* Node : PastedNodes)
	{
		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;
		Node->SnapToGrid(16.0f);
	}

	// Select pasted nodes
	GraphEditorWidget->ClearSelectionSet();
	for (UEdGraphNode* Node : PastedNodes)
	{
		GraphEditorWidget->SetNodeSelection(Node, true);
	}

	// Refresh graph
	GraphEditorWidget->NotifyGraphChanged();
	
	// Re-execute graph to update previews
	ExecuteGraph();
}

bool FMaterializeGraphEditorToolkit::CanPasteNodes() const
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	
	// Check if clipboard contains graph node data
	return ClipboardContent.Contains(TEXT("Begin Object Class=/Script/Materialize.KSampleGraphNode"));
}

void FMaterializeGraphEditorToolkit::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FMaterializeGraphEditorToolkit::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FMaterializeGraphEditorToolkit::ExecuteGraph()
{
	UE_LOG(LogTemp, Log, TEXT("Materialize: ExecuteGraph called"));

	if (!KSampleGraph || !GraphExecutor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize: ExecuteGraph - Graph or Executor is null!"));
		return;
	}

	// Debounce - don't execute more than once every 100ms
	double CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - LastExecutionTime < 0.1)
	{
		UE_LOG(LogTemp, Log, TEXT("Materialize: ExecuteGraph - Debounced"));
		return;
	}
	LastExecutionTime = CurrentTime;

	UE_LOG(LogTemp, Log, TEXT("Materialize: ExecuteGraph - Executing graph with %d nodes"), KSampleGraph->Nodes.Num());

	// Execute with previews at 256x256 for speed
	GraphExecutor->ExecuteWithPreviews(KSampleGraph, 256, 256);

	// Also execute at higher res for the 3D preview
	FMaterializeGraphExecutionResult Result = GraphExecutor->Execute(KSampleGraph, 512, 512);

	UE_LOG(LogTemp, Log, TEXT("Materialize: ExecuteGraph - Result: %d nodes executed, BaseColor=%s"), 
		Result.NodesExecuted, Result.BaseColor ? TEXT("Valid") : TEXT("NULL"));

	// Update 3D viewport with results
	if (PreviewViewport.IsValid() && Result.IsValid())
	{
		PreviewViewport->UpdatePreview(
			Result.BaseColor,
			Result.Normal,
			Result.Roughness,
			Result.Metallic,
			Result.Height,
			Result.AO,
			Result.Emissive
		);
	}

	// Force graph editor to refresh
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->NotifyGraphChanged();
	}

	// Log any errors
	const TArray<FString>& Errors = GraphExecutor->GetErrors();
	for (const FString& Error : Errors)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Graph: %s"), *Error);
	}
}

FReply FMaterializeGraphEditorToolkit::OnCompileClicked()
{
	ExecuteGraph();
	return FReply::Handled();
}

TSharedRef<SDockTab> FMaterializeGraphEditorToolkit::SpawnTab_Graph(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == GraphTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTabTitle", "Graph"))
		[
			SNew(SVerticalBox)
			// Toolbar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("CompileBtn", "Compile"))
					.ToolTipText(LOCTEXT("CompileBtnTooltip", "Execute graph and update previews (also updates 3D viewport)"))
					.OnClicked(this, &FMaterializeGraphEditorToolkit::OnCompileClicked)
				]
			]
			// Graph
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				GraphEditorWidget.ToSharedRef()
			]
		];
}

TSharedRef<SDockTab> FMaterializeGraphEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == DetailsTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabTitle", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMaterializeGraphEditorToolkit::SpawnTab_Preview(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PreviewTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("PreviewTabTitle", "3D Preview"))
		[
			PreviewViewport.ToSharedRef()
		];
}

TSharedRef<SDockTab> FMaterializeGraphEditorToolkit::SpawnTab_Palette(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PaletteTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("PaletteTabTitle", "Node Palette"))
		[
			NodePalette.ToSharedRef()
		];
}

FName FMaterializeGraphEditorToolkit::GetToolkitFName() const
{
	return FName("KSampleGraphEditor");
}

FText FMaterializeGraphEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Materialize Graph Editor");
}

FString FMaterializeGraphEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("KSampleGraphEditor");
}

FLinearColor FMaterializeGraphEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

#undef LOCTEXT_NAMESPACE
