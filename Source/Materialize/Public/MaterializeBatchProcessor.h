#pragma once

#include "CoreMinimal.h"
#include "MaterializeTypes.h"
#include "Engine/EngineTypes.h"
#include "AssetRegistry/AssetData.h"
#include "MaterializeBatchProcessor.generated.h"

class UTexture2D;

// =============================================================================
// BATCH ITEM STATUS
// =============================================================================

UENUM(BlueprintType)
enum class EKBatchItemStatus : uint8
{
	Pending     UMETA(DisplayName = "Pending"),
	Processing  UMETA(DisplayName = "Processing"),
	Completed   UMETA(DisplayName = "Completed"),
	Failed      UMETA(DisplayName = "Failed"),
	Skipped     UMETA(DisplayName = "Skipped")
};

// =============================================================================
// BATCH EXPORT FORMAT
// =============================================================================

UENUM(BlueprintType)
enum class EKBatchExportFormat : uint8
{
	UAsset      UMETA(DisplayName = "UAsset (Project)"),
	PNG         UMETA(DisplayName = "PNG"),
	TGA         UMETA(DisplayName = "TGA"),
	EXR         UMETA(DisplayName = "EXR (HDR)")
};

// =============================================================================
// BATCH NAMING CONVENTION
// =============================================================================

UENUM(BlueprintType)
enum class EKBatchNaming : uint8
{
	SourceName_Suffix   UMETA(DisplayName = "{SourceName}_{Channel}"),
	Folder_Channel      UMETA(DisplayName = "{Folder}/{SourceName}_{Channel}"),
	PBR_Convention      UMETA(DisplayName = "{SourceName}_BaseColor, _Normal, _ORM"),
	Custom              UMETA(DisplayName = "Custom Pattern")
};

// =============================================================================
// BATCH ITEM
// =============================================================================

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKBatchItem
{
	GENERATED_BODY()

	// --- Input ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TSoftObjectPtr<UTexture2D> SourceTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FString SourcePath;

	// --- Parameters (can override batch defaults) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	bool bUseCustomParams = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (EditCondition = "bUseCustomParams"))
	FMaterializeParams CustomParams;

	// --- Status ---
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	EKBatchItemStatus Status = EKBatchItemStatus::Pending;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float ProcessingTimeMs = 0.0f;

	// --- Output ---
	UPROPERTY(BlueprintReadOnly, Category = "Output")
	FString OutputBasePath;

	UPROPERTY(BlueprintReadOnly, Category = "Output")
	TArray<FString> GeneratedAssetPaths;

	// --- Unique ID ---
	UPROPERTY()
	FGuid Id;

	FKBatchItem()
	{
		// Zero-init for reflection compatibility
		Id = FGuid(); 
	}

	FKBatchItem(UTexture2D* InTexture)
		: SourceTexture(InTexture)
	{
		Id = FGuid::NewGuid();
		if (InTexture)
		{
			SourcePath = InTexture->GetPathName();
		}
	}

	static FKBatchItem CreateNew()
	{
		FKBatchItem Item;
		Item.Id = FGuid::NewGuid();
		return Item;
	}
};

// =============================================================================
// BATCH SETTINGS
// =============================================================================

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKBatchSettings
{
	GENERATED_BODY()

	// --- Default Parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	FMaterializeParams DefaultParams;

	// --- Output Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	EKBatchExportFormat ExportFormat = EKBatchExportFormat::UAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	EKBatchNaming NamingConvention = EKBatchNaming::SourceName_Suffix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (EditCondition = "NamingConvention == EKBatchNaming::Custom"))
	FString CustomNamingPattern = TEXT("{SourceName}_{Channel}");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	FDirectoryPath OutputDirectory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	bool bCreateSubfolders = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	bool bOverwriteExisting = false;

	// --- Generation Options ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateNormal = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateRoughness = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateMetallic = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateAO = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateHeight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateEmissive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateORM = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateMaterial = true;

	// --- Processing Options ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
	bool bContinueOnError = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
	int32 MaxConcurrentItems = 1;  // 1 = sequential for GPU stability

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
	bool bShowNotificationOnComplete = true;
};

// =============================================================================
// BATCH PROGRESS
// =============================================================================

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKBatchProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 TotalItems = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 CompletedItems = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 FailedItems = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 SkippedItems = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 CurrentItemIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	FString CurrentItemName;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float OverallProgress = 0.0f;  // 0.0 - 1.0

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float ElapsedTimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float EstimatedTimeRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	bool bIsProcessing = false;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	bool bIsCancelled = false;

	float GetProgressPercent() const { return TotalItems > 0 ? (float)CompletedItems / (float)TotalItems : 0.0f; }
};

// =============================================================================
// BATCH PROCESSOR (Delegate Declarations)
// =============================================================================

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBatchItemStarted, const FGuid&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBatchItemCompleted, const FGuid&, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBatchProgressUpdated, const FKBatchProgress&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBatchCompleted, const FKBatchProgress&);

// =============================================================================
// BATCH PROCESSOR
// =============================================================================

UCLASS(BlueprintType)
class MATERIALIZE_API UMaterializeBatchProcessor : public UObject
{
	GENERATED_BODY()

public:
	UMaterializeBatchProcessor();

	// --- Queue Management ---
	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void AddItem(UTexture2D* Texture);

	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void AddItems(const TArray<UTexture2D*>& Textures);

	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void AddItemsFromPath(const FString& ContentPath, bool bRecursive = true);

	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void RemoveItem(const FGuid& ItemId);

	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void ClearQueue();

	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void MoveItem(int32 FromIndex, int32 ToIndex);

	UFUNCTION(BlueprintPure, Category = "Materialize|Batch")
	int32 GetQueueCount() const { return Queue.Num(); }

	UFUNCTION(BlueprintPure, Category = "Materialize|Batch")
	TArray<FKBatchItem> GetQueue() const { return Queue; }

	UFUNCTION(BlueprintPure, Category = "Materialize|Batch")
	FKBatchItem GetItem(const FGuid& ItemId) const;

	// --- Processing ---
	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void StartProcessing();

	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void PauseProcessing();

	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void ResumeProcessing();

	UFUNCTION(BlueprintCallable, Category = "Materialize|Batch")
	void CancelProcessing();

	UFUNCTION(BlueprintPure, Category = "Materialize|Batch")
	bool IsProcessing() const { return Progress.bIsProcessing; }

	UFUNCTION(BlueprintPure, Category = "Materialize|Batch")
	FKBatchProgress GetProgress() const { return Progress; }

	// --- Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FKBatchSettings Settings;

	// --- Events ---
	FOnBatchItemStarted OnItemStarted;
	FOnBatchItemCompleted OnItemCompleted;
	FOnBatchProgressUpdated OnProgressUpdated;
	FOnBatchCompleted OnBatchCompleted;

private:
	// --- Queue ---
	UPROPERTY()
	TArray<FKBatchItem> Queue;

	// --- Progress ---
	UPROPERTY()
	FKBatchProgress Progress;

	// --- Internal State ---
	double BatchStartTime = 0.0;
	bool bPaused = false;
	int32 CurrentIndex = 0;

	// --- Internal Methods ---
	void ProcessNextItem();
	bool ProcessSingleItem(FKBatchItem& Item);
	FString GetOutputPath(const FKBatchItem& Item, const FString& Channel) const;
	void SaveTextureToAsset(UTexture2D* Texture, const FString& Path);
	void SaveTextureToDisk(UTexture2D* Texture, const FString& Path, EKBatchExportFormat Format);
	void UpdateProgress();
	void FinishBatch();
};
