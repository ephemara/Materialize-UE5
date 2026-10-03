#include "Editor/MaterializeGraphNodeFactory.h"
#include "Editor/Widgets/SMaterializeGraphNode.h"
#include "Graph/Nodes/MaterializeGraphNode.h"

TSharedPtr<SGraphNode> FMaterializeGraphNodeFactory::CreateNode(UEdGraphNode* InNode) const
{
	if (UMaterializeGraphNode* KNode = Cast<UMaterializeGraphNode>(InNode))
	{
		TSharedPtr<SMaterializeGraphNode> GraphNode = SNew(SMaterializeGraphNode, KNode);
		return GraphNode;
	}

	return nullptr;
}
