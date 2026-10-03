#include "SMaterializeBatchWindow.h"
#include "MaterializePresets.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Docking/TabManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "DesktopPlatformModule.h"
#include "Editor.h"

TWeakPtr<SWindow> SMaterializeBatchWindow::BatchWindowPtr;

// =============================================================================
// STATIC WINDOW MANAGEMENT
// =============================================================================

void SMaterializeBatchWindow::OpenWindow()
{
	if (BatchWindowPtr.IsValid())
	{
		BatchWindowPtr.Pin()->BringToFront();
		return;
	}

	// Create processor
	UMaterializeBatchProcessor* Processor = NewObject<UMaterializeBatchProcessor>();
	Processor->AddToRoot();

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Materialize Batch Processor")))
		.ClientSize(FVector2D(1200, 800))
		.SupportsMinimize(true)
		.SupportsMaximize(true)
		[
			SNew(SMaterializeBatchWindow)
			.Processor(MakeShareable(Processor))
		];

	BatchWindowPtr = Window;
	FSlateApplication::Get().AddWindow(Window);
}

void SMaterializeBatchWindow::CloseWindow()
{
	if (BatchWindowPtr.IsValid())
	{
		BatchWindowPtr.Pin()->RequestDestroyWindow();
		BatchWindowPtr.Reset();
	}
}

bool SMaterializeBatchWindow::IsWindowOpen()
{
	return BatchWindowPtr.IsValid();
}

// =============================================================================
// CONSTRUCTION
// =============================================================================

void SMaterializeBatchWindow::Construct(const FArguments& InArgs)
{
	Processor = InArgs._Processor;
	if (!Processor.IsValid())
	{
		Processor = MakeShareable(NewObject<UMaterializeBatchProcessor>());
	}

	// Bind events
	Processor->OnItemStarted.AddSP(this, &SMaterializeBatchWindow::OnItemStarted);
	Processor->OnItemCompleted.AddSP(this, &SMaterializeBatchWindow::OnItemCompleted);
	Processor->OnProgressUpdated.AddSP(this, &SMaterializeBatchWindow::OnProgressUpdated);
	Processor->OnBatchCompleted.AddSP(this, &SMaterializeBatchWindow::OnBatchCompleted);

	// Create settings view
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bAllowSearch = false;
	DetailsArgs.bShowOptions = false;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	SettingsView = PropertyModule.CreateDetailView(DetailsArgs);
	SettingsView->SetObject(Processor.Get());

	// Build UI
	ChildSlot
	[
		SNew(SVerticalBox)
		
		// === TOOLBAR ===
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8)
		[
			SNew(SHorizontalBox)
			
			// Add buttons
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Add from Browser")))
				.OnClicked(this, &SMaterializeBatchWindow::OnAddFromBrowserClicked)
				.IsEnabled_Lambda([this]() { return !IsProcessing(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Add from Folder")))
				.OnClicked(this, &SMaterializeBatchWindow::OnAddFromFolderClicked)
				.IsEnabled_Lambda([this]() { return !IsProcessing(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Remove Selected")))
				.OnClicked(this, &SMaterializeBatchWindow::OnRemoveSelectedClicked)
				.IsEnabled_Lambda([this]() { return !IsProcessing(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Clear All")))
				.OnClicked(this, &SMaterializeBatchWindow::OnClearQueueClicked)
				.IsEnabled_Lambda([this]() { return !IsProcessing(); })
			]
			
			+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]
			
			// Process buttons
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
				.Text(FText::FromString(TEXT("Start Processing")))
				.OnClicked(this, &SMaterializeBatchWindow::OnStartClicked)
				.IsEnabled_Lambda([this]() { return CanProcess() && !IsProcessing(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Pause")))
				.OnClicked(this, &SMaterializeBatchWindow::OnPauseClicked)
				.IsEnabled_Lambda([this]() { return IsProcessing(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Cancel")))
				.OnClicked(this, &SMaterializeBatchWindow::OnCancelClicked)
				.IsEnabled_Lambda([this]() { return IsProcessing(); })
			]
		]
		
		// === MAIN CONTENT ===
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			
			// Left: Queue List
			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(4)
				[
					SNew(SVerticalBox)
					
					// Header
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Queue")))
							.Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right)
						[
							SAssignNew(ItemCountText, STextBlock)
							.Text(this, &SMaterializeBatchWindow::GetItemCountText)
						]
					]
					
					// List
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SAssignNew(QueueListView, SListView<TSharedPtr<FKBatchItem>>)
						.ListItemsSource(&ItemPtrs)
						.OnGenerateRow(this, &SMaterializeBatchWindow::GenerateQueueRow)
						.OnSelectionChanged(this, &SMaterializeBatchWindow::OnQueueSelectionChanged)
						.SelectionMode(ESelectionMode::Multi)
						.HeaderRow(
							SNew(SHeaderRow)
							+ SHeaderRow::Column("Status").DefaultLabel(FText::FromString(TEXT(""))).FixedWidth(24)
							+ SHeaderRow::Column("Name").DefaultLabel(FText::FromString(TEXT("Texture"))).FillWidth(1.0f)
							+ SHeaderRow::Column("Resolution").DefaultLabel(FText::FromString(TEXT("Size"))).FixedWidth(80)
							+ SHeaderRow::Column("Time").DefaultLabel(FText::FromString(TEXT("Time"))).FixedWidth(60)
						)
					]
					
					// Reorder buttons
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Move Up")))
							.OnClicked(this, &SMaterializeBatchWindow::OnMoveUpClicked)
							.IsEnabled_Lambda([this]() { return !IsProcessing(); })
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Move Down")))
							.OnClicked(this, &SMaterializeBatchWindow::OnMoveDownClicked)
							.IsEnabled_Lambda([this]() { return !IsProcessing(); })
						]
					]
				]
			]
			
			// Right: Settings
			+ SSplitter::Slot()
			.Value(0.4f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(4)
				[
					SNew(SVerticalBox)
					
					// Header
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Batch Settings")))
						.Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))
					]
					
					// Settings
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SettingsView.ToSharedRef()
					]
				]
			]
		]
		
		// === PROGRESS BAR ===
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8)
		[
			SNew(SVerticalBox)
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SAssignNew(StatusText, STextBlock)
					.Text(this, &SMaterializeBatchWindow::GetStatusText)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SAssignNew(TimeText, STextBlock)
					.Text(this, &SMaterializeBatchWindow::GetTimeText)
				]
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(OverallProgressBar, SProgressBar)
				.Percent_Lambda([this]() { return Processor.IsValid() ? Processor->GetProgress().OverallProgress : 0.0f; })
			]
		]
	];
}

