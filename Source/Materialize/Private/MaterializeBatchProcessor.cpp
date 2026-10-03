#include "MaterializeBatchProcessor.h"
#include "MaterializeComputeEngine.h"
#include "MaterializeEngine.h"
#include "MaterializePresets.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/PlatformTime.h"
#include "Misc/ScopedSlowTask.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/TextureFactory.h"
#include "Misc/PackageName.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "UObject/SavePackage.h"

UMaterializeBatchProcessor::UMaterializeBatchProcessor()
{
	Settings.DefaultParams = FMaterializePresets::GetDefaultParams();
}

// =============================================================================
// QUEUE MANAGEMENT
// =============================================================================

void UMaterializeBatchProcessor::AddItem(UTexture2D* Texture)
{
	if (!Texture) return;
	
	// Check for duplicates
	for (const FKBatchItem& Item : Queue)
	{
		if (Item.SourcePath == Texture->GetPathName()) return;
	}
	
	Queue.Add(FKBatchItem(Texture));
}

void UMaterializeBatchProcessor::AddItems(const TArray<UTexture2D*>& Textures)
{
	for (UTexture2D* Tex : Textures)
	{
		AddItem(Tex);
	}
}

void UMaterializeBatchProcessor::AddItemsFromPath(const FString& ContentPath, bool bRecursive)
{
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	FARFilter Filter;
	Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(*ContentPath);
	Filter.bRecursivePaths = bRecursive;
	
	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssets(Filter, Assets);
	
	for (const FAssetData& Asset : Assets)
	{
		if (UTexture2D* Tex = Cast<UTexture2D>(Asset.GetAsset()))
		{
			AddItem(Tex);
		}
	}
}

void UMaterializeBatchProcessor::RemoveItem(const FGuid& ItemId)
{
	Queue.RemoveAll([&ItemId](const FKBatchItem& Item) { return Item.Id == ItemId; });
}

void UMaterializeBatchProcessor::ClearQueue()
{
	if (Progress.bIsProcessing)
	{
		CancelProcessing();
	}
	Queue.Empty();
}

void UMaterializeBatchProcessor::MoveItem(int32 FromIndex, int32 ToIndex)
{
	if (!Queue.IsValidIndex(FromIndex) || ToIndex < 0 || ToIndex >= Queue.Num()) return;
	
	FKBatchItem Item = Queue[FromIndex];
	Queue.RemoveAt(FromIndex);
	if (ToIndex > FromIndex) ToIndex--;
	Queue.Insert(Item, ToIndex);
}

FKBatchItem UMaterializeBatchProcessor::GetItem(const FGuid& ItemId) const
{
	const FKBatchItem* Found = Queue.FindByPredicate([&ItemId](const FKBatchItem& Item) { return Item.Id == ItemId; });
	return Found ? *Found : FKBatchItem();
}

// =============================================================================
// PROCESSING
// =============================================================================

void UMaterializeBatchProcessor::StartProcessing()
{
	if (Queue.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Batch: Queue is empty"));
		return;
	}
	
	if (Progress.bIsProcessing)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Batch: Already processing"));
		return;
	}
	
	// Initialize progress
	Progress = FKBatchProgress();
	Progress.TotalItems = Queue.Num();
	Progress.bIsProcessing = true;
	BatchStartTime = FPlatformTime::Seconds();
	CurrentIndex = 0;
	bPaused = false;
	
	// Reset all items to pending
	for (FKBatchItem& Item : Queue)
	{
		Item.Status = EKBatchItemStatus::Pending;
		Item.ErrorMessage.Empty();
		Item.GeneratedAssetPaths.Empty();
	}
	
	UE_LOG(LogTemp, Log, TEXT("Materialize Batch: Starting processing of %d items"), Queue.Num());
	
	ProcessNextItem();
}

void UMaterializeBatchProcessor::PauseProcessing()
{
	if (Progress.bIsProcessing)
	{
		bPaused = true;
		UE_LOG(LogTemp, Log, TEXT("Materialize Batch: Paused"));
	}
}

void UMaterializeBatchProcessor::ResumeProcessing()
{
	if (Progress.bIsProcessing && bPaused)
	{
		bPaused = false;
		UE_LOG(LogTemp, Log, TEXT("Materialize Batch: Resumed"));
		ProcessNextItem();
	}
}

