// Copyright K-Studio. All Rights Reserved.

#include "Graph/MaterializeGraphExecutor.h"
#include "Graph/MaterializeGraphRuntime.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "Graph/Nodes/MaterializeGraphNode_Noise.h"
#include "Graph/Nodes/MaterializeGraphNode_Blend.h"
#include "Graph/Nodes/MaterializeGraphNode_Filter.h"
#include "Graph/Nodes/MaterializeGraphNode_Adjustment.h"
#include "Graph/Nodes/MaterializeGraphNode_Math.h"
#include "Graph/Nodes/MaterializeGraphNode_Output.h"
#include "Graph/Nodes/MaterializeGraphNode_ChannelOutput.h"
#include "KLayerEvaluator.h"
#include "Engine/Texture2D.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHICommandList.h"
#include "TextureResource.h"
#include "Algo/Reverse.h"

FMaterializeGraphExecutor::FMaterializeGraphExecutor()
{
}

FMaterializeGraphExecutor::~FMaterializeGraphExecutor()
{
	ClearCache();
}

void FMaterializeGraphExecutor::ClearCache()
{
	TextureCache.Empty();
}

bool FMaterializeGraphExecutor::CompileGraph(UMaterializeGraph* Graph, TArray<UMaterializeGraphNode*>& OutExecutionOrder, FString& OutError)
{
	OutExecutionOrder.Empty();
	OutError.Empty();

	if (!Graph)
	{
		OutError = TEXT("Graph is null");
		return false;
	}

	if (Graph->Nodes.Num() == 0)
	{
		OutError = TEXT("Graph has no nodes");
		return false;
	}

	// Step 1: Validate graph topology (check for cycles and connectivity)
	if (!ValidateGraphTopology(Graph, OutError))
	{
		return false;
	}

	// Step 2: Perform topological sort to determine execution order
	TopologicalSort(Graph, OutExecutionOrder);

	if (OutExecutionOrder.Num() == 0)
	{
		OutError = TEXT("Topological sort produced empty execution order");
		return false;
	}

	return true;
}

