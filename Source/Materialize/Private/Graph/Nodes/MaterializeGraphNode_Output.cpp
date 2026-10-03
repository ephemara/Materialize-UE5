#include "Graph/Nodes/MaterializeGraphNode_Output.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/MaterializeGraphExecutor.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Graph/MaterializeGraphRuntime.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/MessageDialog.h"
#include "EditorFramework/AssetImportData.h"

#define LOCTEXT_NAMESPACE "KSampleGraphNode_Output"

void UMaterializeGraphNode_Output::AllocateDefaultPins()
{
	// PBR Output channels
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("BaseColor"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Normal"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Roughness"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Metallic"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Height"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("AO"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Emissive"));
}

FText UMaterializeGraphNode_Output::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Title", "Output");
}

FLinearColor UMaterializeGraphNode_Output::GetNodeTitleColor() const
{
	return FLinearColor(1.0f, 0.3f, 0.3f); // Red
}

UEdGraphPin* UMaterializeGraphNode_Output::GetChannelPin(FName ChannelName) const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->PinName == ChannelName)
		{
			return Pin;
		}
	}
	return nullptr;
}

UTexture2D* UMaterializeGraphNode_Output::Execute(FMaterializeGraphContext& Context, int32 InWidth, int32 InHeight)
{
	// Terminal node — collect each connected channel into the context output map.
	// Returns nullptr (no single output texture; results are read by the executor).
	static const TArray<FName> ChannelNames = {
		TEXT("BaseColor"), TEXT("Normal"), TEXT("Roughness"),
		TEXT("Metallic"), TEXT("Height"), TEXT("AO"), TEXT("Emissive")
	};

	for (const FName& ChannelName : ChannelNames)
	{
		UEdGraphPin* Pin = GetChannelPin(ChannelName);
		if (Pin && Pin->LinkedTo.Num() > 0)
		{
			UTexture2D* Tex = GetInputTexture(Pin, Context, InWidth, InHeight);
			if (Tex)
			{
				Context.OutputTextures.Add(ChannelName, Tex);
			}
		}
	}
	return nullptr;
}

void UMaterializeGraphNode_Output::BakeToTextures()
{
	UMaterializeGraph* Graph = Cast<UMaterializeGraph>(GetGraph());
	if (!Graph)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoGraph", "No graph found!"));
		return;
	}

	// Execute graph at full resolution
	FMaterializeGraphExecutor Executor;
	FMaterializeGraphExecutionResult Result = Executor.Execute(Graph, Width, Height);

	if (!Result.IsValid())
	{
		FString ExecutionErrors = FString::Join(Result.Errors, TEXT("\n"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("ExecutionFailed", "Graph execution failed:\n{0}"), 
			FText::FromString(ExecutionErrors)));
		return;
	}

	// Lambda helper to save a texture
	auto SaveTexture = [this](UTexture2D* SourceTex, const FString& Suffix) -> UTexture2D*
	{
		if (!SourceTex)
		{
			return nullptr;
		}

		// Create the package path
		FString PackagePath = OutputPath / (OutputName + Suffix);
		FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);
		
		// Create or find the package
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackageName);
			return nullptr;
		}

		// Create a new persistent texture
		FString AssetName = OutputName + Suffix;
		UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
		if (!NewTexture)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create texture: %s"), *AssetName);
			return nullptr;
		}

		// Copy texture data from source
		int32 SizeX = SourceTex->GetSizeX();
		int32 SizeY = SourceTex->GetSizeY();
		EPixelFormat Format = SourceTex->GetPixelFormat();

		NewTexture->Source.Init(SizeX, SizeY, 1, 1, TSF_BGRA8);
		
		// Lock source and copy data
		FTexture2DMipMap& Mip = SourceTex->GetPlatformData()->Mips[0];
		void* SrcData = Mip.BulkData.Lock(LOCK_READ_ONLY);
		if (SrcData)
		{
			uint8* DestData = NewTexture->Source.LockMip(0);
			FMemory::Memcpy(DestData, SrcData, Mip.BulkData.GetBulkDataSize());
			NewTexture->Source.UnlockMip(0);
			Mip.BulkData.Unlock();
		}

		NewTexture->SRGB = Suffix == TEXT("_BaseColor") || Suffix == TEXT("_Emissive");
		NewTexture->CompressionSettings = Suffix == TEXT("_Normal") ? TC_Normalmap : TC_Default;
		NewTexture->UpdateResource();
		NewTexture->PostEditChange();

		// Mark package dirty and save
		Package->MarkPackageDirty();
		
		FAssetRegistryModule::AssetCreated(NewTexture);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs);

		UE_LOG(LogTemp, Log, TEXT("Saved texture: %s"), *PackageFileName);
		return NewTexture;
	};

	// Save each channel
	int32 SavedCount = 0;
	
	if (Result.BaseColor)
	{
		BakedBaseColor = SaveTexture(Result.BaseColor, TEXT("_BaseColor"));
		if (BakedBaseColor) SavedCount++;
	}
	if (Result.Normal)
	{
		BakedNormal = SaveTexture(Result.Normal, TEXT("_Normal"));
		if (BakedNormal) SavedCount++;
	}
	if (Result.Roughness)
	{
		BakedRoughness = SaveTexture(Result.Roughness, TEXT("_Roughness"));
		if (BakedRoughness) SavedCount++;
	}
	if (Result.Metallic)
	{
		BakedMetallic = SaveTexture(Result.Metallic, TEXT("_Metallic"));
		if (BakedMetallic) SavedCount++;
	}
	if (Result.Height)
	{
		BakedHeight = SaveTexture(Result.Height, TEXT("_Height"));
		if (BakedHeight) SavedCount++;
	}
	if (Result.AO)
	{
		BakedAO = SaveTexture(Result.AO, TEXT("_AO"));
		if (BakedAO) SavedCount++;
	}
	if (Result.Emissive)
	{
		BakedEmissive = SaveTexture(Result.Emissive, TEXT("_Emissive"));
		if (BakedEmissive) SavedCount++;
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
		LOCTEXT("BakeSuccess", "Successfully baked {0} texture(s) to {1}!"),
		FText::AsNumber(SavedCount),
		FText::FromString(OutputPath)));
}

#undef LOCTEXT_NAMESPACE
