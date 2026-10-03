#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KLayerStack.generated.h"

class UTexture2D;

// =============================================================================
// BLEND MODES
// =============================================================================

UENUM(BlueprintType)
enum class EKLayerBlendMode : uint8
{
	Normal      UMETA(DisplayName = "Normal"),
	Multiply    UMETA(DisplayName = "Multiply"),
	Screen      UMETA(DisplayName = "Screen"),
	Overlay     UMETA(DisplayName = "Overlay"),
	SoftLight   UMETA(DisplayName = "Soft Light"),
	HardLight   UMETA(DisplayName = "Hard Light"),
	Add         UMETA(DisplayName = "Add"),
	Subtract    UMETA(DisplayName = "Subtract"),
	Difference  UMETA(DisplayName = "Difference"),
	Exclusion   UMETA(DisplayName = "Exclusion"),
	Darken      UMETA(DisplayName = "Darken"),
	Lighten     UMETA(DisplayName = "Lighten"),
	ColorDodge  UMETA(DisplayName = "Color Dodge"),
	ColorBurn   UMETA(DisplayName = "Color Burn"),
	LinearDodge UMETA(DisplayName = "Linear Dodge"),
	LinearBurn  UMETA(DisplayName = "Linear Burn"),
	VividLight  UMETA(DisplayName = "Vivid Light"),
	LinearLight UMETA(DisplayName = "Linear Light"),
	PinLight    UMETA(DisplayName = "Pin Light"),
	HardMix     UMETA(DisplayName = "Hard Mix")
};

// =============================================================================
// LAYER TYPES
// =============================================================================

UENUM(BlueprintType)
enum class EKLayerType : uint8
{
	Base        UMETA(DisplayName = "Base Pass"),   // Foundation layer linked to Parameters
	Image       UMETA(DisplayName = "Image"),        // Static texture input
	Procedural  UMETA(DisplayName = "Procedural"),   // Generated noise/patterns
	Fill        UMETA(DisplayName = "Fill"),         // Solid color/value
	Adjustment  UMETA(DisplayName = "Adjustment"),   // HSV, Levels, Curves
	Filter      UMETA(DisplayName = "Filter"),       // Blur, Sharpen, Edge
	Generator   UMETA(DisplayName = "Generator"),    // Ambient Occlusion, Curvature, Position
	Folder      UMETA(DisplayName = "Folder")        // Group/Folder container
};

// =============================================================================
// OUTPUT CHANNEL FLAGS
// =============================================================================

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EKLayerOutputChannel : uint8
{
	None        = 0         UMETA(Hidden),
	BaseColor   = 1 << 0    UMETA(DisplayName = "Base Color"),
	Normal      = 1 << 1    UMETA(DisplayName = "Normal"),
	Roughness   = 1 << 2    UMETA(DisplayName = "Roughness"),
	Metallic    = 1 << 3    UMETA(DisplayName = "Metallic"),
	Height      = 1 << 4    UMETA(DisplayName = "Height"),
	AO          = 1 << 5    UMETA(DisplayName = "AO"),
	Emissive    = 1 << 6    UMETA(DisplayName = "Emissive"),
	Mask        = 1 << 7    UMETA(DisplayName = "Mask"),
	All         = 0xFF      UMETA(DisplayName = "All Channels")
};
ENUM_CLASS_FLAGS(EKLayerOutputChannel);

// =============================================================================
// PROCEDURAL NOISE TYPES
// =============================================================================

UENUM(BlueprintType)
enum class EKProceduralNoiseType : uint8
{
	Perlin      UMETA(DisplayName = "Perlin"),
	Simplex     UMETA(DisplayName = "Simplex"),
	Worley      UMETA(DisplayName = "Worley/Voronoi"),
	FBM         UMETA(DisplayName = "FBM"),
	Turbulence  UMETA(DisplayName = "Turbulence"),
	Cellular    UMETA(DisplayName = "Cellular"),
	Gradient    UMETA(DisplayName = "Gradient"),
	Checker     UMETA(DisplayName = "Checker"),
	Brick       UMETA(DisplayName = "Brick"),
	Herringbone UMETA(DisplayName = "Herringbone"),
	Hexagon     UMETA(DisplayName = "Hexagon"),
	Scratches   UMETA(DisplayName = "Scratches"),
	Grunge      UMETA(DisplayName = "Grunge"),
	Rust        UMETA(DisplayName = "Rust"),
	Dust        UMETA(DisplayName = "Dust")
};

// =============================================================================
// FILTER TYPES
// =============================================================================

