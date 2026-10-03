#pragma once

#include "CoreMinimal.h"
#include "KLayerStack.h"
#include "KLayerEvaluator.generated.h"

class UTexture2D;
class URenderTarget2D;

// =============================================================================
// EVALUATION RESULT
// =============================================================================

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKLayerEvalResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	TObjectPtr<UTexture2D> BaseColor;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	TObjectPtr<UTexture2D> Normal;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	TObjectPtr<UTexture2D> Roughness;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	TObjectPtr<UTexture2D> Metallic;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	TObjectPtr<UTexture2D> Height;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	TObjectPtr<UTexture2D> AO;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	TObjectPtr<UTexture2D> Emissive;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	float EvaluationTimeMs = 0.0f;

	bool IsValid() const { return BaseColor != nullptr; }
};

// =============================================================================
// LAYER EVALUATOR
// GPU-accelerated stack compositor
// =============================================================================

UCLASS(BlueprintType)
class MATERIALIZE_API UKLayerEvaluator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Evaluate the entire layer stack and composite all channels
	 * @param Stack The layer stack to evaluate
	 * @param OutResult The composited result textures
	 * @param OutError Descriptive error message if evaluation fails
	 * @return True if evaluation succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static bool EvaluateStack(UPARAM(ref) FKLayerStack& Stack, FKLayerEvalResult& OutResult, FString& OutError);

	/**
	 * Evaluate a single layer (does not composite with others)
	 * @param Layer The layer to evaluate
	 * @param Width Output texture width
	 * @param Height Output texture height
	 * @param OutError Descriptive error message if evaluation fails
	 * @return The evaluated texture, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static UTexture2D* EvaluateSingleLayer(const FKLayer& Layer, int32 Width, int32 Height, FString& OutError);

	/**
	 * Blend two textures together using the specified blend mode
	 * @param Base The base/bottom texture
	 * @param Blend The layer to blend on top
	 * @param BlendMode The blend mode to use
	 * @param Opacity The opacity of the blend layer (0-1)
	 * @param Mask Optional mask texture (white = full blend, black = base only)
	 * @param bInvertMask If true, inverts the mask
	 * @param OutError Descriptive error message if blending fails
	 * @return The blended result, or Base texture on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static UTexture2D* BlendTextures(UTexture2D* Base, UTexture2D* Blend, 
		EKLayerBlendMode BlendMode, float Opacity,
		UTexture2D* Mask, bool bInvertMask, FString& OutError);

	/**
	 * Generate a procedural texture
	 * @param Params Procedural generation parameters
	 * @param Width Output texture width
	 * @param Height Output texture height
	 * @param OutError Descriptive error message if generation fails
	 * @return The generated texture, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static UTexture2D* GenerateProceduralTexture(const FKProceduralParams& Params, int32 Width, int32 Height, FString& OutError);

	/**
	 * Apply a filter to a texture
	 * @param Source The source texture
	 * @param Params Filter parameters
	 * @param OutError Descriptive error message if filter fails
	 * @return The filtered texture, or Source texture on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static UTexture2D* ApplyFilter(UTexture2D* Source, const FKFilterParams& Params, FString& OutError);

	/**
	 * Apply an adjustment to a texture
	 * @param Source The source texture
	 * @param Params Adjustment parameters
	 * @param OutError Descriptive error message if adjustment fails
	 * @return The adjusted texture, or Source texture on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static UTexture2D* ApplyAdjustment(UTexture2D* Source, const FKAdjustmentParams& Params, FString& OutError);

	/**
	 * Add two textures together (A + B)
	 * @param TextureA First input texture
	 * @param TextureB Second input texture
	 * @param OutError Descriptive error message if operation fails
	 * @return The result texture, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static UTexture2D* AddTextures(UTexture2D* TextureA, UTexture2D* TextureB, FString& OutError);

	/**
	 * Multiply two textures together (A * B)
	 * @param TextureA First input texture
	 * @param TextureB Second input texture
	 * @param OutError Descriptive error message if operation fails
	 * @return The result texture, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static UTexture2D* MultiplyTextures(UTexture2D* TextureA, UTexture2D* TextureB, FString& OutError);

	/**
	 * Linear interpolation between two textures (Lerp(A, B, Alpha))
	 * @param TextureA First input texture (Alpha = 0)
	 * @param TextureB Second input texture (Alpha = 1)
	 * @param Alpha Interpolation factor (0-1)
	 * @param OutError Descriptive error message if operation fails
	 * @return The result texture, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "K-Studio|Layers")
	static UTexture2D* LerpTextures(UTexture2D* TextureA, UTexture2D* TextureB, float Alpha, FString& OutError);

	// --- Validation Methods ---

	/**
	 * Validate a layer stack before evaluation
	 * @param Stack The layer stack to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the stack is valid for evaluation
	 */
	static bool ValidateLayerStack(const FKLayerStack& Stack, FString& OutError);

	/**
	 * Validate a blend mode enum value
	 * @param BlendMode The blend mode to validate
	 * @return True if the blend mode is valid
	 */
	static bool ValidateBlendMode(EKLayerBlendMode BlendMode);

	/**
	 * Validate a filter type enum value
	 * @param FilterType The filter type to validate
	 * @return True if the filter type is valid
	 */
	static bool ValidateFilterType(EKFilterType FilterType);

	/**
	 * Validate a texture for use in layer operations
	 * @param Texture The texture to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the texture is valid
	 */
	static bool ValidateTextureFormat(UTexture2D* Texture, FString& OutError);

private:
	// Internal helpers
	static void EvaluateStackInternal_RenderThread(FKLayerStack& Stack, FKLayerEvalResult& OutResult);
	static UTexture2D* CreateTransientTexture(int32 Width, int32 Height, EPixelFormat Format = PF_B8G8R8A8, bool bSRGB = true);
	
	/**
	 * Synchronize layer parameters from CPU to GPU uniform buffers
	 * This ensures that shader parameters match the CPU-side layer data before dispatch
	 */
	template<typename TShaderParameters>
	static void SyncLayerParametersToGPU(TShaderParameters* Params, const FKLayer& Layer);
	
	/**
	 * Validate that GPU parameters match expected CPU values
	 * Used for debugging parameter synchronization issues
	 */
	template<typename TShaderParameters>
	static bool ValidateGPUParameters(const TShaderParameters* Params, const FKLayer& Layer, FString& OutError);
};
