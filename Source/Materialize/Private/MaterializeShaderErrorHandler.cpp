#include "MaterializeShaderErrorHandler.h"
#include "MaterializeErrorHandler.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY_STATIC(LogMaterializeShader, Log, All);

bool FMaterializeShaderErrorHandler::ValidateShader(const FString& ShaderName, bool bIsValid, FString& OutError)
{
	if (!bIsValid)
	{
		OutError = FString::Printf(TEXT("Shader '%s' is not valid or failed to compile"), *ShaderName);
		LogShaderError(ShaderName, OutError);
		return false;
	}

	return true;
}

void FMaterializeShaderErrorHandler::LogShaderError(const FString& Context, const FString& ErrorMessage)
{
	UE_LOG(LogMaterializeShader, Error, TEXT("[%s] %s"), *Context, *ErrorMessage);
	
	// Also log to the main error handler for consistency
	FMaterializeErrorHandler::LogError(Context, ErrorMessage);
}

void FMaterializeShaderErrorHandler::ShowShaderErrorNotification(const FString& Context, const FString& TechnicalError)
{
	// Convert technical error to user-friendly message
	FString UserMessage = MakeUserFriendlyShaderError(TechnicalError);

	// Create notification
	FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Shader Error: %s"), *Context)));
	Info.SubText = FText::FromString(UserMessage);
	Info.ExpireDuration = 8.0f;
	Info.bUseLargeFont = false;
	Info.bFireAndForget = true;
	Info.bUseThrobber = false;
	Info.bUseSuccessFailIcons = true;
	
	// Show notification on game thread
	if (IsInGameThread())
	{
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		// Queue notification to game thread
		AsyncTask(ENamedThreads::GameThread, [Info]()
		{
			FSlateNotificationManager::Get().AddNotification(Info);
		});
	}
}

FString FMaterializeShaderErrorHandler::MakeUserFriendlyShaderError(const FString& TechnicalError)
{
	// Get error translations
	static TMap<FString, FString> Translations = GetErrorTranslations();

	// Check for known error patterns
	for (const auto& Translation : Translations)
	{
		if (TechnicalError.Contains(Translation.Key))
		{
			return Translation.Value;
		}
	}

	// Default user-friendly message
	return TEXT("A GPU shader operation failed. This may be due to:\n"
		"- Outdated graphics drivers\n"
		"- Insufficient GPU memory\n"
		"- Incompatible texture formats\n\n"
		"Please check the Output Log for technical details.");
}

bool FMaterializeShaderErrorHandler::ValidateRHIResource(const FString& ResourceName, bool bIsValid, FString& OutError)
{
	if (!bIsValid)
	{
		OutError = FString::Printf(TEXT("RHI resource '%s' is not valid"), *ResourceName);
		LogShaderError(TEXT("ResourceValidation"), OutError);
		return false;
	}

	return true;
}

TMap<FString, FString> FMaterializeShaderErrorHandler::GetErrorTranslations()
{
	TMap<FString, FString> Translations;

	// Shader compilation errors
	Translations.Add(TEXT("failed to compile"), 
		TEXT("Shader compilation failed. Please ensure your graphics drivers are up to date."));
	
	Translations.Add(TEXT("Invalid resource state"), 
		TEXT("GPU resource is in an invalid state. Try restarting the editor."));
	
	Translations.Add(TEXT("out of memory"), 
		TEXT("Insufficient GPU memory. Try using smaller texture resolutions or closing other applications."));
	
	Translations.Add(TEXT("RHI"), 
		TEXT("Graphics hardware interface error. Please update your graphics drivers."));
	
	Translations.Add(TEXT("D3D"), 
		TEXT("DirectX error. Please ensure DirectX is properly installed and your drivers are up to date."));
	
	Translations.Add(TEXT("Vulkan"), 
		TEXT("Vulkan error. Please ensure Vulkan runtime is properly installed."));
	
	Translations.Add(TEXT("texture"), 
		TEXT("Texture operation failed. The texture may have an incompatible format or size."));
	
	Translations.Add(TEXT("dispatch"), 
		TEXT("Shader dispatch failed. This may indicate a driver issue or GPU timeout."));
	
	Translations.Add(TEXT("UAV"), 
		TEXT("GPU write operation failed. The texture may not support write operations."));
	
	Translations.Add(TEXT("SRV"), 
		TEXT("GPU read operation failed. The texture may not be accessible for reading."));

	return Translations;
}
