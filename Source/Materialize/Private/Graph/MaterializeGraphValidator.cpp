// Copyright K-Studio. All Rights Reserved.

#include "Graph/MaterializeGraphValidator.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "Graph/Nodes/MaterializeGraphNode_Output.h"
#include "EdGraph/EdGraphPin.h"
#include "MaterializeValidation.h"

// ============================================================================
// FMaterializeGraphValidationResult
// ============================================================================

FString FMaterializeGraphValidationResult::GetSummary() const
{
	if (!HasIssues())
	{
		return TEXT("Graph is valid");
	}
	
	FString Summary;
	
	if (Errors.Num() > 0)
	{
		Summary += FString::Printf(TEXT("%d Error(s): "), Errors.Num());
		for (const FString& Error : Errors)
		{
			Summary += Error + TEXT("; ");
		}
	}
	
	if (Warnings.Num() > 0)
	{
		if (!Summary.IsEmpty()) Summary += TEXT(" | ");
		Summary += FString::Printf(TEXT("%d Warning(s): "), Warnings.Num());
		for (const FString& Warning : Warnings)
		{
			Summary += Warning + TEXT("; ");
		}
	}
	
	return Summary;
}

// ============================================================================
// FMaterializeGraphValidator - Public Methods
// ============================================================================

bool FMaterializeGraphValidator::ValidateGraph(UMaterializeGraph* Graph, FMaterializeGraphValidationResult& OutResult)
{
	MATERIALIZE_CHECK_PTR(Graph, false);
	
	OutResult = FMaterializeGraphValidationResult();
	OutResult.bIsValid = true;
	
	// 1. Check for cycles
	if (DetectCycles(Graph, OutResult.CycleNodes))
	{
		OutResult.Errors.Add(FString::Printf(TEXT("Graph contains %d node(s) in cycles"), OutResult.CycleNodes.Num()));
		OutResult.bIsValid = false;
	}
	
	// 2. Find disconnected nodes
	OutResult.DisconnectedNodes = FindDisconnectedNodes(Graph);
	if (OutResult.DisconnectedNodes.Num() > 0)
	{
		OutResult.Warnings.Add(FString::Printf(TEXT("Graph contains %d disconnected node(s)"), OutResult.DisconnectedNodes.Num()));
	}
	
	// 3. Validate all node parameters
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UMaterializeGraphNode* MatNode = Cast<UMaterializeGraphNode>(Node);
		if (!MatNode) continue;
		
		TArray<FString> ParamErrors;
		if (!ValidateNodeParams(MatNode, ParamErrors))
		{
			OutResult.InvalidNodeParams.Add(MatNode, ParamErrors);
			OutResult.Errors.Append(ParamErrors);
			OutResult.bIsValid = false;
		}
	}
	
	// 4. Check for output nodes
	TArray<UMaterializeGraphNode*> OutputNodes = GetOutputNodes(Graph);
	if (OutputNodes.Num() == 0)
	{
		OutResult.Errors.Add(TEXT("Graph has no output node"));
		OutResult.bIsValid = false;
	}
	else if (OutputNodes.Num() > 1)
	{
		OutResult.Warnings.Add(FString::Printf(TEXT("Graph has %d output nodes (only first will be used)"), OutputNodes.Num()));
	}
	
	return OutResult.bIsValid;
}

bool FMaterializeGraphValidator::DetectCycles(UMaterializeGraph* Graph, TArray<UMaterializeGraphNode*>& OutCycleNodes)
{
	MATERIALIZE_CHECK_PTR(Graph, false);
	
	OutCycleNodes.Empty();
	
	TSet<UMaterializeGraphNode*> Visited;
	TSet<UMaterializeGraphNode*> RecursionStack;
	
	// Run DFS from each unvisited node
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UMaterializeGraphNode* MatNode = Cast<UMaterializeGraphNode>(Node);
		if (!MatNode || Visited.Contains(MatNode))
		{
			continue;
		}
		
		if (DetectCyclesRecursive(MatNode, Visited, RecursionStack, OutCycleNodes))
		{
			// Cycle detected, continue to find all cycles
		}
	}
	
	return OutCycleNodes.Num() > 0;
}

