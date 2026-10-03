#include "Graph/MaterializeGraphSchema.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "Graph/Nodes/MaterializeGraphNode_Noise.h"
#include "Graph/Nodes/MaterializeGraphNode_Blend.h"
#include "Graph/Nodes/MaterializeGraphNode_Math.h"
#include "Graph/Nodes/MaterializeGraphNode_Filter.h"
#include "Graph/Nodes/MaterializeGraphNode_Adjustment.h"
#include "Graph/Nodes/MaterializeGraphNode_Output.h"
#include "Graph/Nodes/MaterializeGraphNode_ChannelOutput.h"
#include "Graph/Nodes/MaterializeGraphNode_Knot.h"
#include "Editor/MaterializeConnectionDrawingPolicy.h"
#include "EdGraph/EdGraph.h"

#define LOCTEXT_NAMESPACE "KSampleGraphSchema"

// Pin Category Constants
const FName UMaterializeGraphSchema::PC_Texture = TEXT("Texture");
const FName UMaterializeGraphSchema::PC_Scalar = TEXT("Scalar");
const FName UMaterializeGraphSchema::PC_Vector = TEXT("Vector");
const FName UMaterializeGraphSchema::PC_Mask = TEXT("Mask");
const FName UMaterializeGraphSchema::PC_Normal = TEXT("Normal");

// -----------------------------------------------------------------------------
// Action to add a node
// -----------------------------------------------------------------------------

UEdGraphNode* FMaterializeGraphSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	if (!NodeTemplate)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
	ParentGraph->Modify();
	if (FromPin)
	{
		FromPin->Modify();
	}

	// Duplicate template to create new node
	UMaterializeGraphNode* NewNode = NewObject<UMaterializeGraphNode>(ParentGraph, NodeTemplate->GetClass());
	NewNode->SetFlags(RF_Transactional);
	ParentGraph->AddNode(NewNode, true, bSelectNewNode);

	NewNode->CreateNewGuid();
	NewNode->PostPlacedNewNode();
	NewNode->AllocateDefaultPins();
	NewNode->AutowireNewNode(FromPin);

	NewNode->NodePosX = Location.X;
	NewNode->NodePosY = Location.Y;

	NewNode->SnapToGrid(16.0f); // Standard grid snap size

	return NewNode;
}

// -----------------------------------------------------------------------------
// Schema Implementation
// -----------------------------------------------------------------------------

void UMaterializeGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// Helper lambda to add actions
	auto AddNodeAction = [&](const FText& Category, const FText& Name, const FText& Tooltip, UClass* NodeClass)
	{
		TSharedPtr<FMaterializeGraphSchemaAction_NewNode> Action = MakeShareable(new FMaterializeGraphSchemaAction_NewNode(
			Category, Name, Tooltip, 0
		));
		Action->NodeTemplate = NewObject<UMaterializeGraphNode>(ContextMenuBuilder.OwnerOfTemporaries, NodeClass);
		ContextMenuBuilder.AddAction(Action);
	};

	// Generators
	AddNodeAction(LOCTEXT("Generators", "Generators"), LOCTEXT("Noise", "Procedural Noise"),
		LOCTEXT("NoiseTooltip", "Generate procedural noise (Perlin, Worley, etc.)"), UMaterializeGraphNode_Noise::StaticClass());

	// Math
	AddNodeAction(LOCTEXT("Math", "Math"), LOCTEXT("MathOp", "Math Operation"),
		LOCTEXT("MathTooltip", "Per-pixel math (Add, Multiply, etc.)"), UMaterializeGraphNode_Math::StaticClass());

	// Blend
	AddNodeAction(LOCTEXT("Compositing", "Compositing"), LOCTEXT("Blend", "Blend"),
		LOCTEXT("BlendTooltip", "Blend two textures with blend modes"), UMaterializeGraphNode_Blend::StaticClass());

	// Filters
	AddNodeAction(LOCTEXT("Filters", "Filters"), LOCTEXT("Blur", "Blur"),
		LOCTEXT("BlurTooltip", "Apply Gaussian blur filter"), UMaterializeGraphNode_Blur::StaticClass());
	AddNodeAction(LOCTEXT("Filters", "Filters"), LOCTEXT("Sharpen", "Sharpen"),
		LOCTEXT("SharpenTooltip", "Apply sharpening filter"), UMaterializeGraphNode_Sharpen::StaticClass());
	AddNodeAction(LOCTEXT("Filters", "Filters"), LOCTEXT("EdgeDetect", "Edge Detect"),
		LOCTEXT("EdgeDetectTooltip", "Apply edge detection filter (Sobel)"), UMaterializeGraphNode_EdgeDetect::StaticClass());

	// Adjustments
	AddNodeAction(LOCTEXT("Adjustments", "Adjustments"), LOCTEXT("Levels", "Levels"),
		LOCTEXT("LevelsTooltip", "Adjust input/output levels and gamma"), UMaterializeGraphNode_Levels::StaticClass());
	AddNodeAction(LOCTEXT("Adjustments", "Adjustments"), LOCTEXT("Curves", "Curves"),
		LOCTEXT("CurvesTooltip", "Adjust brightness and contrast curves"), UMaterializeGraphNode_Curves::StaticClass());
	AddNodeAction(LOCTEXT("Adjustments", "Adjustments"), LOCTEXT("HSV", "HSV"),
		LOCTEXT("HSVTooltip", "Adjust hue, saturation, and value"), UMaterializeGraphNode_HSV::StaticClass());

	// Output
	AddNodeAction(LOCTEXT("Output", "Output"), LOCTEXT("Output", "Output"),
		LOCTEXT("OutputTooltip", "Final PBR output node (legacy)"), UMaterializeGraphNode_Output::StaticClass());
	
	// Channel Outputs (new architecture)
	AddNodeAction(LOCTEXT("Output", "Output"), LOCTEXT("ChannelOutput", "Channel Output"),
		LOCTEXT("ChannelOutputTooltip", "Individual output node for a specific PBR channel"), UMaterializeGraphNode_ChannelOutput::StaticClass());

	// Utility
	AddNodeAction(LOCTEXT("Utility", "Utility"), LOCTEXT("Reroute", "Reroute"),
		LOCTEXT("RerouteTooltip", "Reroute node for cable organization"), UMaterializeGraphNode_Knot::StaticClass());
}

