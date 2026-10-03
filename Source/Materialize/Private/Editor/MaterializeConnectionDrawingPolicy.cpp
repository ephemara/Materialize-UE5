// Copyright K-Studio. All Rights Reserved.

#include "Editor/MaterializeConnectionDrawingPolicy.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "SGraphPanel.h"
#include "SGraphPin.h"
#include "GraphEditorSettings.h"
#include "Widgets/SToolTip.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Layout/ArrangedWidget.h"

// Note: UMaterializeGraphNode_Knot will be implemented in Phase 7
// For now, we stub out the knot-related functionality

// ============================================================================
// FMaterializeConnectionDrawingPolicy
// ============================================================================

FMaterializeConnectionDrawingPolicy::FMaterializeConnectionDrawingPolicy(
	int32 InBackLayerID,
	int32 InFrontLayerID,
	float ZoomFactor,
	const FSlateRect& InClippingRect,
	FSlateWindowElementList& InDrawElements,
	UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, SampleGraph(Cast<UMaterializeGraph>(InGraphObj))
	, SampleGraphSchema(SampleGraph ? Cast<UMaterializeGraphSchema>(SampleGraph->GetSchema()) : nullptr)
{
	// Clean wire style - no arrow heads (like Substance Designer)
	ArrowImage = nullptr;
	ArrowRadius = FVector2D::ZeroVector;

	// Subtle deemphasis when hovering over connections
	HoverDeemphasisDarkFraction = 0.4f;
}

bool FMaterializeConnectionDrawingPolicy::FindPinCenter(UEdGraphPin* Pin, FVector2D& OutCenter) const
{
	if (const TSharedPtr<SGraphPin>* pPinWidget = PinToPinWidgetMap.Find(Pin))
	{
		if (FArrangedWidget* pPinEntry = PinGeometries->Find((*pPinWidget).ToSharedRef()))
		{
			OutCenter = FGeometryHelper::CenterOf(pPinEntry->Geometry);
			return true;
		}
	}
	return false;
}

bool FMaterializeConnectionDrawingPolicy::GetAverageConnectedPosition(
	UMaterializeGraphNode_Knot* Knot, 
	EEdGraphPinDirection Direction, 
	FVector2D& OutPos) const
{
	// Stub for Phase 7 - Knot nodes not yet implemented
	return false;
}

bool FMaterializeConnectionDrawingPolicy::ShouldChangeTangentForKnot(UMaterializeGraphNode_Knot* Knot)
{
	// Stub for Phase 7 - Knot nodes not yet implemented
	return false;
}

void FMaterializeConnectionDrawingPolicy::DetermineWiringStyle(
	UEdGraphPin* OutputPin, 
	UEdGraphPin* InputPin, 
	FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;

	// Default wire color
	FLinearColor WireColor = WireColorTexture;
	bool bInactivePin = false;

	// Determine wire color based on pin type
	if (OutputPin)
	{
		const FName& PinCategory = OutputPin->PinType.PinCategory;
		
		if (PinCategory == UMaterializeGraphSchema::PC_Texture)
		{
			WireColor = WireColorTexture;
		}
		else if (PinCategory == UMaterializeGraphSchema::PC_Scalar)
		{
			WireColor = WireColorScalar;
		}
		else if (PinCategory == UMaterializeGraphSchema::PC_Vector)
		{
			WireColor = WireColorVector;
		}
		else if (PinCategory == UMaterializeGraphSchema::PC_Mask)
		{
			WireColor = WireColorMask;
		}
		else if (PinCategory == UMaterializeGraphSchema::PC_Normal)
		{
			WireColor = FLinearColor(0.2f, 0.8f, 0.8f); // Cyan for normal maps
		}

		// Check if node is disabled
		UEdGraphNode* OutputNode = OutputPin->GetOwningNode();
		if (!OutputNode->IsNodeEnabled() || OutputNode->IsDisplayAsDisabledForced() || OutputNode->IsNodeUnrelated())
		{
			bInactivePin = true;
		}
	}

	// Also check input pin's node state
	if (InputPin)
	{
		UEdGraphNode* InputNode = InputPin->GetOwningNode();
		if (!InputNode->IsNodeEnabled() || InputNode->IsDisplayAsDisabledForced() || InputNode->IsNodeUnrelated())
		{
			bInactivePin = true;
		}
	}

	// Apply inactive styling
	if (bInactivePin)
	{
		WireColor = WireColorInactive;
	}

	Params.WireColor = WireColor;

	// Apply hover deemphasis to unrelated wires
	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, Params.WireThickness, Params.WireColor);
	}
}

