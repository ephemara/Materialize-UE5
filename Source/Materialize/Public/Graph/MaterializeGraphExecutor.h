// Copyright K-Studio. All Rights Reserved.
// Graph Executor - Runs the node graph on GPU via RDG

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"

class UMaterializeGraph;
class UMaterializeGraphNode;
class UMaterializeGraphNode_Output;
class UMaterializeGraphNode_ChannelOutput;

/**
 * Execution result from graph processing
 */
struct MATERIALIZE_API FMaterializeGraphExecutionResult
{
	/** Output textures for each PBR channel */
	UTexture2D* BaseColor = nullptr;
	UTexture2D* Normal = nullptr;
	UTexture2D* Roughness = nullptr;
	UTexture2D* Metallic = nullptr;
	UTexture2D* Height = nullptr;
	UTexture2D* AO = nullptr;
	UTexture2D* Emissive = nullptr;

	/** Execution statistics */
	float ExecutionTimeMs = 0.0f;
	int32 NodesExecuted = 0;
	int32 TexturesCreated = 0;

	/** Errors encountered during execution */
	TArray<FString> Errors;

	bool IsValid() const { return Errors.Num() == 0 && NodesExecuted > 0; }
};

/**
 * Executes a Materialize graph on the GPU using RDG
 * Traverses the graph from Output node backward, caching intermediate textures
 */
class MATERIALIZE_API FMaterializeGraphExecutor
{
public:
	FMaterializeGraphExecutor();
	~FMaterializeGraphExecutor();

	/**
	 * Compile graph into execution order with validation
	 * @param Graph - The graph to compile
	 * @param OutExecutionOrder - Output array of nodes in topologically sorted execution order
	 * @param OutError - Error message if compilation fails
	 * @return True if compilation succeeded, false otherwise
	 */
	bool CompileGraph(UMaterializeGraph* Graph, TArray<UMaterializeGraphNode*>& OutExecutionOrder, FString& OutError);

	/**
	 * Validate graph topology (no cycles, all required outputs connected)
	 * @param Graph - The graph to validate
	 * @param OutError - Error message if validation fails
	 * @return True if graph is valid, false otherwise
	 */
	bool ValidateGraphTopology(UMaterializeGraph* Graph, FString& OutError);

	/**
	 * Execute the entire graph and produce output textures
	 * @param Graph - The graph to execute
	 * @param Width - Output texture width
	 * @param Height - Output texture height
	 * @return Execution result with output textures and statistics
	 */
	FMaterializeGraphExecutionResult Execute(UMaterializeGraph* Graph, int32 Width = 1024, int32 Height = 1024);

	/**
	 * Execute a single node and update its preview texture
	 * Useful for selective re-execution when parameters change
	 */
	UTexture2D* ExecuteNode(UMaterializeGraphNode* Node, int32 Width, int32 Height);

	/**
	 * Execute the graph and update all node preview textures
	 * This is called when the user wants live preview updates
	 */
	void ExecuteWithPreviews(UMaterializeGraph* Graph, int32 Width = 256, int32 Height = 256);

	/** Clear all cached textures */
	void ClearCache();

	/** Get last execution errors */
	const TArray<FString>& GetErrors() const { return Errors; }

private:
	/**
	 * Detect cycles in the graph using depth-first search
	 * @param Graph - The graph to check
	 * @return True if a cycle is detected, false otherwise
	 */
	bool DetectCycles(UMaterializeGraph* Graph);

	/**
	 * Perform topological sort on the graph to determine execution order
	 * @param Graph - The graph to sort
	 * @param OutOrder - Output array of nodes in topologically sorted order
	 */
	void TopologicalSort(UMaterializeGraph* Graph, TArray<UMaterializeGraphNode*>& OutOrder);

	/**
	 * Helper for cycle detection - performs DFS and tracks visited/recursion stack
	 * @param Node - Current node being visited
	 * @param Visited - Set of all visited nodes
	 * @param RecursionStack - Set of nodes in current DFS path
	 * @return True if a cycle is detected
	 */
	bool DetectCyclesDFS(UMaterializeGraphNode* Node, TSet<FGuid>& Visited, TSet<FGuid>& RecursionStack);

	/**
	 * Helper for topological sort - performs DFS and adds nodes to stack in post-order
	 * @param Node - Current node being visited
	 * @param Visited - Set of all visited nodes
	 * @param Stack - Stack for topological ordering (nodes added in post-order)
	 */
	void TopologicalSortDFS(UMaterializeGraphNode* Node, TSet<FGuid>& Visited, TArray<UMaterializeGraphNode*>& Stack);

	/** Find the output node in the graph (legacy single output node) */
	UMaterializeGraphNode_Output* FindOutputNode(UMaterializeGraph* Graph);

	/** Find all channel output nodes in the graph (new multi-output architecture) */
	TArray<UMaterializeGraphNode_ChannelOutput*> FindChannelOutputNodes(UMaterializeGraph* Graph);

	/** Recursively execute a node and its dependencies */
	UTexture2D* ExecuteNodeRecursive(UMaterializeGraphNode* Node, int32 Width, int32 Height, TSet<FGuid>& Visited);

	/** Get cached texture for a node or nullptr */
	UTexture2D* GetCachedTexture(UMaterializeGraphNode* Node);

	/** Cache a texture for a node */
	void CacheTexture(UMaterializeGraphNode* Node, UTexture2D* Texture);

	/** Get the input texture for a pin (follows connection) */
	UTexture2D* GetInputTexture(UEdGraphPin* Pin, int32 Width, int32 Height, TSet<FGuid>& Visited);

	/** Create a transient texture */
	UTexture2D* CreateTransientTexture(int32 Width, int32 Height, EPixelFormat Format = PF_B8G8R8A8);

	/** Node-specific execution methods */
	UTexture2D* ExecuteNoiseNode(class UMaterializeGraphNode_Noise* Node, int32 Width, int32 Height);
	UTexture2D* ExecuteBlendNode(class UMaterializeGraphNode_Blend* Node, UTexture2D* Base, UTexture2D* Blend, int32 Width, int32 Height);
	UTexture2D* ExecuteFilterNode(class UMaterializeGraphNode_Filter* Node, UTexture2D* Input, int32 Width, int32 Height);
	UTexture2D* ExecuteAdjustmentNode(class UMaterializeGraphNode_Adjustment* Node, UTexture2D* Input, int32 Width, int32 Height);
	UTexture2D* ExecuteMathNode(class UMaterializeGraphNode_Math* Node, UTexture2D* A, UTexture2D* B, int32 Width, int32 Height);

	/** Texture cache (NodeGuid -> Texture) */
	TMap<FGuid, UTexture2D*> TextureCache;

	/** Errors from last execution */
	TArray<FString> Errors;

	/** Statistics */
	int32 NodesExecuted = 0;
	int32 TexturesCreated = 0;
};
