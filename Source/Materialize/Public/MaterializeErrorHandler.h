#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class UTextureRenderTarget2D;

/**
 * Centralized error handling utility for Materialize layer operations
 * Provides consistent error logging, validation, and user-friendly error messages
 */
class MATERIALIZE_API FMaterializeErrorHandler
{
public:
	/**
	 * Log an error message to the Materialize log category
	 * @param Context The operation context (e.g., "BlendTextures", "ApplyFilter")
	 * @param Message The error message
	 */
	static void LogError(const FString& Context, const FString& Message);

	/**
	 * Log a warning message to the Materialize log category
	 * @param Context The operation context
	 * @param Message The warning message
	 */
	static void LogWarning(const FString& Context, const FString& Message);

	/**
	 * Validate a texture for use in layer operations
	 * @param Texture The texture to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the texture is valid
	 */
	static bool ValidateTexture(UTexture2D* Texture, FString& OutError);

	/**
	 * Validate a render target for use in layer operations
	 * @param RenderTarget The render target to validate
	 * @param OutError Descriptive error message if validation fails
	 * @return True if the render target is valid
	 */
	static bool ValidateRenderTarget(UTextureRenderTarget2D* RenderTarget, FString& OutError);

	/**
	 * Validate texture dimensions
	 * @param Width Texture width
	 * @param Height Texture height
	 * @param OutError Descriptive error message if validation fails
	 * @return True if dimensions are valid
	 */
	static bool ValidateDimensions(int32 Width, int32 Height, FString& OutError);

	/**
	 * Validate a parameter is within a specified range
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
	 * Format an error message with context
	 * @param Context The operation context
	 * @param Message The error message
	 * @return Formatted error string
	 */
	static FString FormatError(const FString& Context, const FString& Message);

	/**
	 * Check if a texture has valid GPU resources
	 * @param Texture The texture to check
	 * @param OutError Descriptive error message if check fails
	 * @return True if GPU resources are available
	 */
	static bool HasValidGPUResource(UTexture2D* Texture, FString& OutError);

	/**
	 * Validate that two textures have compatible dimensions
	 * @param Texture1 First texture
	 * @param Texture2 Second texture
	 * @param OutError Descriptive error message if validation fails
	 * @return True if dimensions are compatible
	 */
	static bool ValidateCompatibleDimensions(UTexture2D* Texture1, UTexture2D* Texture2, FString& OutError);

private:
	// Maximum allowed texture dimension (8K)
	static constexpr int32 MaxTextureDimension = 8192;
};