bool FMaterializeGraphValidator::CanConnectPins(const UEdGraphPin* SourcePin, const UEdGraphPin* TargetPin, FString& OutReason)
{
	MATERIALIZE_CHECK_PTR(SourcePin, false);
	MATERIALIZE_CHECK_PTR(TargetPin, false);
	
	// 1. Check pin directions
	if (SourcePin->Direction == TargetPin->Direction)
	{
		OutReason = TEXT("Cannot connect pins with same direction");
		return false;
	}
	
	// Ensure source is output and target is input
	const UEdGraphPin* OutputPin = (SourcePin->Direction == EGPD_Output) ? SourcePin : TargetPin;
	const UEdGraphPin* InputPin = (SourcePin->Direction == EGPD_Input) ? SourcePin : TargetPin;
	
	// 2. Check if input already has a connection
	if (InputPin->LinkedTo.Num() > 0)
	{
		OutReason = TEXT("Input pin already has a connection");
		return false;
	}
	
	// 3. Check pin types
	if (OutputPin->PinType.PinCategory != InputPin->PinType.PinCategory)
	{
		// Allow some implicit conversions
		bool bCanConvert = false;
		
		// Float to Vector (broadcast)
		if (OutputPin->PinType.PinCategory == TEXT("float") && InputPin->PinType.PinCategory == TEXT("vector"))
		{
			bCanConvert = true;
		}
		// Vector to Float (magnitude or component)
		else if (OutputPin->PinType.PinCategory == TEXT("vector") && InputPin->PinType.PinCategory == TEXT("float"))
		{
			bCanConvert = true;
		}
		
		if (!bCanConvert)
		{
			OutReason = FString::Printf(TEXT("Incompatible pin types: %s -> %s"), 
				*OutputPin->PinType.PinCategory.ToString(), 
				*InputPin->PinType.PinCategory.ToString());
			return false;
		}
	}
	
	// 4. Check for self-connection
	if (OutputPin->GetOwningNode() == InputPin->GetOwningNode())
	{
		OutReason = TEXT("Cannot connect node to itself");
		return false;
	}
	
	return true;
}

bool FMaterializeGraphValidator::ValidateNodeParams(UMaterializeGraphNode* Node, TArray<FString>& OutErrors)
{
	MATERIALIZE_CHECK_PTR(Node, false);
	
	OutErrors.Empty();
	
	// Get node-specific validation
	// This would be overridden by specific node types
	// For now, just check basic validity
	
	// Example: Check if node has required pins
	bool bHasInputs = false;
	bool bHasOutputs = false;
	
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == EGPD_Input) bHasInputs = true;
		if (Pin->Direction == EGPD_Output) bHasOutputs = true;
	}
	
	// Output nodes don't need outputs
	if (!IsOutputNode(Node) && !bHasOutputs)
	{
		OutErrors.Add(FString::Printf(TEXT("Node '%s' has no output pins"), *Node->GetNodeTitle(ENodeTitleType::ListView).ToString()));
	}
	
	return OutErrors.Num() == 0;
}

TArray<UMaterializeGraphNode*> FMaterializeGraphValidator::FindDisconnectedNodes(UMaterializeGraph* Graph)
{
	MATERIALIZE_CHECK_PTR(Graph, TArray<UMaterializeGraphNode*>());
	
	TArray<UMaterializeGraphNode*> DisconnectedNodes;
	
	// Get all output nodes
	TArray<UMaterializeGraphNode*> OutputNodes = GetOutputNodes(Graph);
	if (OutputNodes.Num() == 0)
	{
		// No output nodes, all nodes are disconnected
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UMaterializeGraphNode* MatNode = Cast<UMaterializeGraphNode>(Node))
			{
				DisconnectedNodes.Add(MatNode);
			}
		}
		return DisconnectedNodes;
	}
	
	// Find all nodes reachable from output nodes (backward traversal)
	TSet<UMaterializeGraphNode*> ReachableNodes;
	for (UMaterializeGraphNode* OutputNode : OutputNodes)
	{
		// Traverse backwards from output
		TSet<UMaterializeGraphNode*> Visited;
		TArray<UMaterializeGraphNode*> Stack;
		Stack.Add(OutputNode);
		
		while (Stack.Num() > 0)
		{
			UMaterializeGraphNode* Current = Stack.Pop();
			if (Visited.Contains(Current)) continue;
			Visited.Add(Current);
			ReachableNodes.Add(Current);
			
			// Add all nodes connected to inputs
			for (UEdGraphPin* Pin : Current->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
					{
						if (UMaterializeGraphNode* LinkedNode = Cast<UMaterializeGraphNode>(LinkedPin->GetOwningNode()))
						{
							Stack.Add(LinkedNode);
						}
					}
				}
			}
		}
	}
	
	// Find nodes not reachable from output
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UMaterializeGraphNode* MatNode = Cast<UMaterializeGraphNode>(Node);
		if (MatNode && !ReachableNodes.Contains(MatNode))
		{
			DisconnectedNodes.Add(MatNode);
		}
	}
	
	return DisconnectedNodes;
}