SMaterializeBatchWindow::~SMaterializeBatchWindow()
{
	if (Processor.IsValid())
	{
		Processor->RemoveFromRoot();
	}
}

// =============================================================================
// LIST VIEW
// =============================================================================

TSharedRef<ITableRow> SMaterializeBatchWindow::GenerateQueueRow(TSharedPtr<FKBatchItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SMaterializeBatchQueueRow, OwnerTable)
		.Item(Item);
}

void SMaterializeBatchWindow::OnQueueSelectionChanged(TSharedPtr<FKBatchItem> Item, ESelectInfo::Type SelectInfo)
{
	// Could show item-specific details here
}

// =============================================================================
// BUTTON CALLBACKS
// =============================================================================

FReply SMaterializeBatchWindow::OnAddFromBrowserClicked()
{
	// Get selected assets from content browser
	TArray<FAssetData> SelectedAssets;
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);
	
	for (const FAssetData& Asset : SelectedAssets)
	{
		if (UTexture2D* Tex = Cast<UTexture2D>(Asset.GetAsset()))
		{
			Processor->AddItem(Tex);
		}
	}
	
	RefreshList();
	return FReply::Handled();
}

FReply SMaterializeBatchWindow::OnAddFromFolderClicked()
{
	// Show folder picker
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();
	
	// For simplicity, just use content browser path dialog
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FPathPickerConfig PathConfig;
	PathConfig.DefaultPath = TEXT("/Game");
	PathConfig.OnPathSelected = FOnPathSelected::CreateLambda([this](const FString& Path)
	{
		Processor->AddItemsFromPath(Path, true);
		RefreshList();
	});
	
	// Open path picker (simplified - in real impl would use dialog)
	Processor->AddItemsFromPath(TEXT("/Game"), true);
	RefreshList();
	
	return FReply::Handled();
}

FReply SMaterializeBatchWindow::OnRemoveSelectedClicked()
{
	TArray<TSharedPtr<FKBatchItem>> Selected = QueueListView->GetSelectedItems();
	for (const TSharedPtr<FKBatchItem>& Item : Selected)
	{
		Processor->RemoveItem(Item->Id);
	}
	RefreshList();
	return FReply::Handled();
}

FReply SMaterializeBatchWindow::OnClearQueueClicked()
{
	Processor->ClearQueue();
	RefreshList();
	return FReply::Handled();
}

FReply SMaterializeBatchWindow::OnStartClicked()
{
	Processor->StartProcessing();
	return FReply::Handled();
}

