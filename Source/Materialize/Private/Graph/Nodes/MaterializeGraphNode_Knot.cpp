// Copyright K-Studio. All Rights Reserved.

#include "Graph/Nodes/MaterializeGraphNode_Knot.h"
#include "Graph/MaterializeGraphSchema.h"

#define LOCTEXT_NAMESPACE "KSampleGraphNode_Knot"

UMaterializeGraphNode_Knot::UMaterializeGraphNode_Knot()
{
	bCanRenameNode = false;
}

UTexture2D* UMaterializeGraphNode_Knot::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Pass through whichever pin is connected (input preferred)
	if (UEdGraphPin* InputPin = GetInputPin())
	{
		if (InputPin->LinkedTo.Num() > 0)
		{
			return GetInputTexture(InputPin, Context, Width, Height);
		}
	}

	if (UEdGraphPin* OutputPin = GetOutputPin())
	{
		if (OutputPin->LinkedTo.Num() > 0)
		{
			return GetInputTexture(OutputPin->LinkedTo[0], Context, Width, Height);
		}
	}

	return nullptr;
}

void UMaterializeGraphNode_Knot::AllocateDefaultPins()
{
	// Single input and output pin that passes data through
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("In"));
	CreatePin(EGPD_Output, UMaterializeGraphSchema::PC_Texture, TEXT("Out"));
}

FText UMaterializeGraphNode_Knot::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::GetEmpty();
}

FText UMaterializeGraphNode_Knot::GetTooltipText() const
{
	return LOCTEXT("KnotTooltip", "Reroute Node - Drag to organize cables");
}

FLinearColor UMaterializeGraphNode_Knot::GetNodeTitleColor() const
{
	// Dark neutral color for the knot
	return FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

bool UMaterializeGraphNode_Knot::ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const
{
	// Draw as a small control point instead of full node
	OutInputPinIndex = 0;
	OutOutputPinIndex = 1;
	return true;
}

UEdGraphPin* UMaterializeGraphNode_Knot::GetInputPin() const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			return Pin;
		}
	}
	return nullptr;
}

UEdGraphPin* UMaterializeGraphNode_Knot::GetOutputPin() const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			return Pin;
		}
	}
	return nullptr;
}

FEdGraphPinType UMaterializeGraphNode_Knot::GetPassThroughPinType() const
{
	// Determine type from connected pins
	if (UEdGraphPin* InputPin = GetInputPin())
	{
		if (InputPin->LinkedTo.Num() > 0)
		{
			return InputPin->LinkedTo[0]->PinType;
		}
	}
	
	if (UEdGraphPin* OutputPin = GetOutputPin())
	{
		if (OutputPin->LinkedTo.Num() > 0)
		{
			return OutputPin->LinkedTo[0]->PinType;
		}
	}
	
	// Default to texture type
	FEdGraphPinType DefaultType;
	DefaultType.PinCategory = UMaterializeGraphSchema::PC_Texture;
	return DefaultType;
}

void UMaterializeGraphNode_Knot::UpdatePinTypes()
{
	FEdGraphPinType PassThroughType = GetPassThroughPinType();
	
	if (UEdGraphPin* InputPin = GetInputPin())
	{
		InputPin->PinType = PassThroughType;
	}
	
	if (UEdGraphPin* OutputPin = GetOutputPin())
	{
		OutputPin->PinType = PassThroughType;
	}
}

#undef LOCTEXT_NAMESPACE