TSharedPtr<IToolTip> FMaterializeConnectionDrawingPolicy::GetConnectionToolTip(
	const SGraphPanel& GraphPanel, 
	const FGraphSplineOverlapResult& OverlapData) const
{
	TSharedPtr<SGraphPin> Pin1Widget;
	TSharedPtr<SGraphPin> Pin2Widget;
	OverlapData.GetPinWidgets(GraphPanel, Pin1Widget, Pin2Widget);

	if (!Pin1Widget || !Pin2Widget)
	{
		return FConnectionDrawingPolicy::GetConnectionToolTip(GraphPanel, OverlapData);
	}

	const FText LeftText = FText::Format(
		NSLOCTEXT("Materialize", "PinConnectionTooltipLeft", "<< {0}"), 
		GetNodePinInfo(Pin1Widget));
	const FText RightText = FText::Format(
		NSLOCTEXT("Materialize", "PinConnectionTooltipRight", "{0} >>"), 
		GetNodePinInfo(Pin2Widget));

	return SNew(SToolTip)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 5.0f)
			[
				SNew(STextBlock)
				.Margin(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
				.Justification(ETextJustify::Left)
				.Text(LeftText)
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoHeight()
			.Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Margin(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
				.Justification(ETextJustify::Right)
				.Text(RightText)
			]
		];
}

FText FMaterializeConnectionDrawingPolicy::GetNodePinInfo(const TSharedPtr<SGraphPin>& PinWidget) const
{
	const UEdGraphPin* PinObj = PinWidget->GetPinObj();
	const UEdGraphNode* EdNode = PinObj->GetOwningNode();

	FString NodeTitle = EdNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
	
	// Clean up the title if needed
	NodeTitle.RemoveFromStart(TEXT("Materialize "));

	if (EdNode->GetCanRenameNode())
	{
		FText NodeEditableName = EdNode->GetNodeTitle(ENodeTitleType::EditableTitle);
		NodeTitle = NodeTitle + TEXT(" (") + NodeEditableName.ToString() + TEXT(")");
	}

	const FText PinName = PinObj->GetDisplayName().IsEmptyOrWhitespace() 
		? FText::FromName(PinObj->PinName) 
		: PinObj->GetDisplayName();

	return FText::Format(
		NSLOCTEXT("Materialize", "PinConnectionTooltipPartial", "{0}\n{1}"), 
		FText::FromString(NodeTitle), 
		PinName);
}

// ============================================================================
// FMaterializePinConnectionFactory
// ============================================================================

FConnectionDrawingPolicy* FMaterializePinConnectionFactory::CreateConnectionPolicy(
	const UEdGraphSchema* Schema,
	int32 InBackLayerID,
	int32 InFrontLayerID,
	float ZoomFactor,
	const FSlateRect& InClippingRect,
	FSlateWindowElementList& InDrawElements,
	UEdGraph* InGraphObj) const
{
	if (Schema->IsA(UMaterializeGraphSchema::StaticClass()))
	{
		return new FMaterializeConnectionDrawingPolicy(
			InBackLayerID, 
			InFrontLayerID, 
			ZoomFactor, 
			InClippingRect, 
			InDrawElements, 
			InGraphObj);
	}
	return nullptr;
}