FReply SMaterializeBatchWindow::OnPauseClicked()
{
	if (Processor->IsProcessing())
	{
		Processor->PauseProcessing();
	}
	else
	{
		Processor->ResumeProcessing();
	}
	return FReply::Handled();
}

FReply SMaterializeBatchWindow::OnCancelClicked()
{
	Processor->CancelProcessing();
	return FReply::Handled();
}

FReply SMaterializeBatchWindow::OnMoveUpClicked()
{
	TArray<TSharedPtr<FKBatchItem>> Selected = QueueListView->GetSelectedItems();
	if (Selected.Num() > 0)
	{
		int32 Index = ItemPtrs.Find(Selected[0]);
		if (Index > 0)
		{
			Processor->MoveItem(Index, Index - 1);
			RefreshList();
		}
	}
	return FReply::Handled();
}

FReply SMaterializeBatchWindow::OnMoveDownClicked()
{
	TArray<TSharedPtr<FKBatchItem>> Selected = QueueListView->GetSelectedItems();
	if (Selected.Num() > 0)
	{
		int32 Index = ItemPtrs.Find(Selected[0]);
		if (Index >= 0 && Index < ItemPtrs.Num() - 1)
		{
			Processor->MoveItem(Index, Index + 2);
			RefreshList();
		}
	}
	return FReply::Handled();
}

// =============================================================================
// EVENT HANDLERS
// =============================================================================

void SMaterializeBatchWindow::OnItemStarted(const FGuid& ItemId)
{
	RefreshList();
}

void SMaterializeBatchWindow::OnItemCompleted(const FGuid& ItemId, bool bSuccess)
{
	RefreshList();
}

void SMaterializeBatchWindow::OnProgressUpdated(const FKBatchProgress& Progress)
{
	// UI updates automatically via delegates
}

void SMaterializeBatchWindow::OnBatchCompleted(const FKBatchProgress& FinalProgress)
{
	RefreshList();
}

// =============================================================================
// HELPERS
// =============================================================================

void SMaterializeBatchWindow::RefreshList()
{
	ItemPtrs.Empty();
	if (Processor.IsValid())
	{
		TArray<FKBatchItem> Queue = Processor->GetQueue();
		for (FKBatchItem& Item : Queue)
		{
			ItemPtrs.Add(MakeShareable(new FKBatchItem(Item)));
		}
	}
	
	if (QueueListView.IsValid())
	{
		QueueListView->RequestListRefresh();
	}
}

void SMaterializeBatchWindow::UpdateUI()
{
	// Force UI refresh
}

FText SMaterializeBatchWindow::GetStatusText() const
{
	if (!Processor.IsValid()) return FText::FromString(TEXT("Ready"));
	
	FKBatchProgress Progress = Processor->GetProgress();
	
	if (Progress.bIsCancelled)
		return FText::FromString(TEXT("Cancelled"));
	if (!Progress.bIsProcessing && Progress.CompletedItems > 0)
		return FText::Format(NSLOCTEXT("Materialize", "Complete", "Complete: {0} of {1} processed"), 
			FText::AsNumber(Progress.CompletedItems), FText::AsNumber(Progress.TotalItems));
	if (Progress.bIsProcessing)
		return FText::Format(NSLOCTEXT("Materialize", "Processing", "Processing: {0}"), 
			FText::FromString(Progress.CurrentItemName));
	
	return FText::FromString(TEXT("Ready - Add textures to queue"));
}

FText SMaterializeBatchWindow::GetTimeText() const
{
	if (!Processor.IsValid()) return FText::GetEmpty();
	
	FKBatchProgress Progress = Processor->GetProgress();
	
	if (Progress.bIsProcessing)
	{
		int32 Elapsed = FMath::RoundToInt(Progress.ElapsedTimeSeconds);
		int32 Remaining = FMath::RoundToInt(Progress.EstimatedTimeRemaining);
		return FText::Format(NSLOCTEXT("Materialize", "Time", "Elapsed: {0}s | ETA: {1}s"),
			FText::AsNumber(Elapsed), FText::AsNumber(Remaining));
	}
	else if (Progress.ElapsedTimeSeconds > 0)
	{
		return FText::Format(NSLOCTEXT("Materialize", "TotalTime", "Total: {0}s"),
			FText::AsNumber(FMath::RoundToInt(Progress.ElapsedTimeSeconds)));
	}
	
	return FText::GetEmpty();
}

