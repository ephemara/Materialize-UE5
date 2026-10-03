#pragma once

#include "CoreMinimal.h"
#include "MaterializeTypes.generated.h"

/**
 * Material category for organizing presets
 */
UENUM(BlueprintType)
enum class EMaterializeCategory : uint8
{
	Organic     UMETA(DisplayName = "Organic"),
	Rubber      UMETA(DisplayName = "Rubber/Synth"),
	Ground      UMETA(DisplayName = "Ground/Rock"),
	Fabric      UMETA(DisplayName = "Fabric"),
	Metal       UMETA(DisplayName = "Metal"),
	Plastic     UMETA(DisplayName = "Plastic"),
	Paper       UMETA(DisplayName = "Paper/Card"),
	Custom      UMETA(DisplayName = "Custom")
};

/**
 * Seamless tiling mode
 */
UENUM(BlueprintType)
enum class EKSeamlessMode : uint8
{
	None        UMETA(DisplayName = "None"),
	CrossBlend  UMETA(DisplayName = "Cross Blend"),
	MirrorBlend UMETA(DisplayName = "Mirror Blend"),
	Histogram   UMETA(DisplayName = "Histogram Match")
};

/**
 * PBR generation parameters - matches autopbr DEFAULT_PARAMS
 */
USTRUCT(BlueprintType)
struct MATERIALIZE_API FMaterializeParams
{
	GENERATED_BODY()

	// --- Normal Map ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Normal", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float NormalStrength = 1.0f;

	// --- Roughness ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roughness", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoughnessBase = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roughness", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float RoughnessContrast = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roughness", meta = (ClampMin = "-128", ClampMax = "128"))
	float RoughnessBrightness = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roughness")
	bool bRoughnessInvert = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roughness", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VarianceWeight = 0.5f;

	// --- Metallic ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metallic", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MetallicBase = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metallic", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float MetallicContrast = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metallic", meta = (ClampMin = "-128", ClampMax = "128"))
	float MetallicBias = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metallic", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float MetallicSensitivity = 2.0f;

	// --- Ambient Occlusion ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AO", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AOIntensity = 1.0f;

	// --- Height ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float HeightContrast = 1.0f;

	// --- Weathering ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EdgeWear = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CavityDirt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Dust = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Grunge = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Scratches = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weathering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Noise = 0.0f;

	// --- Special Effects ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BioDetail = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float BioFrequency = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CyberDetail = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float CyberScale = 1.0f;

	// --- Emissive ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EmissiveThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float EmissiveColorBoost = 1.0f;

	// --- Processing ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
	bool bMakeSeamless = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing", meta = (EditCondition = "bMakeSeamless"))
	EKSeamlessMode SeamlessMode = EKSeamlessMode::CrossBlend;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing", meta = (ClampMin = "0.1", ClampMax = "0.5", EditCondition = "bMakeSeamless"))
	float SeamlessBlendWidth = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing", meta = (ClampMin = "0.5", ClampMax = "1.5"))
	float Gamma = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Vignette = 0.0f;

	// --- Output ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	bool bPackORM = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ClampMin = "64", ClampMax = "8192"))
	int32 OutputResolution = 0;  // 0 = match input

	// --- Advanced ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "4", ClampMax = "64"))
	int32 HeightIterations = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
	bool bUseMultiPassHeight = true;

	// --- Advanced Normal/AO ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "1", ClampMax = "6"))
	int32 NormalOctaves = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float NormalSigmaBase = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float NormalAnisotropy = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "1.0", ClampMax = "32.0"))
	float AORadius = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float AOBias = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float AOContrast = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
	bool bAdvancedNormal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
	bool bAdvancedAO = false;

	FMaterializeParams() = default;
};

/**
 * A single material preset
 */
USTRUCT(BlueprintType)
struct MATERIALIZE_API FMaterializePreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMaterializeCategory Category = EMaterializeCategory::Custom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMaterializeParams Params;

	FMaterializePreset() = default;

	FMaterializePreset(FName InId, const FString& InName, EMaterializeCategory InCategory, const FMaterializeParams& InParams)
		: Id(InId)
		, DisplayName(FText::FromString(InName))
		, Category(InCategory)
		, Params(InParams)
	{
	}
};

/**
 * Result of PBR generation
 */
USTRUCT(BlueprintType)
struct MATERIALIZE_API FMaterializeResult
{
	GENERATED_BODY()

	/** BaseColor produced by the layer stack (may be null if no layer targets BaseColor) */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> LayerBaseColor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Normal = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Roughness = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Metallic = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> AO = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Height = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Emissive = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> ORM = nullptr;  // UE5 packed: R=AO, G=Roughness, B=Metallic

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMaterialInstanceDynamic> Material = nullptr;

	UPROPERTY(BlueprintReadOnly)
	float GenerationTimeMs = 0.0f;

	bool IsValid() const { return Normal != nullptr && Roughness != nullptr; }
};

/**
 * Master material preset descriptor
 * Describes a pre-configured master material with specific shading characteristics
 */
USTRUCT(BlueprintType)
struct MATERIALIZE_API FMaterializeMasterPreset
{
	GENERATED_BODY()

	// Unique identifier for the preset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FName PresetId;

	// Display name shown in UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FText DisplayName;

	// Tooltip description
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FText Description;

	// Path to the master material asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FSoftObjectPath MasterMaterialPath;

	// Preview thumbnail texture
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TSoftObjectPtr<UTexture2D> PreviewThumbnail;

	// Default scalar parameter values for this preset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TMap<FName, float> DefaultScalarParams;

	// Default vector parameter values for this preset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TMap<FName, FLinearColor> DefaultVectorParams;

	// Feature flags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	bool bSupportsAnisotropy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	bool bSupportsClearCoat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	bool bSupportsSubsurface = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features")
	bool bSupportsToonShading = false;

	FMaterializeMasterPreset() = default;

	FMaterializeMasterPreset(
		FName InPresetId,
		const FString& InDisplayName,
		const FString& InDescription,
		const FString& InMaterialPath)
		: PresetId(InPresetId)
		, DisplayName(FText::FromString(InDisplayName))
		, Description(FText::FromString(InDescription))
		, MasterMaterialPath(InMaterialPath)
	{
	}
};