bool FMaterializeGraphExecutor::ValidateGraphTopology(UMaterializeGraph* Graph, FString& OutError)
{
	OutError.Empty();

	if (!Graph)
	{
		OutError = TEXT("Graph is null");
		return false;
	}

	// Check for at least one output node
	TArray<UMaterializeGraphNode_ChannelOutput*> ChannelOutputNodes = FindChannelOutputNodes(Graph);
	UMaterializeGraphNode_Output* LegacyOutputNode = FindOutputNode(Graph);

	if (ChannelOutputNodes.Num() == 0 && !LegacyOutputNode)
	{
		OutError = TEXT("Graph has no output nodes. At least one output node is required.");
		return false;
	}

	// Check for required output channels (at least BaseColor, Normal, Roughness)
	if (ChannelOutputNodes.Num() > 0)
	{
		TSet<EMaterializeOutputChannel> FoundChannels;
		for (UMaterializeGraphNode_ChannelOutput* ChannelNode : ChannelOutputNodes)
		{
			FoundChannels.Add(ChannelNode->OutputChannel);
		}

		// Validate that critical channels are present
		if (!FoundChannels.Contains(EMaterializeOutputChannel::BaseColor))
		{
			OutError = TEXT("Graph is missing required BaseColor output node");
			return false;
		}
	}

	// Detect cycles in the graph
	if (DetectCycles(Graph))
	{
		OutError = TEXT("Graph contains cycles. Cyclic dependencies are not allowed.");
		return false;
	}

	// Validate that all nodes have valid connections (no dangling required inputs)
	for (UEdGraphNode* EdNode : Graph->Nodes)
	{
		if (UMaterializeGraphNode* KNode = Cast<UMaterializeGraphNode>(EdNode))
		{
			// Skip output nodes - they're terminals
			if (Cast<UMaterializeGraphNode_Output>(KNode) || Cast<UMaterializeGraphNode_ChannelOutput>(KNode))
			{
				continue;
			}

			// Check if node has any output connections (unless it's an output node)
			bool bHasOutputConnection = false;
			for (UEdGraphPin* Pin : KNode->Pins)
			{
				if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
				{
					bHasOutputConnection = true;
					break;
				}
			}

			// Noise nodes don't need input connections, but should have output connections
			if (Cast<UMaterializeGraphNode_Noise>(KNode))
			{
				if (!bHasOutputConnection)
				{
					UE_LOG(LogTemp, Warning, TEXT("Noise node '%s' has no output connections and will not contribute to final output"), 
						*KNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
				}
			}
		}
	}

	return true;
}

bool FMaterializeGraphExecutor::DetectCycles(UMaterializeGraph* Graph)
{
	if (!Graph)
	{
		return false;
	}

	TSet<FGuid> Visited;
	TSet<FGuid> RecursionStack;

	// Start DFS from each node to detect cycles
	for (UEdGraphNode* EdNode : Graph->Nodes)
	{
		if (UMaterializeGraphNode* KNode = Cast<UMaterializeGraphNode>(EdNode))
		{
			if (!Visited.Contains(KNode->NodeGuid))
			{
				if (DetectCyclesDFS(KNode, Visited, RecursionStack))
				{
					return true; // Cycle detected
				}
			}
		}
	}

	return false; // No cycles
}

bool FMaterializeGraphExecutor::DetectCyclesDFS(UMaterializeGraphNode* Node, TSet<FGuid>& Visited, TSet<FGuid>& RecursionStack)
{
	if (!Node)
	{
		return false;
	}

	// Mark current node as visited and add to recursion stack
	Visited.Add(Node->NodeGuid);
	RecursionStack.Add(Node->NodeGuid);

	// Visit all nodes connected to this node's input pins (dependencies)
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (LinkedPin)
				{
					UMaterializeGraphNode* LinkedNode = Cast<UMaterializeGraphNode>(LinkedPin->GetOwningNode());
					if (LinkedNode)
					{
						// If the linked node is in the recursion stack, we found a cycle
						if (RecursionStack.Contains(LinkedNode->NodeGuid))
						{
							UE_LOG(LogTemp, Error, TEXT("Cycle detected: Node '%s' depends on '%s' which creates a cycle"),
								*Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(),
								*LinkedNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
							return true;
						}

						// If not visited, recurse
						if (!Visited.Contains(LinkedNode->NodeGuid))
						{
							if (DetectCyclesDFS(LinkedNode, Visited, RecursionStack))
							{
								return true;
							}
						}
					}
				}
			}
		}
	}

	// Remove from recursion stack before returning
	RecursionStack.Remove(Node->NodeGuid);
	return false;
}

void FMaterializeGraphExecutor::TopologicalSort(UMaterializeGraph* Graph, TArray<UMaterializeGraphNode*>& OutOrder)
{
	OutOrder.Empty();

	if (!Graph)
	{
		return;
	}

	TSet<FGuid> Visited;
	TArray<UMaterializeGraphNode*> Stack;

	// Perform DFS from each unvisited node
	for (UEdGraphNode* EdNode : Graph->Nodes)
	{
		if (UMaterializeGraphNode* KNode = Cast<UMaterializeGraphNode>(EdNode))
		{
			if (!Visited.Contains(KNode->NodeGuid))
			{
				TopologicalSortDFS(KNode, Visited, Stack);
			}
		}
	}

	// The stack now contains nodes in reverse topological order
	// Reverse it to get the correct execution order (dependencies first)
	OutOrder = Stack;
	Algo::Reverse(OutOrder);
}

void FMaterializeGraphExecutor::TopologicalSortDFS(UMaterializeGraphNode* Node, TSet<FGuid>& Visited, TArray<UMaterializeGraphNode*>& Stack)
{
	if (!Node)
	{
		return;
	}

	// Mark as visited
	Visited.Add(Node->NodeGuid);

	// Visit all dependencies (nodes connected to input pins)
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (LinkedPin)
				{
					UMaterializeGraphNode* LinkedNode = Cast<UMaterializeGraphNode>(LinkedPin->GetOwningNode());
					if (LinkedNode && !Visited.Contains(LinkedNode->NodeGuid))
					{
						TopologicalSortDFS(LinkedNode, Visited, Stack);
					}
				}
			}
		}
	}

	// Add current node to stack after visiting all dependencies (post-order)
	Stack.Add(Node);
}