FText SMaterializeBatchWindow::GetItemCountText() const
{
	if (!Processor.IsValid()) return FText::FromString(TEXT("0 items"));
	return FText::Format(NSLOCTEXT("Materialize", "ItemCount", "{0} items"), FText::AsNumber(Processor->GetQueueCount()));
}

bool SMaterializeBatchWindow::IsProcessing() const
{
	return Processor.IsValid() && Processor->IsProcessing();
}

bool SMaterializeBatchWindow::CanProcess() const
{
	return Processor.IsValid() && Processor->GetQueueCount() > 0;
}

// =============================================================================
// QUEUE ROW WIDGET
// =============================================================================

void SMaterializeBatchQueueRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
{
	Item = InArgs._Item;
	SMultiColumnTableRow<TSharedPtr<FKBatchItem>>::Construct(FSuperRowType::FArguments(), OwnerTable);
}

TSharedRef<SWidget> SMaterializeBatchQueueRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!Item.IsValid()) return SNullWidget::NullWidget;
	
	if (ColumnName == "Status")
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.WidthOverride(20)
			.HeightOverride(20)
			[
				SNew(SImage)
				.Image(this, &SMaterializeBatchQueueRow::GetStatusIcon)
				.ColorAndOpacity(this, &SMaterializeBatchQueueRow::GetStatusColor)
			];
	}
	else if (ColumnName == "Name")
	{
		return SNew(STextBlock)
			.Text(FText::FromString(FPaths::GetBaseFilename(Item->SourcePath)));
	}
	else if (ColumnName == "Resolution")
	{
		if (UTexture2D* Tex = Item->SourceTexture.LoadSynchronous())
		{
			return SNew(STextBlock)
				.Text(FText::Format(NSLOCTEXT("Materialize", "Res", "{0}x{1}"), 
					FText::AsNumber(Tex->GetSizeX()), FText::AsNumber(Tex->GetSizeY())));
		}
		return SNew(STextBlock).Text(FText::FromString(TEXT("-")));
	}
	else if (ColumnName == "Time")
	{
		if (Item->ProcessingTimeMs > 0)
		{
			return SNew(STextBlock)
				.Text(FText::Format(NSLOCTEXT("Materialize", "Ms", "{0}ms"), 
					FText::AsNumber(FMath::RoundToInt(Item->ProcessingTimeMs))));
		}
		return SNew(STextBlock).Text(FText::FromString(TEXT("-")));
	}
	
	return SNullWidget::NullWidget;
}

FSlateColor SMaterializeBatchQueueRow::GetStatusColor() const
{
	if (!Item.IsValid()) return FSlateColor(FLinearColor::Gray);
	
	switch (Item->Status)
	{
		case EKBatchItemStatus::Pending:    return FSlateColor(FLinearColor::Gray);
		case EKBatchItemStatus::Processing: return FSlateColor(FLinearColor(0.2f, 0.6f, 1.0f));
		case EKBatchItemStatus::Completed:  return FSlateColor(FLinearColor::Green);
		case EKBatchItemStatus::Failed:     return FSlateColor(FLinearColor::Red);
		case EKBatchItemStatus::Skipped:    return FSlateColor(FLinearColor::Yellow);
	}
	return FSlateColor(FLinearColor::Gray);
}

FText SMaterializeBatchQueueRow::GetStatusText() const
{
	if (!Item.IsValid()) return FText::GetEmpty();
	
	switch (Item->Status)
	{
		case EKBatchItemStatus::Pending:    return FText::FromString(TEXT("Pending"));
		case EKBatchItemStatus::Processing: return FText::FromString(TEXT("Processing..."));
		case EKBatchItemStatus::Completed:  return FText::FromString(TEXT("Done"));
		case EKBatchItemStatus::Failed:     return FText::FromString(Item->ErrorMessage);
		case EKBatchItemStatus::Skipped:    return FText::FromString(TEXT("Skipped"));
	}
	return FText::GetEmpty();
}

const FSlateBrush* SMaterializeBatchQueueRow::GetStatusIcon() const
{
	if (!Item.IsValid()) return nullptr;
	
	switch (Item->Status)
	{
		case EKBatchItemStatus::Pending:    return FAppStyle::Get().GetBrush("Icons.FilledCircle");
		case EKBatchItemStatus::Processing: return FAppStyle::Get().GetBrush("Icons.Refresh");
		case EKBatchItemStatus::Completed:  return FAppStyle::Get().GetBrush("Icons.Check");
		case EKBatchItemStatus::Failed:     return FAppStyle::Get().GetBrush("Icons.Error");
		case EKBatchItemStatus::Skipped:    return FAppStyle::Get().GetBrush("Icons.Warning");
	}
	return nullptr;
}
