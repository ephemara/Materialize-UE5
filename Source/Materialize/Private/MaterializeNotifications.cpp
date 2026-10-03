// Copyright K-Studio. All Rights Reserved.

#include "MaterializeNotifications.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY_STATIC(LogMaterializeNotifications, Log, All);

// ============================================================================
// FMaterializeNotifications
// ============================================================================

void FMaterializeNotifications::ShowSuccess(const FText& Message, float Duration)
{
	ShowNotification(Message, SNotificationItem::CS_Success, Duration, true);
}

void FMaterializeNotifications::ShowWarning(const FText& Message, float Duration)
{
	ShowNotification(Message, SNotificationItem::CS_Pending, Duration, false);
}

void FMaterializeNotifications::ShowError(const FText& Message, float Duration)
{
	ShowNotification(Message, SNotificationItem::CS_Fail, Duration, true);
}

TSharedPtr<SNotificationItem> FMaterializeNotifications::ShowProgress(const FText& Message, float Progress)
{
	FNotificationInfo Info(Message);
	Info.bFireAndForget = false;
	Info.bUseSuccessFailIcons = false;
	Info.bUseLargeFont = false;
	Info.bUseThrobber = true;
	
	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Pending);
	}
	
	return Notification;
}

void FMaterializeNotifications::UpdateProgress(TSharedPtr<SNotificationItem> Notification, const FText& Message, float Progress)
{
	if (Notification.IsValid())
	{
		Notification->SetText(Message);
		// Note: SNotificationItem doesn't have built-in progress bar, but we can update the text
	}
}

void FMaterializeNotifications::DismissNotification(TSharedPtr<SNotificationItem> Notification)
{
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->ExpireAndFadeout();
	}
}

TSharedPtr<SNotificationItem> FMaterializeNotifications::ShowCustom(
	const FText& Message,
	SNotificationItem::ECompletionState Type,
	float Duration,
	bool bUseSuccessFailIcons)
{
	return ShowNotification(Message, Type, Duration, bUseSuccessFailIcons);
}

TSharedPtr<SNotificationItem> FMaterializeNotifications::ShowNotification(
	const FText& Message,
	SNotificationItem::ECompletionState Type,
	float Duration,
	bool bUseSuccessFailIcons)
{
	FNotificationInfo Info(Message);
	Info.bFireAndForget = true;
	Info.bUseSuccessFailIcons = bUseSuccessFailIcons;
	Info.bUseLargeFont = false;
	
	// Set duration (0 = default)
	if (Duration > 0.0f)
	{
		Info.FadeOutDuration = Duration;
	}
	else
	{
		// Default durations based on type
		switch (Type)
		{
		case SNotificationItem::CS_Success:
			Info.FadeOutDuration = 2.0f;
			break;
		case SNotificationItem::CS_Pending:
			Info.FadeOutDuration = 4.0f;
			break;
		case SNotificationItem::CS_Fail:
			Info.FadeOutDuration = 6.0f;
			break;
		default:
			Info.FadeOutDuration = 3.0f;
			break;
		}
	}
	
	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(Type);
	}
	
	return Notification;
}

// ============================================================================
// FMaterializeErrorContext
// ============================================================================

FString FMaterializeErrorContext::GetFormattedMessage() const
{
	FString Formatted = Message;
	
	if (!Details.IsEmpty())
	{
		Formatted += FString::Printf(TEXT("\n\nDetails: %s"), *Details);
	}
	
	if (!SuggestedAction.IsEmpty())
	{
		Formatted += FString::Printf(TEXT("\n\nSuggested Action: %s"), *SuggestedAction);
	}
	
	return Formatted;
}

FText FMaterializeErrorContext::GetUserMessage() const
{
	FString UserMessage = Message;
	
	if (!SuggestedAction.IsEmpty())
	{
		UserMessage += FString::Printf(TEXT(" %s"), *SuggestedAction);
	}
	
	return FText::FromString(UserMessage);
}

void FMaterializeErrorContext::LogError() const
{
	switch (Severity)
	{
	case ESeverity::Info:
		UE_LOG(LogMaterializeNotifications, Log, TEXT("%s"), *GetFormattedMessage());
		break;
	case ESeverity::Warning:
		UE_LOG(LogMaterializeNotifications, Warning, TEXT("%s"), *GetFormattedMessage());
		break;
	case ESeverity::Error:
	case ESeverity::Critical:
		UE_LOG(LogMaterializeNotifications, Error, TEXT("%s"), *GetFormattedMessage());
		break;
	}
}

void FMaterializeErrorContext::ShowNotification() const
{
	LogError(); // Always log
	
	// Show notification based on severity
	switch (Severity)
	{
	case ESeverity::Info:
		FMaterializeNotifications::ShowSuccess(GetUserMessage());
		break;
	case ESeverity::Warning:
		FMaterializeNotifications::ShowWarning(GetUserMessage());
		break;
	case ESeverity::Error:
	case ESeverity::Critical:
		FMaterializeNotifications::ShowError(GetUserMessage());
		break;
	}
}

// ============================================================================
// MaterializeErrors Namespace
// ============================================================================

