#pragma once

#include "CoreMinimal.h"
#include "RenderGraphBuilder.h"
#include "RHICommandList.h"

/**
 * RAII wrapper for RDG (Render Dependency Graph) resources.
 * Ensures proper execution and cleanup of RDG passes.
 * 
 * Usage:
 *   FMaterializeRDGScope RDGScope(RHICmdList);
 *   FRDGBuilder& GraphBuilder = RDGScope.GetGraphBuilder();
 *   // ... add RDG passes ...
 *   // Automatic execution on scope exit
 */
class MATERIALIZE_API FMaterializeRDGScope
{
public:
	/**
	 * Constructor - creates RDG builder
	 * @param InRHICmdList RHI command list for rendering operations
	 */
	explicit FMaterializeRDGScope(FRHICommandListImmediate& InRHICmdList)
		: RHICmdList(InRHICmdList)
		, GraphBuilder(InRHICmdList)
		, bExecuted(false)
	{
	}

	/**
	 * Destructor - automatically executes RDG graph if not already executed
	 */
	~FMaterializeRDGScope()
	{
		if (!bExecuted)
		{
			Execute();
		}
	}

	/**
	 * Get the RDG builder for adding passes
	 * @return Reference to the graph builder
	 */
	FRDGBuilder& GetGraphBuilder()
	{
		return GraphBuilder;
	}

	/**
	 * Manually execute the RDG graph (optional - will auto-execute on destruction)
	 */
	void Execute()
	{
		if (!bExecuted)
		{
			GraphBuilder.Execute();
			bExecuted = true;
		}
	}

	// Prevent copying
	FMaterializeRDGScope(const FMaterializeRDGScope&) = delete;
	FMaterializeRDGScope& operator=(const FMaterializeRDGScope&) = delete;

	// Prevent moving (RDG builder contains references)
	FMaterializeRDGScope(FMaterializeRDGScope&&) = delete;
	FMaterializeRDGScope& operator=(FMaterializeRDGScope&&) = delete;

private:
	FRHICommandListImmediate& RHICmdList;
	FRDGBuilder GraphBuilder;
	bool bExecuted;
};
