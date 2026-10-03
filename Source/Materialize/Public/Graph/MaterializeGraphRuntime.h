#pragma once

#include "CoreMinimal.h"
#include "KLayerStack.h" // For EKLayerBlendMode, etc.
#include "MaterializeGraphRuntime.generated.h"

class UTexture2D;

/**
 * Base struct for runtime execution nodes.
 * Unlike UEdGraphNode, this is optimized for the rigorous compute loop.
 */
USTRUCT()
struct MATERIALIZE_API FMaterializeNode
{
	GENERATED_BODY()

	virtual ~FMaterializeNode() = default;

	// Unique ID for this node instance
	FGuid NodeId;

	// Inputs (References to other nodes)
	TArray<FGuid> InputNodes;

	/** Execute this node's logic */
	virtual bool Execute(class FMaterializeGraphContext& Context) { return true; }
};

/**
 * Context passed around during graph execution
 * Stores intermediate textures and execution state
 */
class MATERIALIZE_API FMaterializeGraphContext
{
public:
	/** Intermediate texture storage (node GUID → texture) */
	TMap<FGuid, UTexture2D*> IntermediateTextures;

	/** Final output textures by channel name (e.g. "BaseColor", "Normal", etc.) */
	TMap<FName, UTexture2D*> OutputTextures;

	/** Access to texture pool, randomness, time, etc. */
	float Time = 0.0f;

	/** Execution parameters */
	FIntPoint TextureResolution = FIntPoint(1024, 1024);

	/** Error tracking */
	TArray<FString> Errors;

	/** Get cached texture for a node */
	UTexture2D* GetCachedTexture(const FGuid& NodeGuid) const
	{
		if (const UTexture2D* const* Found = IntermediateTextures.Find(NodeGuid))
		{
			return const_cast<UTexture2D*>(*Found);
		}
		return nullptr;
	}

	/** Cache a texture for a node */
	void CacheTexture(const FGuid& NodeGuid, UTexture2D* Texture)
	{
		IntermediateTextures.Add(NodeGuid, Texture);
	}

	/** Clear all cached textures */
	void ClearCache()
	{
		IntermediateTextures.Empty();
	}
};