bool FMaterializeGraphValidator::ValidateConnection(UMaterializeGraph* Graph, const UEdGraphPin* SourcePin, const UEdGraphPin* TargetPin, FString& OutReason)
{
	// First check basic pin compatibility
	if (!CanConnectPins(SourcePin, TargetPin, OutReason))
	{
		return false;
	}
	
	// Check if connection would create a cycle
	UMaterializeGraphNode* SourceNode = Cast<UMaterializeGraphNode>(SourcePin->GetOwningNode());
	UMaterializeGraphNode* TargetNode = Cast<UMaterializeGraphNode>(TargetPin->GetOwningNode());
	
	if (WouldCreateCycle(SourceNode, TargetNode))
	{
		OutReason = TEXT("Connection would create a cycle");
		return false;
	}
	
	return true;
}

// ============================================================================
// FMaterializeGraphValidator - Private Methods
// ============================================================================

bool FMaterializeGraphValidator::DetectCyclesRecursive(
	UMaterializeGraphNode* Node,
	TSet<UMaterializeGraphNode*>& Visited,
	TSet<UMaterializeGraphNode*>& RecursionStack,
	TArray<UMaterializeGraphNode*>& OutCycleNodes)
{
	MATERIALIZE_CHECK_PTR(Node, false);
	
	Visited.Add(Node);
	RecursionStack.Add(Node);
	
	// Visit all nodes connected to outputs
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				UMaterializeGraphNode* LinkedNode = Cast<UMaterializeGraphNode>(LinkedPin->GetOwningNode());
				if (!LinkedNode) continue;
				
				if (!Visited.Contains(LinkedNode))
				{
					// Recurse
					if (DetectCyclesRecursive(LinkedNode, Visited, RecursionStack, OutCycleNodes))
					{
						return true;
					}
				}
				else if (RecursionStack.Contains(LinkedNode))
				{
					// Cycle detected!
					OutCycleNodes.AddUnique(LinkedNode);
					OutCycleNodes.AddUnique(Node);
					return true;
				}
			}
		}
	}
	
	RecursionStack.Remove(Node);
	return false;
}

bool FMaterializeGraphValidator::WouldCreateCycle(UMaterializeGraphNode* SourceNode, UMaterializeGraphNode* TargetNode)
{
	MATERIALIZE_CHECK_PTR(SourceNode, false);
	MATERIALIZE_CHECK_PTR(TargetNode, false);
	
	// Check if TargetNode can already reach SourceNode
	// If yes, adding SourceNode -> TargetNode would create a cycle
	TSet<UMaterializeGraphNode*> Reachable;
	GetReachableNodes(TargetNode, Reachable);
	
	return Reachable.Contains(SourceNode);
}

void FMaterializeGraphValidator::GetReachableNodes(UMaterializeGraphNode* StartNode, TSet<UMaterializeGraphNode*>& OutReachable)
{
	MATERIALIZE_CHECK_PTR(StartNode, );
	
	TArray<UMaterializeGraphNode*> Stack;
	Stack.Add(StartNode);
	
	while (Stack.Num() > 0)
	{
		UMaterializeGraphNode* Current = Stack.Pop();
		if (OutReachable.Contains(Current)) continue;
		OutReachable.Add(Current);
		
		// Add all nodes connected to outputs
		for (UEdGraphPin* Pin : Current->Pins)
		{
			if (Pin->Direction == EGPD_Output)
			{
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					if (UMaterializeGraphNode* LinkedNode = Cast<UMaterializeGraphNode>(LinkedPin->GetOwningNode()))
					{
						Stack.Add(LinkedNode);
					}
				}
			}
		}
	}
}

bool FMaterializeGraphValidator::IsOutputNode(UMaterializeGraphNode* Node)
{
	return Node && Node->IsA<UMaterializeGraphNode_Output>();
}

TArray<UMaterializeGraphNode*> FMaterializeGraphValidator::GetOutputNodes(UMaterializeGraph* Graph)
{
	TArray<UMaterializeGraphNode*> OutputNodes;
	
	MATERIALIZE_CHECK_PTR(Graph, OutputNodes);
	
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UMaterializeGraphNode* MatNode = Cast<UMaterializeGraphNode>(Node))
		{
			if (IsOutputNode(MatNode))
			{
				OutputNodes.Add(MatNode);
			}
		}
	}
	
	return OutputNodes;
}
