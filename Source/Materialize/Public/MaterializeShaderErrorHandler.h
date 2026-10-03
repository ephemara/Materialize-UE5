#pragma once

#include "CoreMinimal.h"
#include "RHICommandList.h"

/**
 * Utility class for handling shader compilation and execution errors gracefully
 * Provides try-catch wrappers and user-friendly error messages for GPU operations
 */
class MATERIALIZE_API FMaterializeShaderErrorHandler
{
public:
	/**
	 * Execute a render command with error handling
	 * @param CommandName Name of the command for error reporting
	 * @param RenderCommand Lambda function containing the render command
	 * @param OutError Descriptive error message if execution fails
	 * @return True if execution succeeded
	 */
	template<typename TRenderCommand>
	static bool ExecuteRenderCommandSafe(const FString& CommandName, TRenderCommand&& RenderCommand, FString& OutError)
	{
		bool bSuccess = true;
		FString ErrorMessage;

		try
		{
			// Execute the render command
			RenderCommand();
		}
		catch (const std::exception& e)
		{
			bSuccess = false;
			ErrorMessage = FString::Printf(TEXT("Render command '%s' threw exception: %s"), 
				*CommandName, UTF8_TO_TCHAR(e.what()));
		}
		catch (...)
		{
			bSuccess = false;
			ErrorMessage = FString::Printf(TEXT("Render command '%s' threw unknown exception"), *CommandName);
		}

		if (!bSuccess)
		{
			OutError = ErrorMessage;
			LogShaderError(CommandName, ErrorMessage);
		}

		return bSuccess;
	}

	/**
	 * Check if a shader is valid and compiled
	 * @param ShaderName Name of the shader for error reporting
	 * @param bIsValid Whether the shader is valid
	 * @param OutError Descriptive error message if shader is invalid
	 * @return True if shader is valid
	 */
	static bool ValidateShader(const FString& ShaderName, bool bIsValid, FString& OutError);

	/**
	 * Log a shader compilation or execution error
	 * @param Context The operation context (e.g., "GradientPass", "HeightIntegration")
	 * @param ErrorMessage The error message
	 */
	static void LogShaderError(const FString& Context, const FString& ErrorMessage);

	/**
	 * Display a user-friendly error notification for shader failures
	 * @param Context The operation context
	 * @param TechnicalError The technical error message
	 */
	static void ShowShaderErrorNotification(const FString& Context, const FString& TechnicalError);

	/**
	 * Convert a technical shader error into a user-friendly message
	 * @param TechnicalError The technical error message
	 * @return User-friendly error message
	 */
	static FString MakeUserFriendlyShaderError(const FString& TechnicalError);

	/**
	 * Check if RHI resources are valid before shader dispatch
	 * @param ResourceName Name of the resource for error reporting
	 * @param bIsValid Whether the resource is valid
	 * @param OutError Descriptive error message if resource is invalid
	 * @return True if resource is valid
	 */
	static bool ValidateRHIResource(const FString& ResourceName, bool bIsValid, FString& OutError);

	/**
	 * Wrap a shader dispatch with validation and error handling
	 * @param ShaderName Name of the shader being dispatched
	 * @param DispatchFunc Lambda function that performs the shader dispatch
	 * @param OutError Descriptive error message if dispatch fails
	 * @return True if dispatch succeeded
	 */
	template<typename TDispatchFunc>
	static bool DispatchShaderSafe(const FString& ShaderName, TDispatchFunc&& DispatchFunc, FString& OutError)
	{
		try
		{
			// Execute the shader dispatch
			DispatchFunc();
			return true;
		}
		catch (const std::exception& e)
		{
			OutError = FString::Printf(TEXT("Shader '%s' dispatch failed: %s"), 
				*ShaderName, UTF8_TO_TCHAR(e.what()));
			LogShaderError(ShaderName, OutError);
			ShowShaderErrorNotification(ShaderName, OutError);
			return false;
		}
		catch (...)
		{
			OutError = FString::Printf(TEXT("Shader '%s' dispatch failed with unknown error"), *ShaderName);
			LogShaderError(ShaderName, OutError);
			ShowShaderErrorNotification(ShaderName, OutError);
			return false;
		}
	}

private:
	// Common shader error patterns and their user-friendly translations
	static TMap<FString, FString> GetErrorTranslations();
};
