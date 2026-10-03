#pragma once

#include "CoreMinimal.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/MaterializeGraphRuntime.h"

class UTexture2D;
class UMaterializeGraphNode;

/**
 * Compiled operation ready for GPU execution
 */
struct FMaterializeCompiledOp
{
	FGuid NodeId;
	TFunction<UTexture2D*(const TMap<FGuid, UTexture2D*>& InputTextures, int32 Width, int32 Height)> Execute;
};

/**
 * Compiler that traverses the graph and produces an ordered list of operations
 */
class MATERIALIZE_API FMaterializeGraphCompiler
{
public:
	FMaterializeGraphCompiler() = default;

	/** Compile the graph starting from the output node */
	bool Compile(UMaterializeGraph* Graph);

	/** Execute compiled operations and return final texture */
	UTexture2D* Execute(int32 Width, int32 Height);

	/** Get compilation errors */
	const TArray<FString>& GetErrors() const { return Errors; }

private:
	void TraverseNode(UMaterializeGraphNode* Node, TSet<FGuid>& Visited);
	
	TArray<FMaterializeCompiledOp> CompiledOps;
	TArray<FString> Errors;
	TMap<FGuid, UTexture2D*> TextureCache;
};