UENUM(BlueprintType)
enum class EKFilterType : uint8
{
	Blur        UMETA(DisplayName = "Blur"),
	GaussianBlur UMETA(DisplayName = "Gaussian Blur"),
	Sharpen     UMETA(DisplayName = "Sharpen"),
	EdgeDetect  UMETA(DisplayName = "Edge Detect"),
	Emboss      UMETA(DisplayName = "Emboss"),
	HighPass    UMETA(DisplayName = "High Pass"),
	LowPass     UMETA(DisplayName = "Low Pass"),
	Median      UMETA(DisplayName = "Median"),
	Dilate      UMETA(DisplayName = "Dilate"),
	Erode       UMETA(DisplayName = "Erode"),
	Invert      UMETA(DisplayName = "Invert"),
	Normalize   UMETA(DisplayName = "Normalize"),
	AutoLevels  UMETA(DisplayName = "Auto Levels")
};

// =============================================================================
// ADJUSTMENT TYPES
// =============================================================================

UENUM(BlueprintType)
enum class EKAdjustmentType : uint8
{
	Levels      UMETA(DisplayName = "Levels"),
	Curves      UMETA(DisplayName = "Curves"),
	HSV         UMETA(DisplayName = "HSV"),
	Brightness  UMETA(DisplayName = "Brightness/Contrast"),
	ColorBalance UMETA(DisplayName = "Color Balance"),
	Vibrance    UMETA(DisplayName = "Vibrance"),
	Threshold   UMETA(DisplayName = "Threshold"),
	Posterize   UMETA(DisplayName = "Posterize"),
	Gradient    UMETA(DisplayName = "Gradient Map")
};

// =============================================================================
// GENERATOR TYPES (computed from mesh data)
// =============================================================================

UENUM(BlueprintType)
enum class EKGeneratorType : uint8
{
	AmbientOcclusion UMETA(DisplayName = "Ambient Occlusion"),
	Curvature        UMETA(DisplayName = "Curvature"),
	Position         UMETA(DisplayName = "Position"),
	WorldNormal      UMETA(DisplayName = "World Normal"),
	Thickness        UMETA(DisplayName = "Thickness"),
	EdgeWear         UMETA(DisplayName = "Edge Wear"),
	Dirt             UMETA(DisplayName = "Dirt Accumulation"),
	LightMap         UMETA(DisplayName = "Light Map")
};

// =============================================================================
// PROCEDURAL LAYER PARAMS
// =============================================================================

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKProceduralParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural")
	EKProceduralNoiseType NoiseType = EKProceduralNoiseType::Perlin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural", meta = (ClampMin = "0.01", ClampMax = "100.0"))
	float Scale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural", meta = (ClampMin = "1", ClampMax = "16"))
	int32 Octaves = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Persistence = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float Lacunarity = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural")
	FVector2D Offset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural")
	bool bSeamless = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural")
	float Time = 0.0f;
};

// =============================================================================
// FILTER LAYER PARAMS
// =============================================================================

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKFilterParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	EKFilterType FilterType = EKFilterType::Blur;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter", meta = (ClampMin = "1", ClampMax = "32"))
	int32 KernelSize = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float Threshold = 0.0f;
};

// =============================================================================
// ADJUSTMENT LAYER PARAMS
// =============================================================================

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKAdjustmentParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustment")
	EKAdjustmentType AdjustmentType = EKAdjustmentType::Levels;

	// Levels
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InputBlack = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InputWhite = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels", meta = (ClampMin = "0.1", ClampMax = "9.9"))
	float Gamma = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OutputBlack = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OutputWhite = 1.0f;

	// HSV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HSV", meta = (ClampMin = "-180.0", ClampMax = "180.0"))
	float HueShift = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HSV", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float SaturationAdjust = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HSV", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float ValueAdjust = 0.0f;

	// Brightness/Contrast
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrightnessContrast", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Brightness = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BrightnessContrast", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Contrast = 0.0f;
};

