#include "Graph/MaterializeGraph.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraph/EdGraphNode.h"

UMaterializeGraph::UMaterializeGraph()
{
	Schema = UMaterializeGraphSchema::StaticClass();
}

void UMaterializeGraph::NotifyGraphChanged(const FEdGraphEditAction& Action)
{
	Super::NotifyGraphChanged(Action);
	
	// Notify listeners (e.g. the editor) so they can re-execute the graph
	OnGraphStructureChanged.Broadcast();
}

void UMaterializeGraph::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	
	// Serialize our custom Nodes array
	// Note: UEdGraph already serializes its Nodes array, but we maintain our own
	// typed array for easier access to UMaterializeGraphNode-specific functionality
	Ar << Nodes;
}

UMaterializeGraphNode* UMaterializeGraph::AddNode(TSubclassOf<UMaterializeGraphNode> NodeClass, const FVector2D& Position)
{
	if (!NodeClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UMaterializeGraph::AddNode: NodeClass is null"));
		return nullptr;
	}

	// Create the node instance
	UMaterializeGraphNode* NewNode = NewObject<UMaterializeGraphNode>(this, NodeClass, NAME_None, RF_Transactional);
	if (!NewNode)
	{
		UE_LOG(LogTemp, Error, TEXT("UMaterializeGraph::AddNode: Failed to create node of class %s"), *NodeClass->GetName());
		return nullptr;
	}

	// Set the node's position
	NewNode->NodePosX = Position.X;
	NewNode->NodePosY = Position.Y;

	// Create a unique GUID for the node
	NewNode->CreateNewGuid();

	// Allocate default pins for the node
	NewNode->AllocateDefaultPins();

	// Add to both our typed array and the base EdGraph's Nodes array
	Nodes.Add(NewNode);
	UEdGraph::AddNode(NewNode, true, true);

	// Mark the graph as modified
	// Create empty action struct (FEdGraphEditAction is forward declared, so we can't construct it properly)
	// Just call the base class version which will handle the notification
	Super::NotifyGraphChanged();
	MarkPackageDirty();

	return NewNode;
}

bool UMaterializeGraph::RemoveNode(UMaterializeGraphNode* Node)
{
	if (!Node)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMaterializeGraph::RemoveNode: Node is null"));
		return false;
	}

	if (!Nodes.Contains(Node))
	{
		UE_LOG(LogTemp, Warning, TEXT("UMaterializeGraph::RemoveNode: Node not found in graph"));
		return false;
	}

	// Break all pin connections before removing the node
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			Pin->BreakAllPinLinks();
		}
	}

	// Remove from our typed array
	Nodes.Remove(Node);

	// Remove from the base EdGraph's Nodes array
	UEdGraph::RemoveNode(Node, true);

	// Mark the graph as modified
	Super::NotifyGraphChanged();
	MarkPackageDirty();

	return true;
}

bool UMaterializeGraph::ConnectNodes(UMaterializeGraphNode* OutputNode, int32 OutputPinIndex, UMaterializeGraphNode* InputNode, int32 InputPinIndex)
{
	if (!OutputNode || !InputNode)
	{
		UE_LOG(LogTemp, Error, TEXT("UMaterializeGraph::ConnectNodes: OutputNode or InputNode is null"));
		return false;
	}

	// Validate pin indices
	if (!OutputNode->Pins.IsValidIndex(OutputPinIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("UMaterializeGraph::ConnectNodes: Invalid OutputPinIndex %d for node %s"), 
			OutputPinIndex, *OutputNode->GetName());
		return false;
	}

	if (!InputNode->Pins.IsValidIndex(InputPinIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("UMaterializeGraph::ConnectNodes: Invalid InputPinIndex %d for node %s"), 
			InputPinIndex, *InputNode->GetName());
		return false;
	}

	UEdGraphPin* OutputPin = OutputNode->Pins[OutputPinIndex];
	UEdGraphPin* InputPin = InputNode->Pins[InputPinIndex];

	if (!OutputPin || !InputPin)
	{
		UE_LOG(LogTemp, Error, TEXT("UMaterializeGraph::ConnectNodes: OutputPin or InputPin is null"));
		return false;
	}

	// Validate pin directions
	if (OutputPin->Direction != EGPD_Output)
	{
		UE_LOG(LogTemp, Error, TEXT("UMaterializeGraph::ConnectNodes: OutputPin is not an output pin"));
		return false;
	}

	if (InputPin->Direction != EGPD_Input)
	{
		UE_LOG(LogTemp, Error, TEXT("UMaterializeGraph::ConnectNodes: InputPin is not an input pin"));
		return false;
	}

	// Use the schema to validate the connection
	const UMaterializeGraphSchema* GraphSchema = Cast<UMaterializeGraphSchema>(GetSchema());
	if (GraphSchema)
	{
		const FPinConnectionResponse Response = GraphSchema->CanCreateConnection(OutputPin, InputPin);
		if (!Response.CanSafeConnect())
		{
			UE_LOG(LogTemp, Warning, TEXT("UMaterializeGraph::ConnectNodes: Schema rejected connection: %s"), 
				*Response.Message.ToString());
			return false;
		}
	}

	// Create the connection
	OutputPin->MakeLinkTo(InputPin);

	// Mark the graph as modified
	Super::NotifyGraphChanged();
	MarkPackageDirty();

	return true;
}
