#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "MaterializeBatchProcessor.h"

#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "IDetailsView.h"

/**
 * Batch Processor Window - Full-featured UI for batch operations
 */
class MATERIALIZE_API SMaterializeBatchWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterializeBatchWindow) {}
		SLATE_ARGUMENT(TSharedPtr<UMaterializeBatchProcessor>, Processor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMaterializeBatchWindow();

	static void OpenWindow();
	static void CloseWindow();
	static bool IsWindowOpen();

private:
	// --- Data ---
	TSharedPtr<UMaterializeBatchProcessor> Processor;
	TArray<TSharedPtr<FKBatchItem>> ItemPtrs;
	
	// --- Widgets ---
	TSharedPtr<SListView<TSharedPtr<FKBatchItem>>> QueueListView;
	TSharedPtr<SProgressBar> OverallProgressBar;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<STextBlock> TimeText;
	TSharedPtr<STextBlock> ItemCountText;
	TSharedPtr<IDetailsView> SettingsView;
	
	// --- List View Callbacks ---
	TSharedRef<ITableRow> GenerateQueueRow(TSharedPtr<FKBatchItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnQueueSelectionChanged(TSharedPtr<FKBatchItem> Item, ESelectInfo::Type SelectInfo);
	
	// --- Button Callbacks ---
	FReply OnAddFromBrowserClicked();
	FReply OnAddFromFolderClicked();
	FReply OnRemoveSelectedClicked();
	FReply OnClearQueueClicked();
	FReply OnStartClicked();
	FReply OnPauseClicked();
	FReply OnCancelClicked();
	FReply OnMoveUpClicked();
	FReply OnMoveDownClicked();
	
	// --- Event Handlers ---
	void OnItemStarted(const FGuid& ItemId);
	void OnItemCompleted(const FGuid& ItemId, bool bSuccess);
	void OnProgressUpdated(const FKBatchProgress& Progress);
	void OnBatchCompleted(const FKBatchProgress& FinalProgress);
	
	// --- Helpers ---
	void RefreshList();
	void UpdateUI();
	FText GetStatusText() const;
	FText GetTimeText() const;
	FText GetItemCountText() const;
	bool IsProcessing() const;
	bool CanProcess() const;
	
	// --- Static Window Instance ---
	static TWeakPtr<SWindow> BatchWindowPtr;
};

/**
 * Batch Queue Row Widget
 */
class SMaterializeBatchQueueRow : public SMultiColumnTableRow<TSharedPtr<FKBatchItem>>
{
public:
	SLATE_BEGIN_ARGS(SMaterializeBatchQueueRow) {}
		SLATE_ARGUMENT(TSharedPtr<FKBatchItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FKBatchItem> Item;
	
	FSlateColor GetStatusColor() const;
	FText GetStatusText() const;
	const FSlateBrush* GetStatusIcon() const;
};
