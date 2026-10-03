#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class UTextureRenderTarget2D;
class UMaterial;
class UMaterialInstanceDynamic;
class UMaterialExpression;

/**
 * Safe resource cleanup utilities for Materialize plugin
 * Provides safe cleanup methods that verify resources exist and are valid
 * before attempting to destroy them, preventing crashes during cleanup operations.
 */
class MATERIALIZE_API FMaterializeSafeCleanup
{
public:
	/**
	 * Safely cleanup a texture and set the pointer to null
	 * Verifies the texture is valid before calling ConditionalBeginDestroy
	 * 
	 * @param Texture Reference to texture pointer (will be set to nullptr after cleanup)
	 */
	static void CleanupTexture(UTexture2D*& Texture);

	/**
	 * Safely cleanup a render target and set the pointer to null
	 * Verifies the render target is valid before calling ConditionalBeginDestroy
	 * 
	 * @param RenderTarget Reference to render target pointer (will be set to nullptr after cleanup)
	 */
	static void CleanupRenderTarget(UTextureRenderTarget2D*& RenderTarget);

	/**
	 * Safely cleanup a material and set the pointer to null
	 * Verifies the material is valid before calling ConditionalBeginDestroy
	 * 
	 * @param Material Reference to material pointer (will be set to nullptr after cleanup)
	 */
	static void CleanupMaterial(UMaterial*& Material);

	/**
	 * Safely cleanup a material instance dynamic and set the pointer to null
	 * Verifies the MID is valid before calling ConditionalBeginDestroy
	 * 
	 * @param MID Reference to MID pointer (will be set to nullptr after cleanup)
	 */
	static void CleanupMaterialInstance(UMaterialInstanceDynamic*& MID);

	/**
	 * Safely cleanup a material expression and set the pointer to null
	 * Verifies the expression is valid before calling ConditionalBeginDestroy
	 * 
	 * @param Expression Reference to expression pointer (will be set to nullptr after cleanup)
	 */
	static void CleanupMaterialExpression(UMaterialExpression*& Expression);

	/**
	 * Safely cleanup an array of UObject pointers
	 * Verifies each object is valid before calling ConditionalBeginDestroy
	 * Empties the array after cleanup
	 * 
	 * @param Array Reference to array of UObject pointers (will be emptied after cleanup)
	 */
	template<typename T>
	static void CleanupArray(TArray<T*>& Array)
	{
		for (T* Item : Array)
		{
			if (Item && IsValid(Item))
			{
				Item->ConditionalBeginDestroy();
			}
		}
		Array.Empty();
	}

	/**
	 * Safely cleanup a map of UObject pointers
	 * Verifies each object is valid before calling ConditionalBeginDestroy
	 * Empties the map after cleanup
	 * 
	 * @param Map Reference to map of UObject pointers (will be emptied after cleanup)
	 */
	template<typename KeyType, typename ValueType>
	static void CleanupMap(TMap<KeyType, ValueType*>& Map)
	{
		for (auto& Pair : Map)
		{
			if (Pair.Value && IsValid(Pair.Value))
			{
				Pair.Value->ConditionalBeginDestroy();
			}
		}
		Map.Empty();
	}

	/**
	 * Safely cleanup a generic UObject pointer and set it to null
	 * Verifies the object is valid before calling ConditionalBeginDestroy
	 * 
	 * @param Object Reference to UObject pointer (will be set to nullptr after cleanup)
	 */
	template<typename T>
	static void CleanupObject(T*& Object)
	{
		if (Object && IsValid(Object))
		{
			Object->ConditionalBeginDestroy();
			Object = nullptr;
		}
	}
};