namespace MaterializeErrors
{
	FMaterializeErrorContext MasterMaterialNotFound(const FString& MaterialPath)
	{
		FMaterializeErrorContext Error;
		Error.Message = FString::Printf(TEXT("Master material not found: %s"), *MaterialPath);
		Error.Details = TEXT("The master material asset could not be loaded from the specified path.");
		Error.SuggestedAction = TEXT("Ensure the Materialize plugin content is enabled in your project settings.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Error;
		return Error;
	}
	
	FMaterializeErrorContext MasterMaterialCompilationFailed(const TArray<FString>& CompilationErrors)
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Master material compilation failed");
		Error.Details = FString::Printf(TEXT("Material has %d compilation error(s):\n"), CompilationErrors.Num());
		for (const FString& CompError : CompilationErrors)
		{
			Error.Details += FString::Printf(TEXT("  - %s\n"), *CompError);
		}
		Error.SuggestedAction = TEXT("Check the material in the Material Editor for specific errors.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Critical;
		return Error;
	}
	
	FMaterializeErrorContext TransientMaterialGenerationFailed(const FString& Reason)
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Failed to generate transient material");
		Error.Details = Reason;
		Error.SuggestedAction = TEXT("Check the output log for more details.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Error;
		return Error;
	}
	
	FMaterializeErrorContext TextureLoadFailed(const FString& TexturePath)
	{
		FMaterializeErrorContext Error;
		Error.Message = FString::Printf(TEXT("Failed to load texture: %s"), *TexturePath);
		Error.Details = TEXT("The texture asset could not be loaded.");
		Error.SuggestedAction = TEXT("Verify the texture exists and is not corrupted.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Error;
		return Error;
	}
	
	FMaterializeErrorContext InvalidTextureFormat(const FString& TexturePath, const FString& ExpectedFormat)
	{
		FMaterializeErrorContext Error;
		Error.Message = FString::Printf(TEXT("Invalid texture format: %s"), *TexturePath);
		Error.Details = FString::Printf(TEXT("Expected format: %s"), *ExpectedFormat);
		Error.SuggestedAction = TEXT("Use a texture with the correct format.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Warning;
		return Error;
	}
	
	FMaterializeErrorContext TextureCompressionInvalid(const FString& TexturePath)
	{
		FMaterializeErrorContext Error;
		Error.Message = FString::Printf(TEXT("Invalid texture compression: %s"), *TexturePath);
		Error.Details = TEXT("The texture compression settings are not compatible with the material parameter.");
		Error.SuggestedAction = TEXT("Adjust texture compression settings in the texture editor.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Warning;
		return Error;
	}
	
	FMaterializeErrorContext GraphContainsCycles(int32 NumCycleNodes)
	{
		FMaterializeErrorContext Error;
		Error.Message = FString::Printf(TEXT("Graph contains cycles (%d node(s) affected)"), NumCycleNodes);
		Error.Details = TEXT("Circular dependencies detected in the node graph.");
		Error.SuggestedAction = TEXT("Remove connections that create cycles.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Error;
		return Error;
	}
	
	FMaterializeErrorContext GraphHasNoOutput()
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Graph has no output node");
		Error.Details = TEXT("The graph must have at least one output node.");
		Error.SuggestedAction = TEXT("Add an output node to the graph.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Error;
		return Error;
	}
	
	FMaterializeErrorContext GraphExecutionFailed(const FString& Reason)
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Graph execution failed");
		Error.Details = Reason;
		Error.SuggestedAction = TEXT("Check the graph for errors and ensure all nodes are properly connected.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Error;
		return Error;
	}
	
	FMaterializeErrorContext InvalidNodeConnection(const FString& Reason)
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Cannot create connection");
		Error.Details = Reason;
		Error.SuggestedAction = TEXT("Check pin types and ensure no cycles are created.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Warning;
		return Error;
	}
	
	FMaterializeErrorContext PreviewViewportFailed(const FString& Reason)
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Preview viewport failed");
		Error.Details = Reason;
		Error.SuggestedAction = TEXT("Preview has been disabled. Check the output log for details.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Warning;
		return Error;
	}
	
	FMaterializeErrorContext MIDCreationFailed()
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Failed to create material instance");
		Error.Details = TEXT("Material Instance Dynamic creation failed.");
		Error.SuggestedAction = TEXT("Ensure the master material is valid and loaded.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Error;
		return Error;
	}
	
	FMaterializeErrorContext PluginContentNotMounted()
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Materialize plugin content not found");
		Error.Details = TEXT("The plugin content folder could not be accessed.");
		Error.SuggestedAction = TEXT("Enable the Materialize plugin content in Project Settings > Plugins.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Critical;
		return Error;
	}
	
	FMaterializeErrorContext DefaultTexturesNotFound()
	{
		FMaterializeErrorContext Error;
		Error.Message = TEXT("Default textures not found");
		Error.Details = TEXT("The plugin's default textures (T_Default_Normal, T_Default_ORM) could not be loaded.");
		Error.SuggestedAction = TEXT("Reinstall the Materialize plugin or use engine default textures.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Warning;
		return Error;
	}
	
	FMaterializeErrorContext OperationCancelled(const FString& OperationName)
	{
		FMaterializeErrorContext Error;
		Error.Message = FString::Printf(TEXT("Operation cancelled: %s"), *OperationName);
		Error.Details = TEXT("The operation was cancelled by the user.");
		Error.SuggestedAction = TEXT("");
		Error.Severity = FMaterializeErrorContext::ESeverity::Info;
		return Error;
	}
	
	FMaterializeErrorContext UnexpectedError(const FString& Context, const FString& Details)
	{
		FMaterializeErrorContext Error;
		Error.Message = FString::Printf(TEXT("Unexpected error in %s"), *Context);
		Error.Details = Details;
		Error.SuggestedAction = TEXT("Please report this issue with the output log.");
		Error.Severity = FMaterializeErrorContext::ESeverity::Critical;
		return Error;
	}
}
