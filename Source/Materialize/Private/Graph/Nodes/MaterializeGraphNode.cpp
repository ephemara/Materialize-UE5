#include "Graph/Nodes/MaterializeGraphNode.h"
#include "Graph/MaterializeGraphRuntime.h"
#include "Engine/Texture.h"
#include "EdGraph/EdGraphPin.h"

UMaterializeGraphNode::UMaterializeGraphNode()
{
	bCanRenameNode = true;
	
	// Generate a unique GUID for this node instance
	if (!NodeGuid.IsValid())
	{
		NodeGuid = FGuid::NewGuid();
	}
}

void UMaterializeGraphNode::AllocateDefaultPins()
{
	// Base class does nothing, derived nodes will implement this
}

FLinearColor UMaterializeGraphNode::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.2f, 0.2f); // Dark Grey
}

FText UMaterializeGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Materialize Node"));
}

UMaterializeGraph* UMaterializeGraphNode::GetKSampleGraph() const
{
	return Cast<UMaterializeGraph>(GetGraph());
}

void UMaterializeGraphNode::InvalidatePreview()
{
	// Clear cached preview texture
	PreviewTexture = nullptr;
	
	// Notify listeners that preview needs regeneration
	OnPreviewInvalidated.ExecuteIfBound();
	
	// Mark package as potentially dirty
	if (UMaterializeGraph* Graph = GetKSampleGraph())
	{
		Graph->MarkPackageDirty();
	}
}

FGuid UMaterializeGraphNode::GetNodeGuid() const
{
	// Ensure we have a valid GUID
	if (!NodeGuid.IsValid())
	{
		// This is const, but we need to generate a GUID
		// In practice, the constructor should have already done this
		const_cast<UMaterializeGraphNode*>(this)->NodeGuid = FGuid::NewGuid();
	}
	return NodeGuid;
}

UTexture2D* UMaterializeGraphNode::GetInputTexture(UEdGraphPin* Pin, FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	if (!Pin || Pin->LinkedTo.Num() == 0)
	{
		return nullptr;
	}

	// Get the connected pin
	UEdGraphPin* LinkedPin = Pin->LinkedTo[0];
	if (!LinkedPin || !LinkedPin->GetOwningNode())
	{
		return nullptr;
	}

	// Get the source node
	UMaterializeGraphNode* SourceNode = Cast<UMaterializeGraphNode>(LinkedPin->GetOwningNode());
	if (!SourceNode)
	{
		return nullptr;
	}

	// Execute the source node to get its output
	return SourceNode->Execute(Context, Width, Height);
}

