// Copyright K-Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class UMaterializeGraphSchema;
struct FMaterializeGraphSchemaAction_NewNode;

/** Tree item for node palette */
struct FMaterializePaletteItem
{
	FText CategoryName;
	TSharedPtr<FMaterializeGraphSchemaAction_NewNode> Action;
	TArray<TSharedPtr<FMaterializePaletteItem>> Children;
	
	bool IsCategory() const { return !Action.IsValid(); }
};

/**
 * Node palette widget for Materialize graph editor
 * Displays categorized list of nodes that can be spawned
 */
class SMaterializeNodePalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterializeNodePalette) {}
		SLATE_ARGUMENT(const UMaterializeGraphSchema*, GraphSchema)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Build the palette tree structure */
	void BuildPaletteTree();
	
	/** Generate tree row widget */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FMaterializePaletteItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	/** Get children for tree item */
	void OnGetChildren(TSharedPtr<FMaterializePaletteItem> Item, TArray<TSharedPtr<FMaterializePaletteItem>>& OutChildren);
	
	/** Handle item selection */
	void OnSelectionChanged(TSharedPtr<FMaterializePaletteItem> Item, ESelectInfo::Type SelectInfo);
	
	/** Handle filter text change */
	void OnFilterTextChanged(const FText& InFilterText);
	
	/** Check if item matches filter */
	bool DoesItemMatchFilter(const TSharedPtr<FMaterializePaletteItem>& Item) const;

	/** Schema for getting node actions */
	const UMaterializeGraphSchema* GraphSchema = nullptr;
	
	/** Root items for tree */
	TArray<TSharedPtr<FMaterializePaletteItem>> RootItems;
	
	/** Tree view widget */
	TSharedPtr<STreeView<TSharedPtr<FMaterializePaletteItem>>> TreeView;
	
	/** Current filter text */
	FText FilterText;
};