// =============================================================================
// BASE LAYER
// =============================================================================

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKLayer
{
	GENERATED_BODY()

	// --- Identity ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	FGuid Id;

	// --- Type ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	EKLayerType LayerType = EKLayerType::Image;

	// --- Blending ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending")
	EKLayerBlendMode BlendMode = EKLayerBlendMode::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 1.0f;

	// --- Output Channels ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (Bitmask, BitmaskEnum = "/Script/Materialize.EKLayerOutputChannel"))
	int32 OutputChannels = static_cast<int32>(EKLayerOutputChannel::All);

	// --- Visibility ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layer")
	bool bSolo = false;

	// --- Mask ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	bool bHasMask = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	TObjectPtr<UTexture2D> MaskTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	bool bInvertMask = false;

	// --- Image Layer Data ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Image", meta = (EditCondition = "LayerType == EKLayerType::Image", EditConditionHides))
	TObjectPtr<UTexture2D> ImageTexture;

	// --- Fill Layer Data ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fill", meta = (EditCondition = "LayerType == EKLayerType::Fill", EditConditionHides))
	FLinearColor FillColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fill", meta = (EditCondition = "LayerType == EKLayerType::Fill", EditConditionHides))
	float FillValue = 1.0f;

	// --- Procedural Layer Data ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural", meta = (EditCondition = "LayerType == EKLayerType::Procedural", EditConditionHides))
	FKProceduralParams ProceduralParams;

	// --- Filter Layer Data ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter", meta = (EditCondition = "LayerType == EKLayerType::Filter", EditConditionHides))
	FKFilterParams FilterParams;

	// --- Adjustment Layer Data ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustment", meta = (EditCondition = "LayerType == EKLayerType::Adjustment", EditConditionHides))
	FKAdjustmentParams AdjustmentParams;

	// --- Source binding for Filter/Adjustment ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source", meta = (EditCondition = "LayerType == EKLayerType::Filter || LayerType == EKLayerType::Adjustment", EditConditionHides))
	int32 SourceLayerIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source", meta = (EditCondition = "LayerType == EKLayerType::Filter || LayerType == EKLayerType::Adjustment", EditConditionHides))
	TObjectPtr<UTexture2D> SourceOverride;

	// --- Generator Layer Data ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generator", meta = (EditCondition = "LayerType == EKLayerType::Generator", EditConditionHides))
	EKGeneratorType GeneratorType = EKGeneratorType::AmbientOcclusion;

	// --- Folder/Group ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Folder", meta = (EditCondition = "LayerType == EKLayerType::Folder", EditConditionHides))
	bool bFolderExpanded = true;

	UPROPERTY(BlueprintReadOnly, Category = "Folder")
	int32 ParentIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bDirty = true;

	// --- Cached Output (transient, not saved) ---
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedOutput;

	FKLayer()
	{
		Id = FGuid(); // Zero-init to satisfy struct property check
	}

	FKLayer(FName InName, EKLayerType InType)
		: Name(InName)
		, LayerType(InType)
	{
		Id = FGuid::NewGuid();
	}
};

// =============================================================================
// LAYER STACK
// =============================================================================

DECLARE_MULTICAST_DELEGATE(FOnKLayerStackChanged);

/** Serialization version for FKLayerStack. Increment when the layout changes. */
namespace EKLayerStackVersion
{
	enum Type : int32
	{
		Initial        = 0,  // Original layout
		AddedSoloFlag  = 1,  // bSolo added to FKLayer
		AddedLockFlag  = 2,  // bLocked added to FKLayer
		AddedDirtyFlag = 3,  // bDirty / CachedOutput added

		// Always keep this last
		LatestPlusOne,
		Latest = LatestPlusOne - 1
	};
}

USTRUCT(BlueprintType)
struct MATERIALIZE_API FKLayerStack
{
	GENERATED_BODY()

	// --- Serialization version (written on save, checked on load) ---
	UPROPERTY()
	int32 Version = EKLayerStackVersion::Latest;

	// --- Layers (bottom to top) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stack")
	TArray<FKLayer> Layers;

	// --- Stack Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stack")
	int32 Width = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stack")
	int32 Height = 1024;

	// --- Selection ---
	UPROPERTY(BlueprintReadWrite, Category = "Stack")
	int32 SelectedLayerIndex = INDEX_NONE;

	// --- Methods ---

	/**
	 * Migrate serialized data from an older version to the current layout.
	 * Call this after loading a FKLayerStack from disk (e.g. in UObject::PostLoad).
	 * Returns true if any migration was performed.
	 */
	bool MigrateFromOldVersion()
	{
		if (Version >= EKLayerStackVersion::Latest)
		{
			return false;
		}

		if (Version < EKLayerStackVersion::AddedSoloFlag)
		{
			// bSolo was not serialized in old data - default is already false, nothing to do.
		}

		if (Version < EKLayerStackVersion::AddedLockFlag)
		{
			// bLocked was not serialized in old data - default is already false, nothing to do.
		}

		if (Version < EKLayerStackVersion::AddedDirtyFlag)
		{
			// Old stacks have no dirty tracking - mark everything dirty so they re-evaluate.
			MarkAllDirty();
		}

		Version = EKLayerStackVersion::Latest;
		return true;
	}

	// Add a new layer at the top of the stack
	int32 AddLayer(const FKLayer& Layer)
	{
		Layers.Add(Layer);
		return Layers.Num() - 1;
	}

	// Add a new layer at a specific index
	int32 InsertLayer(int32 Index, const FKLayer& Layer)
	{
		if (Index < 0 || Index > Layers.Num()) Index = Layers.Num();
		Layers.Insert(Layer, Index);
		return Index;
	}