void UMaterializeBatchProcessor::CancelProcessing()
{
	if (Progress.bIsProcessing)
	{
		Progress.bIsCancelled = true;
		Progress.bIsProcessing = false;
		
		// Mark remaining items as skipped
		for (int32 i = CurrentIndex; i < Queue.Num(); ++i)
		{
			if (Queue[i].Status == EKBatchItemStatus::Pending)
			{
				Queue[i].Status = EKBatchItemStatus::Skipped;
				Progress.SkippedItems++;
			}
		}
		
		UE_LOG(LogTemp, Log, TEXT("Materialize Batch: Cancelled"));
		FinishBatch();
	}
}

void UMaterializeBatchProcessor::ProcessNextItem()
{
	if (bPaused || Progress.bIsCancelled) return;
	
	// Find next pending item
	while (CurrentIndex < Queue.Num())
	{
		if (Queue[CurrentIndex].Status == EKBatchItemStatus::Pending)
		{
			break;
		}
		CurrentIndex++;
	}
	
	if (CurrentIndex >= Queue.Num())
	{
		FinishBatch();
		return;
	}
	
	FKBatchItem& Item = Queue[CurrentIndex];
	
	// Update progress
	Progress.CurrentItemIndex = CurrentIndex;
	Progress.CurrentItemName = FPaths::GetBaseFilename(Item.SourcePath);
	UpdateProgress();
	
	// Broadcast start event
	OnItemStarted.Broadcast(Item.Id);
	
	// Process the item
	double ItemStartTime = FPlatformTime::Seconds();
	bool bSuccess = ProcessSingleItem(Item);
	Item.ProcessingTimeMs = (FPlatformTime::Seconds() - ItemStartTime) * 1000.0f;
	
	// Update status
	if (bSuccess)
	{
		Item.Status = EKBatchItemStatus::Completed;
		Progress.CompletedItems++;
	}
	else
	{
		Item.Status = EKBatchItemStatus::Failed;
		Progress.FailedItems++;
		
		if (!Settings.bContinueOnError)
		{
			CancelProcessing();
			return;
		}
	}
	
	// Broadcast completion event
	OnItemCompleted.Broadcast(Item.Id, bSuccess);
	UpdateProgress();
	
	// Move to next
	CurrentIndex++;
	
	// Use a slight delay to allow UI updates and prevent GPU stalls
	FTimerHandle TimerHandle;
	GEditor->GetTimerManager()->SetTimer(TimerHandle, [this]()
	{
		ProcessNextItem();
	}, 0.01f, false);
}

bool UMaterializeBatchProcessor::ProcessSingleItem(FKBatchItem& Item)
{
	// Load the texture
	UTexture2D* SourceTexture = Item.SourceTexture.LoadSynchronous();
	if (!SourceTexture)
	{
		Item.ErrorMessage = TEXT("Failed to load source texture");
		return false;
	}
	
	// Get params (custom or default)
	const FMaterializeParams& Params = Item.bUseCustomParams ? Item.CustomParams : Settings.DefaultParams;
	
	// Generate PBR maps
	FMaterializeResult Result;
	if (!UMaterializeComputeEngine::GeneratePBRMapsGPU(SourceTexture, Params, Result))
	{
		Item.ErrorMessage = TEXT("GPU generation failed");
		return false;
	}
	
	// Determine base output path
	FString BaseName = FPaths::GetBaseFilename(Item.SourcePath);
	FString BaseOutputPath;
	
	if (Settings.OutputDirectory.Path.IsEmpty())
	{
		// Same folder as source
		BaseOutputPath = FPaths::GetPath(Item.SourcePath);
	}
	else
	{
		BaseOutputPath = Settings.OutputDirectory.Path;
	}
	
	if (Settings.bCreateSubfolders)
	{
		BaseOutputPath = FPaths::Combine(BaseOutputPath, BaseName);
	}
	
	Item.OutputBasePath = BaseOutputPath;
	
	// Save individual maps based on settings
	auto SaveMap = [&](UTexture2D* Tex, const FString& Channel, bool bShouldSave)
	{
		if (!bShouldSave || !Tex) return;
		
		FString OutputPath = GetOutputPath(Item, Channel);
		
		if (Settings.ExportFormat == EKBatchExportFormat::UAsset)
		{
			SaveTextureToAsset(Tex, OutputPath);
		}
		else
		{
			SaveTextureToDisk(Tex, OutputPath, Settings.ExportFormat);
		}
		
		Item.GeneratedAssetPaths.Add(OutputPath);
	};
	
	SaveMap(Result.Normal, TEXT("Normal"), Settings.bGenerateNormal);
	SaveMap(Result.Roughness, TEXT("Roughness"), Settings.bGenerateRoughness);
	SaveMap(Result.Metallic, TEXT("Metallic"), Settings.bGenerateMetallic);
	SaveMap(Result.AO, TEXT("AO"), Settings.bGenerateAO);
	SaveMap(Result.Height, TEXT("Height"), Settings.bGenerateHeight);
	SaveMap(Result.Emissive, TEXT("Emissive"), Settings.bGenerateEmissive);
	SaveMap(Result.ORM, TEXT("ORM"), Settings.bGenerateORM);
	
	// Generate material instance if requested
	if (Settings.bGenerateMaterial)
	{
		FString MaterialPath = GetOutputPath(Item, TEXT("Material"));
		// Material creation would go here - using existing UMaterializeEngine logic
	}
	
	return true;
}