UMaterializeGraphNode_Output* FMaterializeGraphExecutor::FindOutputNode(UMaterializeGraph* Graph)
{
	if (!Graph) return nullptr;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UMaterializeGraphNode_Output* OutputNode = Cast<UMaterializeGraphNode_Output>(Node))
		{
			return OutputNode;
		}
	}
	return nullptr;
}

TArray<UMaterializeGraphNode_ChannelOutput*> FMaterializeGraphExecutor::FindChannelOutputNodes(UMaterializeGraph* Graph)
{
	TArray<UMaterializeGraphNode_ChannelOutput*> OutputNodes;
	
	if (!Graph) return OutputNodes;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UMaterializeGraphNode_ChannelOutput* ChannelOutputNode = Cast<UMaterializeGraphNode_ChannelOutput>(Node))
		{
			OutputNodes.Add(ChannelOutputNode);
		}
	}
	
	return OutputNodes;
}

UTexture2D* FMaterializeGraphExecutor::CreateTransientTexture(int32 Width, int32 Height, EPixelFormat Format)
{
	UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, Format);
	if (Tex)
	{
		Tex->SRGB = true;
		Tex->Filter = TF_Bilinear;
		Tex->AddressX = TA_Wrap;
		Tex->AddressY = TA_Wrap;
		Tex->UpdateResource();
		TexturesCreated++;
	}
	return Tex;
}

UTexture2D* FMaterializeGraphExecutor::GetCachedTexture(UMaterializeGraphNode* Node)
{
	if (!Node) return nullptr;
	UTexture2D** Found = TextureCache.Find(Node->NodeGuid);
	return Found ? *Found : nullptr;
}

void FMaterializeGraphExecutor::CacheTexture(UMaterializeGraphNode* Node, UTexture2D* Texture)
{
	if (Node && Texture)
	{
		TextureCache.Add(Node->NodeGuid, Texture);
	}
}

UTexture2D* FMaterializeGraphExecutor::GetInputTexture(UEdGraphPin* Pin, int32 Width, int32 Height, TSet<FGuid>& Visited)
{
	if (!Pin || Pin->LinkedTo.Num() == 0)
	{
		return nullptr;
	}

	UEdGraphPin* SourcePin = Pin->LinkedTo[0];
	if (!SourcePin) return nullptr;

	UMaterializeGraphNode* SourceNode = Cast<UMaterializeGraphNode>(SourcePin->GetOwningNode());
	if (!SourceNode) return nullptr;

	return ExecuteNodeRecursive(SourceNode, Width, Height, Visited);
}

