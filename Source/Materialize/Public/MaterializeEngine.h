#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MaterializeTypes.h"
#include "MaterializeEngine.generated.h"

/**
 * GPU-accelerated PBR map generation engine.
 * Generates Normal, Roughness, Metallic, AO, Height, and Emissive maps from a single source texture.
 */
UCLASS()
class MATERIALIZE_API UMaterializeEngine : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Generate all PBR maps from a source texture.
	 * 
	 * @param SourceTexture The input texture (photo, scan, etc.)
	 * @param Params The generation parameters
	 * @param OutResult The generated textures and material
	 * @return True if generation succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Materialize")
	static bool GeneratePBRMaps(
		UTexture2D* SourceTexture,
		const FMaterializeParams& Params,
		FMaterializeResult& OutResult
	);

	/**
	 * Generate PBR maps and save them as assets.
	 * 
	 * @param SourceTexture The input texture
	 * @param Params The generation parameters
	 * @param OutputPath Where to save the generated textures (uses source location if empty)
	 * @param BaseName Base name for generated assets (uses source name if empty)
	 * @param OutResult The generated and saved assets
	 * @return True if generation and save succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Materialize")
	static bool GenerateAndSavePBRMaps(
		UTexture2D* SourceTexture,
		const FMaterializeParams& Params,
		const FString& OutputPath,
		const FString& BaseName,
		FMaterializeResult& OutResult
	);

	/**
	 * Generate a normal map from luminance using Sobel filter.
	 */
	static UTexture2D* GenerateNormalMap(
		const TArray<FColor>& SourcePixels,
		int32 Width,
		int32 Height,
		float Strength,
		const FMaterializeParams& Params
	);

	/**
	 * Generate roughness map from luminance.
	 */
	static UTexture2D* GenerateRoughnessMap(
		const TArray<FColor>& SourcePixels,
		const TArray<float>& GrayBuffer,
		const TArray<float>& EdgeMagnitude,
		int32 Width,
		int32 Height,
		const FMaterializeParams& Params
	);

	/**
	 * Generate metallic map from luminance.
	 */
	static UTexture2D* GenerateMetallicMap(
		const TArray<FColor>& SourcePixels,
		const TArray<float>& GrayBuffer,
		const TArray<float>& EdgeMagnitude,
		int32 Width,
		int32 Height,
		const FMaterializeParams& Params
	);

	/**
	 * Generate ambient occlusion map.
	 */
	static UTexture2D* GenerateAOMap(
		const TArray<float>& GrayBuffer,
		int32 Width,
		int32 Height,
		const FMaterializeParams& Params
	);

	/**
	 * Generate height/displacement map.
	 */
	static UTexture2D* GenerateHeightMap(
		const TArray<float>& GrayBuffer,
		int32 Width,
		int32 Height,
		const FMaterializeParams& Params
	);

	/**
	 * Generate emissive map from bright areas.
	 */
	static UTexture2D* GenerateEmissiveMap(
		const TArray<FColor>& SourcePixels,
		const TArray<float>& GrayBuffer,
		int32 Width,
		int32 Height,
		const FMaterializeParams& Params
	);
	
	/**
	 * Pack AO (R), Roughness (G), Metallic (B) into a single texture.
	 */
	static UTexture2D* PackORM(UTexture2D* AO, UTexture2D* Roughness, UTexture2D* Metallic);

private:
	/** Read texture pixels into a color array */
	static bool ReadTexturePixels(UTexture2D* Texture, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight);

	/** Create a grayscale buffer from colors */
	static void CreateGrayscaleBuffer(const TArray<FColor>& Colors, TArray<float>& OutGray);

	/** Compute Sobel edge detection */
	static void ComputeSobelEdges(
		const TArray<float>& GrayBuffer,
		int32 Width,
		int32 Height,
		TArray<float>& OutDx,
		TArray<float>& OutDy,
		TArray<float>& OutMagnitude
	);

	/** Create a UTexture2D from pixel data */
	static UTexture2D* CreateTextureFromPixels(
		const TArray<FColor>& Pixels,
		int32 Width,
		int32 Height,
		const FString& TextureName,
		bool bSRGB = false
	);

	/** Clamp value to 0-255 range */
	static uint8 ClampByte(float Value);

	/** Linear interpolation */
	static float Lerp(float A, float B, float T);

	/** Generate cyber noise pattern */
	static float CyberNoise(int32 X, int32 Y, float Scale);

	/** Generate bio noise pattern */
	static float BioNoise(int32 X, int32 Y, float Frequency);
};