FString UMaterializeBatchProcessor::GetOutputPath(const FKBatchItem& Item, const FString& Channel) const
{
	FString BaseName = FPaths::GetBaseFilename(Item.SourcePath);
	FString OutputName;
	
	switch (Settings.NamingConvention)
	{
		case EKBatchNaming::SourceName_Suffix:
			OutputName = FString::Printf(TEXT("%s_%s"), *BaseName, *Channel);
			break;
			
		case EKBatchNaming::Folder_Channel:
			OutputName = FString::Printf(TEXT("%s/%s_%s"), *BaseName, *BaseName, *Channel);
			break;
			
		case EKBatchNaming::PBR_Convention:
			if (Channel == TEXT("Normal")) OutputName = FString::Printf(TEXT("%s_N"), *BaseName);
			else if (Channel == TEXT("ORM")) OutputName = FString::Printf(TEXT("%s_ORM"), *BaseName);
			else if (Channel == TEXT("Height")) OutputName = FString::Printf(TEXT("%s_H"), *BaseName);
			else if (Channel == TEXT("Emissive")) OutputName = FString::Printf(TEXT("%s_E"), *BaseName);
			else OutputName = FString::Printf(TEXT("%s_%s"), *BaseName, *Channel);
			break;
			
		case EKBatchNaming::Custom:
			OutputName = Settings.CustomNamingPattern;
			OutputName = OutputName.Replace(TEXT("{SourceName}"), *BaseName);
			OutputName = OutputName.Replace(TEXT("{Channel}"), *Channel);
			break;
	}
	
	return FPaths::Combine(Item.OutputBasePath, OutputName);
}

void UMaterializeBatchProcessor::SaveTextureToAsset(UTexture2D* Texture, const FString& Path)
{
	if (!Texture) return;
	
	int32 Width = Texture->GetSizeX();
	int32 Height = Texture->GetSizeY();

	TArray<FColor> Pixels;
	if (!UMaterializeComputeEngine::ReadbackTexture(Texture, Pixels))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: Failed to readback texture for asset: %s"), *Texture->GetName());
		return;
	}

	FString PackagePath = Path;
	if (!PackagePath.StartsWith(TEXT("/")))
	{
		PackagePath = TEXT("/Game/") + PackagePath;
	}
	
	// Get or create the package
	FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);
	FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package) return;
	
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
	if (!NewTexture) return;

	// Init Platform Data
	NewTexture->SetPlatformData(new FTexturePlatformData());
	NewTexture->GetPlatformData()->SizeX = Width;
	NewTexture->GetPlatformData()->SizeY = Height;
	NewTexture->GetPlatformData()->PixelFormat = PF_B8G8R8A8;

	// Allocate Mips
	int32 NumMips = 1;
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->GetPlatformData()->Mips.Add(Mip);
	Mip->SizeX = Width;
	Mip->SizeY = Height;

	// Copy Data
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	void* Data = Mip->BulkData.Realloc(Pixels.Num() * sizeof(FColor));
	FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	Mip->BulkData.Unlock();

	// Init Source for Editor Persistence (Optional but recommended)
	NewTexture->Source.Init(Width, Height, 1, 1, ETextureSourceFormat::TSF_BGRA8, (const uint8*)Pixels.GetData());
	
	NewTexture->SRGB = Texture->SRGB;
	NewTexture->CompressionSettings = Texture->CompressionSettings;
	// Determine compression settings based on name/type if needed
	if (Path.Contains("Normal")) 
	{
		NewTexture->CompressionSettings = TC_Normalmap;
		NewTexture->SRGB = false;
	}
	else if (Path.Contains("Mask") || Path.Contains("ORM") || Path.Contains("Roughness") || Path.Contains("Metallic") || Path.Contains("AO") || Path.Contains("Height"))
	{
		NewTexture->CompressionSettings = TC_Masks;
		NewTexture->SRGB = false;
	}
	
	NewTexture->UpdateResource();
	
	NewTexture->MarkPackageDirty();
	
	// Register with asset registry
	FAssetRegistryModule::AssetCreated(NewTexture);
	
	// Save the package
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	
	FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(Package, NewTexture, *PackageFilename, SaveArgs);
}