const FPinConnectionResponse UMaterializeGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	// Basic validation
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pin"));
	}

	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect pins on the same node"));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect pins of the same direction"));
	}

	// Ensure we have Output -> Input ordering
	const UEdGraphPin* OutputPin = (A->Direction == EGPD_Output) ? A : B;
	const UEdGraphPin* InputPin = (A->Direction == EGPD_Input) ? A : B;

	// Get pin types
	const FName& OutputType = OutputPin->PinType.PinCategory;
	const FName& InputType = InputPin->PinType.PinCategory;

	// Pin type compatibility rules based on design document:
	// - Color (PC_Texture) can connect to Color or Vector
	// - Scalar can connect to Scalar
	// - Vector can connect to Vector or Color
	// - Normal can only connect to Normal
	// - Mask can connect to Scalar (mask is essentially a scalar channel)

	// Normal pins are strict - only Normal to Normal
	if (OutputType == PC_Normal || InputType == PC_Normal)
	{
		if (OutputType == PC_Normal && InputType == PC_Normal)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
		}
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Normal pins can only connect to other Normal pins"));
	}

	// Scalar compatibility: Scalar and Mask are compatible
	if (OutputType == PC_Scalar || OutputType == PC_Mask)
	{
		if (InputType == PC_Scalar || InputType == PC_Mask)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
		}
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Scalar/Mask pins can only connect to Scalar/Mask pins"));
	}

	// Texture (Color) and Vector are compatible with each other
	if (OutputType == PC_Texture || OutputType == PC_Vector)
	{
		if (InputType == PC_Texture || InputType == PC_Vector)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
		}
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Color/Vector pins can only connect to Color/Vector pins"));
	}

	// Fallback for any unhandled cases
	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Incompatible pin types"));
}

bool UMaterializeGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	if (A && B)
	{
		UEdGraphPin* Output = (A->Direction == EGPD_Output) ? A : B;
		UEdGraphPin* Input = (A->Direction == EGPD_Input) ? A : B;

		if (CanCreateConnection(Output, Input).Response == CONNECT_RESPONSE_DISALLOW)
		{
			return false;
		}

		// Materialize graph logic supports Single Input <- Single Output connection for texture flow
		// But outputs can drive multiple inputs.
		if (Input->LinkedTo.Num() > 0)
		{
			Input->BreakAllPinLinks();
		}

		Output->MakeLinkTo(Input);
		return true;
	}
	return false;
}

FLinearColor UMaterializeGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	// Type-based pin colors matching wire colors
	if (PinType.PinCategory == PC_Texture) return FLinearColor(0.9f, 0.5f, 0.1f);  // Orange - Color/RGBA
	if (PinType.PinCategory == PC_Scalar)  return FLinearColor(0.5f, 0.5f, 0.5f);  // Gray - Single channel
	if (PinType.PinCategory == PC_Vector)  return FLinearColor(0.2f, 0.4f, 0.9f);  // Blue - Multi-channel
	if (PinType.PinCategory == PC_Mask)    return FLinearColor(0.7f, 0.7f, 0.7f);  // Light Gray - Mask/alpha
	if (PinType.PinCategory == PC_Normal)  return FLinearColor(0.2f, 0.8f, 0.8f);  // Cyan - Normal map
	return FLinearColor::White;
}

void UMaterializeGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);
}

void UMaterializeGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(LOCTEXT("BreakPinLinks", "Break Pin Links"));
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
}

int32 UMaterializeGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	// Count selected nodes
	if (!Graph) return 0;
	
	int32 Count = 0;
	// Helper logic usually involves the editor engine's selection set
	return Count; 
}

TSharedPtr<FEdGraphSchemaAction> UMaterializeGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(new FEdGraphSchemaAction_NewNode(FText::GetEmpty(), LOCTEXT("AddComment", "Add Comment"), FText::GetEmpty(), 0));
}

FConnectionDrawingPolicy* UMaterializeGraphSchema::CreateConnectionDrawingPolicy(
	int32 InBackLayerID, 
	int32 InFrontLayerID, 
	float InZoomFactor, 
	const FSlateRect& InClippingRect, 
	FSlateWindowElementList& InDrawElements, 
	UEdGraph* InGraphObj) const
{
	return new FMaterializeConnectionDrawingPolicy(
		InBackLayerID, 
		InFrontLayerID, 
		InZoomFactor, 
		InClippingRect, 
		InDrawElements, 
		InGraphObj);
}

#undef LOCTEXT_NAMESPACE

