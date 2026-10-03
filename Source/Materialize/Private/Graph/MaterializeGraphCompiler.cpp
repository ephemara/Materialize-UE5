#include "Graph/MaterializeGraphCompiler.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "Graph/Nodes/MaterializeGraphNode_Noise.h"
#include "KLayerEvaluator.h"

bool FMaterializeGraphCompiler::Compile(UMaterializeGraph* Graph)
{
	CompiledOps.Empty();
	Errors.Empty();
	TextureCache.Empty();

	if (!Graph)
	{
		Errors.Add(TEXT("Graph is null"));
		return false;
	}

	TSet<FGuid> Visited;

	// Traverse all nodes (in future, start from output node and work backwards)
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UMaterializeGraphNode* KNode = Cast<UMaterializeGraphNode>(Node))
		{
			TraverseNode(KNode, Visited);
		}
	}

	return Errors.Num() == 0;
}

void FMaterializeGraphCompiler::TraverseNode(UMaterializeGraphNode* Node, TSet<FGuid>& Visited)
{
	if (!Node) return;
	if (Visited.Contains(Node->NodeGuid)) return;
	Visited.Add(Node->NodeGuid);

	// First, traverse all input nodes (dependencies)
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
		{
			if (UMaterializeGraphNode* InputNode = Cast<UMaterializeGraphNode>(Pin->LinkedTo[0]->GetOwningNode()))
			{
				TraverseNode(InputNode, Visited);
			}
		}
	}

	// Now compile this node
	FMaterializeCompiledOp Op;
	Op.NodeId = Node->NodeGuid;

	// Determine node type and create execution lambda
	if (UMaterializeGraphNode_Noise* NoiseNode = Cast<UMaterializeGraphNode_Noise>(Node))
	{
		FKProceduralParams Params = NoiseNode->Params;
		Op.Execute = [Params](const TMap<FGuid, UTexture2D*>& Inputs, int32 Width, int32 Height) -> UTexture2D*
		{
			FString Error;
			return UKLayerEvaluator::GenerateProceduralTexture(Params, Width, Height, Error);
		};
	}
	else
	{
		// Unknown node type - create passthrough
		Op.Execute = [](const TMap<FGuid, UTexture2D*>& Inputs, int32 Width, int32 Height) -> UTexture2D*
		{
			return nullptr;
		};
	}

	CompiledOps.Add(MoveTemp(Op));
}

UTexture2D* FMaterializeGraphCompiler::Execute(int32 Width, int32 Height)
{
	TextureCache.Empty();

	UTexture2D* LastResult = nullptr;

	for (const FMaterializeCompiledOp& Op : CompiledOps)
	{
		UTexture2D* Result = Op.Execute(TextureCache, Width, Height);
		if (Result)
		{
			TextureCache.Add(Op.NodeId, Result);
			LastResult = Result;
		}
	}

	return LastResult;
}
