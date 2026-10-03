#pragma once

#include "CoreMinimal.h"
#include "MaterializeTypes.h"
#include "UObject/NoExportTypes.h"
#include "RenderGraphResources.h"
#include "MaterializeComputeEngine.generated.h"

class UTexture2D;

/**
 * High-performance GPU-based PBR generator using RDG
 */
UCLASS()
class MATERIALIZE_API UMaterializeComputeEngine : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Generates PBR maps on the GPU. Faster and more precise than CPU implementation.
	 * @param SourceTexture The input image
	 * @param Params Configuration for generation
	 * @param OutResult Struct to be filled with generated transient textures
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Materialize")
	static bool GeneratePBRMapsGPU(UTexture2D* SourceTexture, const FMaterializeParams& Params, FMaterializeResult& OutResult);

	/**
	 * Reads GPU texture data back to CPU for saving.
	 */
	static bool ReadbackTexture(UTexture2D* Texture, TArray<FColor>& OutPixels);
	static void ReadbackResult(const FMaterializeResult& Result, TMap<FString, TArray<FColor>>& OutMap);

	/**
	 * Make a texture seamlessly tileable
	 * @param SourceTexture The input texture
	 * @param Mode Tiling algorithm to use
	 * @param BlendWidth Edge blend width (0.1-0.5)
	 * @return Seamless texture
	 */
	UFUNCTION(BlueprintCallable, Category = "Materialize")
	static UTexture2D* MakeSeamless(UTexture2D* SourceTexture, EKSeamlessMode Mode, float BlendWidth = 0.25f);

	/**
	 * Pack AO, Roughness, Metallic into UE5 ORM format
	 * @param AO Ambient Occlusion texture
	 * @param Roughness Roughness texture  
	 * @param Metallic Metallic texture
	 * @return Packed ORM texture (R=AO, G=Roughness, B=Metallic)
	 */
	UFUNCTION(BlueprintCallable, Category = "Materialize")
	static UTexture2D* PackORM(UTexture2D* AO, UTexture2D* Roughness, UTexture2D* Metallic);

	/**
	 * Cleanup transient resources from a result structure
	 * @param Result The result structure containing transient textures
	 */
	static void CleanupTransientResources(FMaterializeResult& Result);

	/**
	 * Validate that RHI resources are valid before use
	 * @param TextureRHI The RHI texture to validate
	 * @param OutError Error message if validation fails
	 * @return True if valid, false otherwise
	 */
	static bool ValidateRHIResource(FRHITexture* TextureRHI, FString& OutError);

private:
	// Internal RDG execution
	static void ExecuteGenPass_RenderThread(FRHICommandListImmediate& RHICmdList, UTexture2D* SourceTexture, const FMaterializeParams& Params, FMaterializeResult& OutResult);
};
