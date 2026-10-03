// Copyright K-Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Notifications/SNotificationList.h"

// Forward declaration for enum
enum class ECompletionState : uint8;

/**
 * Notification utility class for Materialize plugin
 * Provides toast notifications for success, warnings, and errors
 */
class MATERIALIZE_API FMaterializeNotifications
{
public:
	/**
	 * Show a success notification
	 * 
	 * @param Message The success message to display
	 * @param Duration How long to show the notification (seconds, 0 = default)
	 */
	static void ShowSuccess(const FText& Message, float Duration = 0.0f);
	
	/**
	 * Show a warning notification
	 * 
	 * @param Message The warning message to display
	 * @param Duration How long to show the notification (seconds, 0 = default)
	 */
	static void ShowWarning(const FText& Message, float Duration = 0.0f);
	
	/**
	 * Show an error notification
	 * 
	 * @param Message The error message to display
	 * @param Duration How long to show the notification (seconds, 0 = default)
	 */
	static void ShowError(const FText& Message, float Duration = 0.0f);
	
	/**
	 * Show a progress notification
	 * Useful for long-running operations
	 * 
	 * @param Message The progress message to display
	 * @param Progress Progress value (0.0 to 1.0)
	 * @return Handle to the notification (for updating or dismissing)
	 */
	static TSharedPtr<SNotificationItem> ShowProgress(const FText& Message, float Progress = 0.0f);
	
	/**
	 * Update an existing progress notification
	 * 
	 * @param Notification The notification to update
	 * @param Message New message text
	 * @param Progress New progress value (0.0 to 1.0)
	 */
	static void UpdateProgress(TSharedPtr<SNotificationItem> Notification, const FText& Message, float Progress);
	
	/**
	 * Dismiss a notification
	 * 
	 * @param Notification The notification to dismiss
	 */
	static void DismissNotification(TSharedPtr<SNotificationItem> Notification);
	
	/**
	 * Show a notification with custom settings
	 * 
	 * @param Message The message to display
	 * @param Type The notification type (Info, Success, Warning, Error)
	 * @param Duration How long to show the notification (seconds, 0 = default)
	 * @param bUseSuccessFailIcons Whether to use success/fail icons
	 * @return Handle to the notification
	 */
	static TSharedPtr<SNotificationItem> ShowCustom(
		const FText& Message,
		SNotificationItem::ECompletionState Type,
		float Duration = 0.0f,
		bool bUseSuccessFailIcons = true
	);

private:
	/**
	 * Internal helper to show a notification
	 * 
	 * @param Message The message to display
	 * @param Type The notification type
	 * @param Duration How long to show the notification
	 * @param bUseSuccessFailIcons Whether to use success/fail icons
	 * @return Handle to the notification
	 */
	static TSharedPtr<SNotificationItem> ShowNotification(
		const FText& Message,
		SNotificationItem::ECompletionState Type,
		float Duration,
		bool bUseSuccessFailIcons
	);
};

/**
 * Error context helper for Materialize operations
 * Provides structured error information with actionable instructions
 */
struct MATERIALIZE_API FMaterializeErrorContext
{
	// Error message
	FString Message;
	
	// Detailed description
	FString Details;
	
	// Suggested action to fix the error
	FString SuggestedAction;
	
	// Error severity
	enum class ESeverity
	{
		Info,
		Warning,
		Error,
		Critical
	} Severity = ESeverity::Error;
	
	// Constructor
	FMaterializeErrorContext() = default;
	
	FMaterializeErrorContext(const FString& InMessage, ESeverity InSeverity = ESeverity::Error)
		: Message(InMessage), Severity(InSeverity)
	{
	}
	
	// Get formatted error message with details and suggested action
	FString GetFormattedMessage() const;
	
	// Get user-facing message (suitable for notifications)
	FText GetUserMessage() const;
	
	// Log the error to the output log
	void LogError() const;
	
	// Show as notification
	void ShowNotification() const;
};

/**
 * Common error contexts for Materialize operations
 */
namespace MaterializeErrors
{
	// Material loading errors
	MATERIALIZE_API FMaterializeErrorContext MasterMaterialNotFound(const FString& MaterialPath);
	MATERIALIZE_API FMaterializeErrorContext MasterMaterialCompilationFailed(const TArray<FString>& CompilationErrors);
	MATERIALIZE_API FMaterializeErrorContext TransientMaterialGenerationFailed(const FString& Reason);
	
	// Texture errors
	MATERIALIZE_API FMaterializeErrorContext TextureLoadFailed(const FString& TexturePath);
	MATERIALIZE_API FMaterializeErrorContext InvalidTextureFormat(const FString& TexturePath, const FString& ExpectedFormat);
	MATERIALIZE_API FMaterializeErrorContext TextureCompressionInvalid(const FString& TexturePath);
	
	// Graph errors
	MATERIALIZE_API FMaterializeErrorContext GraphContainsCycles(int32 NumCycleNodes);
	MATERIALIZE_API FMaterializeErrorContext GraphHasNoOutput();
	MATERIALIZE_API FMaterializeErrorContext GraphExecutionFailed(const FString& Reason);
	MATERIALIZE_API FMaterializeErrorContext InvalidNodeConnection(const FString& Reason);
	
	// Preview errors
	MATERIALIZE_API FMaterializeErrorContext PreviewViewportFailed(const FString& Reason);
	MATERIALIZE_API FMaterializeErrorContext MIDCreationFailed();
	
	// Plugin content errors
	MATERIALIZE_API FMaterializeErrorContext PluginContentNotMounted();
	MATERIALIZE_API FMaterializeErrorContext DefaultTexturesNotFound();
	
	// General errors
	MATERIALIZE_API FMaterializeErrorContext OperationCancelled(const FString& OperationName);
	MATERIALIZE_API FMaterializeErrorContext UnexpectedError(const FString& Context, const FString& Details);
}
