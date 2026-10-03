#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class UTextureRenderTarget2D;
class UMaterializeGraph;
struct FKLayerStack;
struct FKLayer;
enum class EKLayerBlendMode : uint8;
enum class EKFilterType : uint8;

/**
 * Centralized validation utility for Materialize plugin
 * Provides comprehensive input validation for textures, layer stacks, graphs, and parameters
 */
class MATERIALIZE_API FMaterializeValidator
{
public:
	/**
	 * Validate a texture for use in material operations
	 * @param Texture The texture to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the texture is valid
	 */
	static bool ValidateTexture(UTexture2D* Texture, FString& OutError);

	/**
	 * Validate a render target for use in material operations
	 * @param RenderTarget The render target to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the render target is valid
	 */
	static bool ValidateRenderTarget(UTextureRenderTarget2D* RenderTarget, FString& OutError);

	/**
	 * Validate a layer stack for processing
	 * @param LayerStack The layer stack to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the layer stack is valid
	 */
	static bool ValidateLayerStack(const FKLayerStack& LayerStack, FString& OutError);

	/**
	 * Validate a single layer
	 * @param Layer The layer to validate
	 * @param LayerIndex Index of the layer in the stack (for error messages)
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the layer is valid
	 */
	static bool ValidateLayer(const FKLayer& Layer, int32 LayerIndex, FString& OutError);

	/**
	 * Validate a graph for execution
	 * @param Graph The graph to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the graph is valid
	 */
	static bool ValidateGraph(UMaterializeGraph* Graph, FString& OutError);

	/**
	 * Validate a blend mode is supported
	 * @param BlendMode The blend mode to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the blend mode is valid
	 */
	static bool ValidateBlendMode(EKLayerBlendMode BlendMode, FString& OutError);

	/**
	 * Validate a filter type is supported
	 * @param FilterType The filter type to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the filter type is valid
	 */
	static bool ValidateFilterType(EKFilterType FilterType, FString& OutError);

	/**
	 * Validate texture format is compatible with GPU operations
	 * @param Texture The texture to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the texture format is valid
	 */
	static bool ValidateTextureFormat(UTexture2D* Texture, FString& OutError);

	/**
	 * Validate texture dimensions are within acceptable range
	 * @param Width Texture width
	 * @param Height Texture height
	 * @param OutError Descriptive error message if validation fails
	 * @return True if dimensions are valid
	 */
	static bool ValidateDimensions(int32 Width, int32 Height, FString& OutError);

	/**
	 * Validate that two textures have compatible dimensions
	 * @param Texture1 First texture
	 * @param Texture2 Second texture
	 * @param OutError Descriptive error message if validation fails
	 * @return True if dimensions are compatible
	 */
	static bool ValidateCompatibleDimensions(UTexture2D* Texture1, UTexture2D* Texture2, FString& OutError);

	/**
	 * Validate a float parameter is within a specified range
	 * @param Value The value to validate
	 * @param Min Minimum allowed value
	 * @param Max Maximum allowed value
	 * @param ParamName Name of the parameter (for error message)
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the value is within range
	 */
	static bool ValidateRange(float Value, float Min, float Max, const FString& ParamName, FString& OutError);

	/**
	 * Validate an integer parameter is within a specified range
	 * @param Value The value to validate
	 * @param Min Minimum allowed value
	 * @param Max Maximum allowed value
	 * @param ParamName Name of the parameter (for error message)
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the value is within range
	 */
	static bool ValidateRange(int32 Value, int32 Min, int32 Max, const FString& ParamName, FString& OutError);

	/**
	 * Validate opacity value (0.0 to 1.0)
	 * @param Opacity The opacity value to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if opacity is valid
	 */
	static bool ValidateOpacity(float Opacity, FString& OutError);

private:
	// Maximum allowed texture dimension (8K)
	static constexpr int32 MaxTextureDimension = 8192;
	
	// Minimum allowed texture dimension
	static constexpr int32 MinTextureDimension = 1;
};