	// Remove a layer by index
	bool RemoveLayer(int32 Index)
	{
		if (!Layers.IsValidIndex(Index)) return false;
		Layers.RemoveAt(Index);
		return true;
	}

	// Move a layer
	bool MoveLayer(int32 FromIndex, int32 ToIndex)
	{
		if (!Layers.IsValidIndex(FromIndex) || ToIndex < 0 || ToIndex >= Layers.Num()) return false;
		FKLayer Layer = Layers[FromIndex];
		Layers.RemoveAt(FromIndex);
		if (ToIndex > FromIndex) ToIndex--;
		Layers.Insert(Layer, ToIndex);
		return true;
	}

	// Duplicate a layer
	int32 DuplicateLayer(int32 Index)
	{
		if (!Layers.IsValidIndex(Index)) return INDEX_NONE;
		FKLayer NewLayer = Layers[Index];
		NewLayer.Id = FGuid::NewGuid();
		NewLayer.Name = FName(*FString::Printf(TEXT("%s_Copy"), *Layers[Index].Name.ToString()));
		return AddLayer(NewLayer);
	}

	// Mark a layer as dirty (needs re-evaluation)
	void MarkDirty(int32 Index)
	{
		if (Layers.IsValidIndex(Index))
		{
			Layers[Index].bDirty = true;
			// Mark all layers above as dirty too (they depend on this one)
			for (int32 i = Index + 1; i < Layers.Num(); ++i)
			{
				Layers[i].bDirty = true;
			}
		}
	}

	// Mark all layers as dirty
	void MarkAllDirty()
	{
		for (FKLayer& Layer : Layers)
		{
			Layer.bDirty = true;
		}
	}

	// Clear dirty flags after a successful evaluation
	void ClearDirtyFlags()
	{
		for (FKLayer& Layer : Layers)
		{
			Layer.bDirty = false;
		}
	}

	// Get visible layers only
	// Returns layer indices in bottom-to-top order (index 0 = bottom, higher indices = top)
	// This order is critical for correct alpha compositing
	TArray<int32> GetVisibleLayerIndices() const
	{
		TArray<int32> Result;
		bool bHasSolo = false;
		
		// Check for any enabled solo layers (disabled layers cannot contribute even if solo-flagged)
		for (int32 i = 0; i < Layers.Num(); ++i)
		{
			const FKLayer& L = Layers[i];
			if (L.bSolo && L.bEnabled && !L.bLocked)
			{
				bHasSolo = true;
				break;
			}
		}
		
		for (int32 i = 0; i < Layers.Num(); ++i)
		{
			const FKLayer& Layer = Layers[i];
			if (!Layer.bEnabled) continue;
			if (Layer.bLocked && Layer.LayerType != EKLayerType::Base) continue;
			if (bHasSolo && !Layer.bSolo) continue;
			Result.Add(i);
		}
		
		return Result;
	}

	// Find layer by GUID
	int32 FindLayerByGuid(const FGuid& Guid) const
	{
		for (int32 i = 0; i < Layers.Num(); ++i)
		{
			if (Layers[i].Id == Guid) return i;
		}
		return INDEX_NONE;
	}

	// Find layer by name
	int32 FindLayerByName(FName Name) const
	{
		for (int32 i = 0; i < Layers.Num(); ++i)
		{
			if (Layers[i].Name == Name) return i;
		}
		return INDEX_NONE;
	}

	// Create common layer types
	static FKLayer CreateImageLayer(FName Name, UTexture2D* Texture)
	{
		FKLayer Layer(Name, EKLayerType::Image);
		Layer.ImageTexture = Texture;
		return Layer;
	}

	static FKLayer CreateFillLayer(FName Name, FLinearColor Color)
	{
		FKLayer Layer(Name, EKLayerType::Fill);
		Layer.FillColor = Color;
		return Layer;
	}

	static FKLayer CreateProceduralLayer(FName Name, EKProceduralNoiseType NoiseType)
	{
		FKLayer Layer(Name, EKLayerType::Procedural);
		Layer.ProceduralParams.NoiseType = NoiseType;
		return Layer;
	}

	static FKLayer CreateFilterLayer(FName Name, EKFilterType FilterType)
	{
		FKLayer Layer(Name, EKLayerType::Filter);
		Layer.FilterParams.FilterType = FilterType;
		return Layer;
	}

	static FKLayer CreateAdjustmentLayer(FName Name, EKAdjustmentType AdjustmentType)
	{
		FKLayer Layer(Name, EKLayerType::Adjustment);
		Layer.AdjustmentParams.AdjustmentType = AdjustmentType;
		return Layer;
	}

	static FKLayer CreateFolderLayer(FName Name)
	{
		return FKLayer(Name, EKLayerType::Folder);
	}
};