FMaterializeGraphExecutionResult FMaterializeGraphExecutor::Execute(UMaterializeGraph* Graph, int32 Width, int32 Height)
{
	FMaterializeGraphExecutionResult Result;
	double StartTime = FPlatformTime::Seconds();

	Errors.Empty();
	NodesExecuted = 0;
	TexturesCreated = 0;
	ClearCache();

	if (!Graph)
	{
		Errors.Add(TEXT("Graph is null"));
		Result.Errors = Errors;
		return Result;
	}

	// Create execution context for intermediate texture storage
	FMaterializeGraphContext Context;
	Context.TextureResolution = FIntPoint(Width, Height);
	Context.Time = (float)FPlatformTime::Seconds();

	// Compile and validate graph before execution
	TArray<UMaterializeGraphNode*> ExecutionOrder;
	FString CompileError;
	if (!CompileGraph(Graph, ExecutionOrder, CompileError))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: Graph compilation failed: %s"), *CompileError);
		Errors.Add(FString::Printf(TEXT("Graph compilation failed: %s"), *CompileError));
		Result.Errors = Errors;
		return Result;
	}

	UE_LOG(LogTemp, Log, TEXT("Materialize: Graph compiled successfully. Execution order: %d nodes"), ExecutionOrder.Num());
	
	// Log execution order for debugging
	for (int32 i = 0; i < ExecutionOrder.Num(); ++i)
	{
		UE_LOG(LogTemp, Verbose, TEXT("  [%d] %s"), i, *ExecutionOrder[i]->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
	}

	// Execute nodes in topological order (dependencies first)
	// This ensures each node's inputs are ready before execution
	for (UMaterializeGraphNode* Node : ExecutionOrder)
	{
		if (!Node)
		{
			continue;
		}

		// Check if already executed (may be cached from previous execution)
		if (Context.GetCachedTexture(Node->NodeGuid))
		{
			UE_LOG(LogTemp, Verbose, TEXT("Materialize: Node %s already cached, skipping"), 
				*Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			continue;
		}

		// Execute the node using its Execute() method
		UTexture2D* NodeOutput = Node->Execute(Context, Width, Height);
		
		if (NodeOutput)
		{
			// Cache the result for downstream nodes
			Context.CacheTexture(Node->NodeGuid, NodeOutput);
			CacheTexture(Node, NodeOutput);
			NodesExecuted++;
			
			UE_LOG(LogTemp, Verbose, TEXT("Materialize: Executed node %s -> %dx%d texture"), 
				*Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(),
				NodeOutput->GetSizeX(), NodeOutput->GetSizeY());
		}
		else
		{
			// Some nodes (like output nodes) may not produce textures
			UE_LOG(LogTemp, Verbose, TEXT("Materialize: Node %s produced no output texture"), 
				*Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
		}
	}

	// Collect output textures from channel output nodes
	TArray<UMaterializeGraphNode_ChannelOutput*> ChannelOutputNodes = FindChannelOutputNodes(Graph);
	
	if (ChannelOutputNodes.Num() > 0)
	{
		// New architecture: Collect textures from channel output nodes
		for (UMaterializeGraphNode_ChannelOutput* ChannelNode : ChannelOutputNodes)
		{
			// Get the cached texture for this output node
			UTexture2D* ChannelTexture = Context.GetCachedTexture(ChannelNode->NodeGuid);
			
			if (!ChannelTexture)
			{
				// If not cached, try to get from input pin
				for (UEdGraphPin* Pin : ChannelNode->Pins)
				{
					if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
					{
						UEdGraphPin* SourcePin = Pin->LinkedTo[0];
						if (SourcePin)
						{
							UMaterializeGraphNode* SourceNode = Cast<UMaterializeGraphNode>(SourcePin->GetOwningNode());
							if (SourceNode)
							{
								ChannelTexture = Context.GetCachedTexture(SourceNode->NodeGuid);
							}
						}
						break;
					}
				}
			}
			
			if (ChannelTexture)
			{
				// Route to appropriate output based on channel type
				switch (ChannelNode->OutputChannel)
				{
					case EMaterializeOutputChannel::BaseColor:
						Result.BaseColor = ChannelTexture;
						UE_LOG(LogTemp, Log, TEXT("Materialize: BaseColor output: %dx%d"), 
							ChannelTexture->GetSizeX(), ChannelTexture->GetSizeY());
						break;
					case EMaterializeOutputChannel::Normal:
						Result.Normal = ChannelTexture;
						UE_LOG(LogTemp, Log, TEXT("Materialize: Normal output: %dx%d"), 
							ChannelTexture->GetSizeX(), ChannelTexture->GetSizeY());
						break;
					case EMaterializeOutputChannel::Roughness:
						Result.Roughness = ChannelTexture;
						UE_LOG(LogTemp, Log, TEXT("Materialize: Roughness output: %dx%d"), 
							ChannelTexture->GetSizeX(), ChannelTexture->GetSizeY());
						break;
					case EMaterializeOutputChannel::Metallic:
						Result.Metallic = ChannelTexture;
						UE_LOG(LogTemp, Log, TEXT("Materialize: Metallic output: %dx%d"), 
							ChannelTexture->GetSizeX(), ChannelTexture->GetSizeY());
						break;
					case EMaterializeOutputChannel::AO:
						Result.AO = ChannelTexture;
						UE_LOG(LogTemp, Log, TEXT("Materialize: AO output: %dx%d"), 
							ChannelTexture->GetSizeX(), ChannelTexture->GetSizeY());
						break;
					case EMaterializeOutputChannel::Height:
						Result.Height = ChannelTexture;
						UE_LOG(LogTemp, Log, TEXT("Materialize: Height output: %dx%d"), 
							ChannelTexture->GetSizeX(), ChannelTexture->GetSizeY());
						break;
					case EMaterializeOutputChannel::Emissive:
						Result.Emissive = ChannelTexture;
						UE_LOG(LogTemp, Log, TEXT("Materialize: Emissive output: %dx%d"), 
							ChannelTexture->GetSizeX(), ChannelTexture->GetSizeY());
						break;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Materialize: Channel output node for %s has no texture"), 
					*UEnum::GetValueAsString(ChannelNode->OutputChannel));
			}
		}
	}
	else
	{
		// Legacy architecture: Find single output node with multiple pins
		UMaterializeGraphNode_Output* OutputNode = FindOutputNode(Graph);
		if (!OutputNode)
		{
			Errors.Add(TEXT("No output nodes found in graph"));
			Result.Errors = Errors;
			return Result;
		}

		// Collect textures from output node's input pins
		for (UEdGraphPin* Pin : OutputNode->Pins)
		{
			if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
			{
				UEdGraphPin* SourcePin = Pin->LinkedTo[0];
				if (SourcePin)
				{
					UMaterializeGraphNode* SourceNode = Cast<UMaterializeGraphNode>(SourcePin->GetOwningNode());
					if (SourceNode)
					{
						UTexture2D* ChannelTexture = Context.GetCachedTexture(SourceNode->NodeGuid);
						
						// Route to appropriate output based on pin name
						FName PinName = Pin->PinName;
						if (PinName == TEXT("BaseColor"))
						{
							Result.BaseColor = ChannelTexture;
						}
						else if (PinName == TEXT("Normal"))
						{
							Result.Normal = ChannelTexture;
						}
						else if (PinName == TEXT("Roughness"))
						{
							Result.Roughness = ChannelTexture;
						}
						else if (PinName == TEXT("Metallic"))
						{
							Result.Metallic = ChannelTexture;
						}
						else if (PinName == TEXT("Height"))
						{
							Result.Height = ChannelTexture;
						}
						else if (PinName == TEXT("AO"))
						{
							Result.AO = ChannelTexture;
						}
						else if (PinName == TEXT("Emissive"))
						{
							Result.Emissive = ChannelTexture;
						}
					}
				}
			}
		}
	}

	// Copy errors from context
	Errors.Append(Context.Errors);

	Result.ExecutionTimeMs = (float)((FPlatformTime::Seconds() - StartTime) * 1000.0);
	Result.NodesExecuted = NodesExecuted;
	Result.TexturesCreated = TexturesCreated;
	Result.Errors = Errors;

	UE_LOG(LogTemp, Log, TEXT("Materialize: Graph execution complete. Time: %.2fms, Nodes: %d, Textures: %d, Errors: %d"),
		Result.ExecutionTimeMs, Result.NodesExecuted, Result.TexturesCreated, Result.Errors.Num());

	return Result;
}

void FMaterializeGraphExecutor::ExecuteWithPreviews(UMaterializeGraph* Graph, int32 Width, int32 Height)
{
	if (!Graph) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize: ExecuteWithPreviews - Graph is null!"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Materialize: ExecuteWithPreviews starting. Graph has %d nodes"), Graph->Nodes.Num());

	Errors.Empty();
	NodesExecuted = 0;
	TexturesCreated = 0;
	ClearCache();

	// Execute all nodes and populate their previews
	TSet<FGuid> Visited;

	for (UEdGraphNode* EdNode : Graph->Nodes)
	{
		if (UMaterializeGraphNode* KNode = Cast<UMaterializeGraphNode>(EdNode))
		{
			UE_LOG(LogTemp, Log, TEXT("Materialize: Executing node: %s (%s)"), 
				*KNode->GetName(), *KNode->GetClass()->GetName());
			
			// Execute this node (will recursively execute dependencies)
			UTexture2D* Result = ExecuteNodeRecursive(KNode, Width, Height, Visited);
			
			// Set the preview texture on the node
			if (Result)
			{
				UE_LOG(LogTemp, Log, TEXT("Materialize: Node %s produced texture %dx%d"), 
					*KNode->GetName(), Result->GetSizeX(), Result->GetSizeY());
				KNode->SetPreviewTexture(Result);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Materialize: Node %s produced NO texture"), *KNode->GetName());
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Materialize: ExecuteWithPreviews complete. Executed %d nodes, created %d textures"), 
		NodesExecuted, TexturesCreated);

	// Need to flush render commands since GPU textures are async
	FlushRenderingCommands();
}

UTexture2D* FMaterializeGraphExecutor::ExecuteNode(UMaterializeGraphNode* Node, int32 Width, int32 Height)
{
	if (!Node) return nullptr;

	TSet<FGuid> Visited;
	return ExecuteNodeRecursive(Node, Width, Height, Visited);
}

UTexture2D* FMaterializeGraphExecutor::ExecuteNodeRecursive(UMaterializeGraphNode* Node, int32 Width, int32 Height, TSet<FGuid>& Visited)
{
	if (!Node) return nullptr;

	// Check if already executed this frame
	if (Visited.Contains(Node->NodeGuid))
	{
		return GetCachedTexture(Node);
	}
	Visited.Add(Node->NodeGuid);

	// Check cache
	if (UTexture2D* Cached = GetCachedTexture(Node))
	{
		return Cached;
	}

	UTexture2D* Result = nullptr;

	// Dispatch based on node type
	if (UMaterializeGraphNode_Noise* NoiseNode = Cast<UMaterializeGraphNode_Noise>(Node))
	{
		Result = ExecuteNoiseNode(NoiseNode, Width, Height);
	}
	else if (UMaterializeGraphNode_Blend* BlendNode = Cast<UMaterializeGraphNode_Blend>(Node))
	{
		// Get input textures
		UTexture2D* BaseTexture = nullptr;
		UTexture2D* BlendTexture = nullptr;

		for (UEdGraphPin* Pin : BlendNode->Pins)
		{
			if (Pin->Direction == EGPD_Input)
			{
				if (Pin->PinName == TEXT("Base"))
				{
					BaseTexture = GetInputTexture(Pin, Width, Height, Visited);
				}
				else if (Pin->PinName == TEXT("Blend"))
				{
					BlendTexture = GetInputTexture(Pin, Width, Height, Visited);
				}
			}
		}

		Result = ExecuteBlendNode(BlendNode, BaseTexture, BlendTexture, Width, Height);
	}
	else if (UMaterializeGraphNode_Filter* FilterNode = Cast<UMaterializeGraphNode_Filter>(Node))
	{
		// Get input texture
		UTexture2D* InputTexture = nullptr;
		for (UEdGraphPin* Pin : FilterNode->Pins)
		{
			if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
			{
				InputTexture = GetInputTexture(Pin, Width, Height, Visited);
				break;
			}
		}

		Result = ExecuteFilterNode(FilterNode, InputTexture, Width, Height);
	}
	else if (UMaterializeGraphNode_Adjustment* AdjustmentNode = Cast<UMaterializeGraphNode_Adjustment>(Node))
	{
		// Get input texture
		UTexture2D* InputTexture = nullptr;
		for (UEdGraphPin* Pin : AdjustmentNode->Pins)
		{
			if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
			{
				InputTexture = GetInputTexture(Pin, Width, Height, Visited);
				break;
			}
		}

		Result = ExecuteAdjustmentNode(AdjustmentNode, InputTexture, Width, Height);
	}
	else if (UMaterializeGraphNode_Math* MathNode = Cast<UMaterializeGraphNode_Math>(Node))
	{
		// Get input textures A and B
		UTexture2D* TextureA = nullptr;
		UTexture2D* TextureB = nullptr;

		for (UEdGraphPin* Pin : MathNode->Pins)
		{
			if (Pin->Direction == EGPD_Input)
			{
				if (Pin->PinName == TEXT("A"))
				{
					TextureA = GetInputTexture(Pin, Width, Height, Visited);
				}
				else if (Pin->PinName == TEXT("B"))
				{
					TextureB = GetInputTexture(Pin, Width, Height, Visited);
				}
			}
		}

		Result = ExecuteMathNode(MathNode, TextureA, TextureB, Width, Height);
	}
	else if (UMaterializeGraphNode_Output* OutputNode = Cast<UMaterializeGraphNode_Output>(Node))
	{
		// Output node doesn't produce a texture, it collects them
		// For preview purposes, show the first connected input
		for (UEdGraphPin* Pin : OutputNode->Pins)
		{
			if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
			{
				Result = GetInputTexture(Pin, Width, Height, Visited);
				break;
			}
		}
	}
	else if (UMaterializeGraphNode_ChannelOutput* ChannelOutputNode = Cast<UMaterializeGraphNode_ChannelOutput>(Node))
	{
		// Channel output node extracts texture from input
		// Execute using the node's Execute method which handles the logic
		FMaterializeGraphContext Context;
		Context.TextureResolution = FIntPoint(Width, Height);
		Result = ChannelOutputNode->Execute(Context, Width, Height);
	}

	// Cache result
	if (Result)
	{
		CacheTexture(Node, Result);
		NodesExecuted++;
		
		// Update node preview
		Node->SetPreviewTexture(Result);
	}

	return Result;
}

UTexture2D* FMaterializeGraphExecutor::ExecuteNoiseNode(UMaterializeGraphNode_Noise* Node, int32 Width, int32 Height)
{
	if (!Node) return nullptr;

	UE_LOG(LogTemp, Log, TEXT("Materialize: ExecuteNoiseNode - Type: %d, Scale: %f, Octaves: %d"),
		(int32)Node->Params.NoiseType, Node->Params.Scale, Node->Params.Octaves);

	// Use the existing KLayerEvaluator procedural generator
	FString Error;
	UTexture2D* Result = UKLayerEvaluator::GenerateProceduralTexture(Node->Params, Width, Height, Error);
	
	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: ExecuteNoiseNode - GenerateProceduralTexture returned null!"));
	}
	
	return Result;
}

UTexture2D* FMaterializeGraphExecutor::ExecuteBlendNode(UMaterializeGraphNode_Blend* Node, UTexture2D* Base, UTexture2D* Blend, int32 Width, int32 Height)
{
	if (!Node) return nullptr;

	// If we have both inputs, blend them
	if (Base && Blend)
	{
		FString Error;
		return UKLayerEvaluator::BlendTextures(
			Base, 
			Blend, 
			Node->BlendMode, 
			Node->Opacity, 
			nullptr, // No mask for now
			false,   // No invert
			Error
		);
	}
	
	// Pass through whichever input we have
	return Base ? Base : Blend;
}

UTexture2D* FMaterializeGraphExecutor::ExecuteFilterNode(UMaterializeGraphNode_Filter* Node, UTexture2D* Input, int32 Width, int32 Height)
{
	if (!Node || !Input) return Input;

	FString Error;
	return UKLayerEvaluator::ApplyFilter(Input, Node->FilterParams, Error);
}

UTexture2D* FMaterializeGraphExecutor::ExecuteAdjustmentNode(UMaterializeGraphNode_Adjustment* Node, UTexture2D* Input, int32 Width, int32 Height)
{
	if (!Node || !Input) return Input;

	FString Error;
	return UKLayerEvaluator::ApplyAdjustment(Input, Node->AdjustmentParams, Error);
}

UTexture2D* FMaterializeGraphExecutor::ExecuteMathNode(UMaterializeGraphNode_Math* Node, UTexture2D* A, UTexture2D* B, int32 Width, int32 Height)
{
	if (!Node) return nullptr;

	// Math operations are essentially blend modes:
	// Add = Blend Mode Add
	// Multiply = Blend Mode Multiply
	// Lerp = Linear interpolation

	if (!A && !B) return nullptr;
	if (!A) return B;
	if (!B) return A;

	FString Error;

	// Determine blend mode based on node type
	EKLayerBlendMode BlendMode = EKLayerBlendMode::Normal;
	float BlendAlpha = 1.0f;

	if (Node->IsA<UMaterializeGraphNode_Add>())
	{
		BlendMode = EKLayerBlendMode::Add;
	}
	else if (Node->IsA<UMaterializeGraphNode_Multiply>())
	{
		BlendMode = EKLayerBlendMode::Multiply;
	}
	else if (UMaterializeGraphNode_Lerp* LerpNode = Cast<UMaterializeGraphNode_Lerp>(Node))
	{
		BlendMode = EKLayerBlendMode::Normal;
		BlendAlpha = LerpNode->Alpha;
	}

	return UKLayerEvaluator::BlendTextures(A, B, BlendMode, BlendAlpha, nullptr, false, Error);
}