void UMaterializeBatchProcessor::SaveTextureToDisk(UTexture2D* Texture, const FString& Path, EKBatchExportFormat Format)
{
	if (!Texture) return;
	
	int32 Width = Texture->GetSizeX();
	int32 Height = Texture->GetSizeY();
	
	TArray<FColor> Pixels;
	if (!UMaterializeComputeEngine::ReadbackTexture(Texture, Pixels))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: Failed to readback texture for saving: %s"), *Texture->GetName());
		return;
	}
	
	if (Pixels.Num() != Width * Height)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: Readback size mismatch"));
		return;
	}

	TArray<uint8> ImageData;
	FString Extension;
	
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	
	EImageFormat ImageFormat;
	switch (Format)
	{
		case EKBatchExportFormat::PNG:
			ImageFormat = EImageFormat::PNG;
			Extension = TEXT(".png");
			break;
		case EKBatchExportFormat::TGA:
			ImageFormat = EImageFormat::BMP; // TGA not directly supported, use BMP or implement TGA writer
			Extension = TEXT(".bmp");
			break;
		case EKBatchExportFormat::EXR:
			ImageFormat = EImageFormat::EXR;
			Extension = TEXT(".exr");
			break;
		default:
			ImageFormat = EImageFormat::PNG;
			Extension = TEXT(".png");
			break;
	}
	
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (ImageWrapper.IsValid())
	{
		ImageWrapper->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor), Width, Height, ERGBFormat::BGRA, 8);
		ImageData = ImageWrapper->GetCompressed();
	}
	
	// Ensure directory exists
	FString Directory = FPaths::GetPath(Path);
	IFileManager::Get().MakeDirectory(*Directory, true);
	
	// Write file
	FString FullPath = Path + Extension;
	FFileHelper::SaveArrayToFile(ImageData, *FullPath);
}

void UMaterializeBatchProcessor::UpdateProgress()
{
	Progress.ElapsedTimeSeconds = FPlatformTime::Seconds() - BatchStartTime;
	Progress.OverallProgress = Progress.TotalItems > 0 ? (float)(Progress.CompletedItems + Progress.FailedItems + Progress.SkippedItems) / (float)Progress.TotalItems : 0.0f;
	
	// Estimate remaining time
	if (Progress.CompletedItems > 0)
	{
		float AvgTimePerItem = Progress.ElapsedTimeSeconds / (float)Progress.CompletedItems;
		int32 RemainingItems = Progress.TotalItems - Progress.CompletedItems - Progress.FailedItems - Progress.SkippedItems;
		Progress.EstimatedTimeRemaining = AvgTimePerItem * RemainingItems;
	}
	
	OnProgressUpdated.Broadcast(Progress);
}

void UMaterializeBatchProcessor::FinishBatch()
{
	Progress.bIsProcessing = false;
	Progress.ElapsedTimeSeconds = FPlatformTime::Seconds() - BatchStartTime;
	Progress.OverallProgress = 1.0f;
	Progress.EstimatedTimeRemaining = 0.0f;
	
	// Show notification
	if (Settings.bShowNotificationOnComplete)
	{
		FText NotificationText = FText::Format(
			NSLOCTEXT("Materialize", "BatchComplete", "Materialize Batch Complete\n{0} completed, {1} failed, {2} skipped\nTotal time: {3}s"),
			FText::AsNumber(Progress.CompletedItems),
			FText::AsNumber(Progress.FailedItems),
			FText::AsNumber(Progress.SkippedItems),
			FText::AsNumber(FMath::RoundToInt(Progress.ElapsedTimeSeconds))
		);
		
		FNotificationInfo Info(NotificationText);
		Info.bFireAndForget = true;
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = true;
		
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Materialize Batch: Finished - %d completed, %d failed, %d skipped in %.2fs"),
		Progress.CompletedItems, Progress.FailedItems, Progress.SkippedItems, Progress.ElapsedTimeSeconds);
	
	OnBatchCompleted.Broadcast(Progress);
}
