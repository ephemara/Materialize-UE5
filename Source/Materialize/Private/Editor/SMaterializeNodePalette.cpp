// Copyright K-Studio. All Rights Reserved.

#include "Editor/SMaterializeNodePalette.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "EdGraph/EdGraphSchema.h"

#define LOCTEXT_NAMESPACE "KSampleNodePalette"

// ============================================================================
// Palette Tree Row Widget
// ============================================================================

class SMaterializePaletteTreeRow : public STableRow<TSharedPtr<FMaterializePaletteItem>>
{
public:
	SLATE_BEGIN_ARGS(SMaterializePaletteTreeRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FMaterializePaletteItem> InItem)
	{
		Item = InItem;
		
		STableRow<TSharedPtr<FMaterializePaletteItem>>::Construct(
			STableRow<TSharedPtr<FMaterializePaletteItem>>::FArguments()
			.Padding(2.0f)
			.Content()
			[
				SNew(STextBlock)
				.Text(this, &SMaterializePaletteTreeRow::GetDisplayText)
				.Font(this, &SMaterializePaletteTreeRow::GetDisplayFont)
				.ToolTipText(this, &SMaterializePaletteTreeRow::GetTooltipText)
			],
			InOwnerTable
		);
	}

private:
	FText GetDisplayText() const
	{
		if (Item.IsValid())
		{
			if (Item->IsCategory())
			{
				return Item->CategoryName;
			}
			else if (Item->Action.IsValid())
			{
				return Item->Action->GetMenuDescription();
			}
		}
		return FText::GetEmpty();
	}

	FSlateFontInfo GetDisplayFont() const
	{
		if (Item.IsValid() && Item->IsCategory())
		{
			return FCoreStyle::GetDefaultFontStyle("Bold", 9);
		}
		return FCoreStyle::GetDefaultFontStyle("Regular", 9);
	}

	FText GetTooltipText() const
	{
		if (Item.IsValid() && Item->Action.IsValid())
		{
			return Item->Action->GetTooltipDescription();
		}
		return FText::GetEmpty();
	}

	TSharedPtr<FMaterializePaletteItem> Item;
};

// ============================================================================
// SMaterializeNodePalette
// ============================================================================

void SMaterializeNodePalette::Construct(const FArguments& InArgs)
{
	GraphSchema = InArgs._GraphSchema;
	
	BuildPaletteTree();

	ChildSlot
	[
		SNew(SVerticalBox)
		
		// Search box
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SSearchBox)
			.HintText(LOCTEXT("SearchHint", "Search Nodes..."))
			.OnTextChanged(this, &SMaterializeNodePalette::OnFilterTextChanged)
		]
		
		// Node tree
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(TreeView, STreeView<TSharedPtr<FMaterializePaletteItem>>)
				.TreeItemsSource(&RootItems)
				.OnGenerateRow(this, &SMaterializeNodePalette::OnGenerateRow)
				.OnGetChildren(this, &SMaterializeNodePalette::OnGetChildren)
				.OnSelectionChanged(this, &SMaterializeNodePalette::OnSelectionChanged)
				.SelectionMode(ESelectionMode::Single)
			]
		]
	];

	// Expand all categories by default
	for (const TSharedPtr<FMaterializePaletteItem>& Item : RootItems)
	{
		TreeView->SetItemExpansion(Item, true);
	}
}

void SMaterializeNodePalette::BuildPaletteTree()
{
	RootItems.Empty();

	if (!GraphSchema)
	{
		return;
	}

	// For now, create a simple placeholder structure
	// TODO: Implement proper node palette with schema actions
	TSharedPtr<FMaterializePaletteItem> GeneratorsCategory = MakeShared<FMaterializePaletteItem>();
	GeneratorsCategory->CategoryName = LOCTEXT("Generators", "Generators");
	RootItems.Add(GeneratorsCategory);

	TSharedPtr<FMaterializePaletteItem> FiltersCategory = MakeShared<FMaterializePaletteItem>();
	FiltersCategory->CategoryName = LOCTEXT("Filters", "Filters");
	RootItems.Add(FiltersCategory);

	TSharedPtr<FMaterializePaletteItem> BlendCategory = MakeShared<FMaterializePaletteItem>();
	BlendCategory->CategoryName = LOCTEXT("Blend", "Blend");
	RootItems.Add(BlendCategory);

	TSharedPtr<FMaterializePaletteItem> MathCategory = MakeShared<FMaterializePaletteItem>();
	MathCategory->CategoryName = LOCTEXT("Math", "Math");
	RootItems.Add(MathCategory);

	TSharedPtr<FMaterializePaletteItem> OutputCategory = MakeShared<FMaterializePaletteItem>();
	OutputCategory->CategoryName = LOCTEXT("Output", "Output");
	RootItems.Add(OutputCategory);
}

TSharedRef<ITableRow> SMaterializeNodePalette::OnGenerateRow(TSharedPtr<FMaterializePaletteItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SMaterializePaletteTreeRow, OwnerTable, Item);
}

void SMaterializeNodePalette::OnGetChildren(TSharedPtr<FMaterializePaletteItem> Item, TArray<TSharedPtr<FMaterializePaletteItem>>& OutChildren)
{
	if (Item.IsValid())
	{
		// Apply filter if active
		if (!FilterText.IsEmpty())
		{
			for (const TSharedPtr<FMaterializePaletteItem>& Child : Item->Children)
			{
				if (DoesItemMatchFilter(Child))
				{
					OutChildren.Add(Child);
				}
			}
		}
		else
		{
			OutChildren = Item->Children;
		}
	}
}

void SMaterializeNodePalette::OnSelectionChanged(TSharedPtr<FMaterializePaletteItem> Item, ESelectInfo::Type SelectInfo)
{
	// Node spawning is handled via drag-drop or double-click in full implementation
	// For now, selection just highlights the item
}

void SMaterializeNodePalette::OnFilterTextChanged(const FText& InFilterText)
{
	FilterText = InFilterText;
	
	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
		
		// Expand all categories when filtering
		if (!FilterText.IsEmpty())
		{
			for (const TSharedPtr<FMaterializePaletteItem>& Item : RootItems)
			{
				TreeView->SetItemExpansion(Item, true);
			}
		}
	}
}

bool SMaterializeNodePalette::DoesItemMatchFilter(const TSharedPtr<FMaterializePaletteItem>& Item) const
{
	if (FilterText.IsEmpty())
	{
		return true;
	}

	if (!Item.IsValid())
	{
		return false;
	}

	// Categories always match (we filter their children)
	if (Item->IsCategory())
	{
		return true;
	}

	// Check if node name or tooltip contains filter text
	if (Item->Action.IsValid())
	{
		FString FilterString = FilterText.ToString();
		FString NodeName = Item->Action->GetMenuDescription().ToString();
		FString Tooltip = Item->Action->GetTooltipDescription().ToString();
		
		return NodeName.Contains(FilterString, ESearchCase::IgnoreCase) ||
		       Tooltip.Contains(FilterString, ESearchCase::IgnoreCase);
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
