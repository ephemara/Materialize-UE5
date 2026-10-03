#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "MaterializeGraph.generated.h"

class UMaterializeGraphSchema;
class UMaterializeGraphNode;

DECLARE_MULTICAST_DELEGATE(FOnMaterializeGraphStructureChanged);

UCLASS()
class MATERIALIZE_API UMaterializeGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	UMaterializeGraph();

	/** Fired when graph structure changes (connections, nodes added/removed). */
	FOnMaterializeGraphStructureChanged OnGraphStructureChanged;

	//~ Begin UEdGraph Interface
	virtual void NotifyGraphChanged(const FEdGraphEditAction& Action) override;
	//~ End UEdGraph Interface

	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

	/**
	 * Add a new node to the graph
	 * @param NodeClass The class of node to create
	 * @param Position The position in graph space to place the node
	 * @return The newly created node, or nullptr if creation failed
	 */
	UMaterializeGraphNode* AddNode(TSubclassOf<UMaterializeGraphNode> NodeClass, const FVector2D& Position);
	
	// Expose base class AddNode to avoid hiding warning
	using UEdGraph::AddNode;

	/**
	 * Remove a node from the graph
	 * @param Node The node to remove
	 * @return True if the node was successfully removed
	 */
	bool RemoveNode(UMaterializeGraphNode* Node);

	/**
	 * Connect two nodes together
	 * @param OutputNode The node providing the output
	 * @param OutputPinIndex The index of the output pin on OutputNode
	 * @param InputNode The node receiving the input
	 * @param InputPinIndex The index of the input pin on InputNode
	 * @return True if the connection was successfully created
	 */
	bool ConnectNodes(UMaterializeGraphNode* OutputNode, int32 OutputPinIndex, UMaterializeGraphNode* InputNode, int32 InputPinIndex);
};
