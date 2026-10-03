// Copyright K-Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterializeGraph;
class UMaterializeGraphNode;
class UEdGraphPin;

/**
 * Validation result for graph operations
 * Contains errors, warnings, and information about problematic nodes
 */
struct MATERIALIZE_API FMaterializeGraphValidationResult
{
	// Overall validation success
	bool bIsValid = true;
	
	// List of errors (blocking issues)
	TArray<FString> Errors;
	
	// List of warnings (non-blocking issues)
	TArray<FString> Warnings;
	
	// Nodes involved in cycles
	TArray<UMaterializeGraphNode*> CycleNodes;
	
	// Disconnected nodes (no path to output)
	TArray<UMaterializeGraphNode*> DisconnectedNodes;
	
	// Nodes with invalid parameters
	TMap<UMaterializeGraphNode*, TArray<FString>> InvalidNodeParams;
	
	// Helper to check if there are any issues
	bool HasIssues() const { return !bIsValid || Errors.Num() > 0 || Warnings.Num() > 0; }
	
	// Helper to get summary text
	FString GetSummary() const;
};

/**
 * Graph validator for Materialize node graphs
 * Provides validation for graph structure, connections, and parameters
 */
class MATERIALIZE_API FMaterializeGraphValidator
{
public:
	/**
	 * Validate entire graph structure
	 * Checks for cycles, disconnected nodes, invalid parameters, etc.
	 * 
	 * @param Graph The graph to validate
	 * @param OutResult Validation result with errors and warnings
	 * @return True if graph is valid (no blocking errors)
	 */
	static bool ValidateGraph(UMaterializeGraph* Graph, FMaterializeGraphValidationResult& OutResult);
	
	/**
	 * Detect cycles in the graph using depth-first search
	 * 
	 * @param Graph The graph to check
	 * @param OutCycleNodes Nodes that are part of cycles
	 * @return True if cycles were detected
	 */
	static bool DetectCycles(UMaterializeGraph* Graph, TArray<UMaterializeGraphNode*>& OutCycleNodes);
	
	/**
	 * Check if two pins can be connected
	 * Validates pin types, directions, and compatibility
	 * 
	 * @param SourcePin The source pin (output)
	 * @param TargetPin The target pin (input)
	 * @param OutReason Reason why connection is invalid (if false)
	 * @return True if pins can be connected
	 */
	static bool CanConnectPins(const UEdGraphPin* SourcePin, const UEdGraphPin* TargetPin, FString& OutReason);
	
	/**
	 * Validate node parameters
	 * Checks parameter ranges, required values, etc.
	 * 
	 * @param Node The node to validate
	 * @param OutErrors List of parameter validation errors
	 * @return True if all parameters are valid
	 */
	static bool ValidateNodeParams(UMaterializeGraphNode* Node, TArray<FString>& OutErrors);
	
	/**
	 * Find disconnected nodes (no path to output)
	 * 
	 * @param Graph The graph to check
	 * @return Array of nodes that are disconnected from output
	 */
	static TArray<UMaterializeGraphNode*> FindDisconnectedNodes(UMaterializeGraph* Graph);
	
	/**
	 * Validate a single connection
	 * Checks if the connection would create a cycle or violate other rules
	 * 
	 * @param Graph The graph containing the connection
	 * @param SourcePin The source pin
	 * @param TargetPin The target pin
	 * @param OutReason Reason why connection is invalid (if false)
	 * @return True if connection is valid
	 */
	static bool ValidateConnection(UMaterializeGraph* Graph, const UEdGraphPin* SourcePin, const UEdGraphPin* TargetPin, FString& OutReason);

private:
	/**
	 * Internal cycle detection using DFS with recursion stack
	 * 
	 * @param Node Current node being visited
	 * @param Visited Set of all visited nodes
	 * @param RecursionStack Set of nodes in current DFS path
	 * @param OutCycleNodes Nodes that are part of cycles
	 * @return True if cycle detected
	 */
	static bool DetectCyclesRecursive(
		UMaterializeGraphNode* Node,
		TSet<UMaterializeGraphNode*>& Visited,
		TSet<UMaterializeGraphNode*>& RecursionStack,
		TArray<UMaterializeGraphNode*>& OutCycleNodes
	);
	
	/**
	 * Check if adding a connection would create a cycle
	 * 
	 * @param SourceNode The source node
	 * @param TargetNode The target node
	 * @return True if connection would create a cycle
	 */
	static bool WouldCreateCycle(UMaterializeGraphNode* SourceNode, UMaterializeGraphNode* TargetNode);
	
	/**
	 * Get all nodes reachable from a given node (forward traversal)
	 * 
	 * @param StartNode The starting node
	 * @param OutReachable Set of reachable nodes
	 */
	static void GetReachableNodes(UMaterializeGraphNode* StartNode, TSet<UMaterializeGraphNode*>& OutReachable);
	
	/**
	 * Check if a node is an output node
	 * 
	 * @param Node The node to check
	 * @return True if node is an output node
	 */
	static bool IsOutputNode(UMaterializeGraphNode* Node);
	
	/**
	 * Get all output nodes in the graph
	 * 
	 * @param Graph The graph to search
	 * @return Array of output nodes
	 */
	static TArray<UMaterializeGraphNode*> GetOutputNodes(UMaterializeGraph* Graph);
};
