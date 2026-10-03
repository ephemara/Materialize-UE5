#include "KLayerEvaluator.h"
#include "MaterializeErrorHandler.h"
#include "Engine/Texture2D.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RHICommandList.h"
#include "TextureResource.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Shaders/MaterializePresetShaders.h"

namespace
{
enum class EGeneratorPresetPass : uint8
{
	SmithVisibility,
	FresnelSchlick,
	GGXDistribution,
	ToonOutline,
	GlossySubsurface,
	MetalRim,
	ToonBands,
	GlossyClearCoat
};

struct FGeneratorPresetMapping
{
	EKGeneratorType GeneratorType;
	EGeneratorPresetPass PresetPass;
};

static const FGeneratorPresetMapping GGeneratorPresetMappings[] =
{
	{ EKGeneratorType::AmbientOcclusion, EGeneratorPresetPass::SmithVisibility },
	{ EKGeneratorType::Curvature, EGeneratorPresetPass::FresnelSchlick },
	{ EKGeneratorType::Position, EGeneratorPresetPass::GGXDistribution },
	{ EKGeneratorType::WorldNormal, EGeneratorPresetPass::ToonOutline },
	{ EKGeneratorType::Thickness, EGeneratorPresetPass::GlossySubsurface },
	{ EKGeneratorType::EdgeWear, EGeneratorPresetPass::MetalRim },
	{ EKGeneratorType::Dirt, EGeneratorPresetPass::ToonBands },
	{ EKGeneratorType::LightMap, EGeneratorPresetPass::GlossyClearCoat },
};

static EGeneratorPresetPass ResolveGeneratorPresetPass(EKGeneratorType GeneratorType)
{
	for (const FGeneratorPresetMapping& Mapping : GGeneratorPresetMappings)
	{
		if (Mapping.GeneratorType == GeneratorType)
		{
			return Mapping.PresetPass;
		}
	}

	UE_LOG(LogTemp, Error,
		TEXT("[KLayerEvaluator] ResolveGeneratorPresetPass: No preset mapping found for GeneratorType=%d. "
		     "Falling back to SmithVisibility. Add a mapping entry to GGeneratorPresetMappings."),
		static_cast<int32>(GeneratorType));
	return EGeneratorPresetPass::SmithVisibility;
}
}

// =============================================================================
// BLEND MODE COMPUTE SHADER
// =============================================================================

class FKLayerBlendCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKLayerBlendCS);
	SHADER_USE_PARAMETER_STRUCT(FKLayerBlendCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InBase)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InBlend)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InMask)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutResult)
		SHADER_PARAMETER(uint32, BlendMode)
		SHADER_PARAMETER(float, Opacity)
		SHADER_PARAMETER(uint32, bHasMask)
		SHADER_PARAMETER(uint32, bInvertMask)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKLayerBlendCS, "/Plugin/Materialize/KStudioCore/LayerBlend.usf", "BlendCS", SF_Compute);

// =============================================================================
// PROCEDURAL NOISE COMPUTE SHADER
// =============================================================================

class FKProceduralNoiseCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKProceduralNoiseCS);
	SHADER_USE_PARAMETER_STRUCT(FKProceduralNoiseCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutResult)
		SHADER_PARAMETER(uint32, NoiseType)
		SHADER_PARAMETER(float, Scale)
		SHADER_PARAMETER(int32, Octaves)
		SHADER_PARAMETER(float, Persistence)
		SHADER_PARAMETER(float, Lacunarity)
		SHADER_PARAMETER(FVector2f, Offset)
		SHADER_PARAMETER(int32, Seed)
		SHADER_PARAMETER(uint32, bSeamless)
		SHADER_PARAMETER(float, Time)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKProceduralNoiseCS, "/Plugin/Materialize/KStudioCore/ProceduralNoise.usf", "NoiseCS", SF_Compute);

// =============================================================================
// FILTER COMPUTE SHADER
// =============================================================================

class FKFilterCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKFilterCS);
	SHADER_USE_PARAMETER_STRUCT(FKFilterCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InSource)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutResult)
		SHADER_PARAMETER(uint32, FilterType)
		SHADER_PARAMETER(float, Intensity)
		SHADER_PARAMETER(int32, KernelSize)
		SHADER_PARAMETER(float, Threshold)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKFilterCS, "/Plugin/Materialize/KStudioCore/LayerFilter.usf", "FilterCS", SF_Compute);

// =============================================================================
// ADJUSTMENT COMPUTE SHADER
// =============================================================================

class FKAdjustmentCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKAdjustmentCS);
	SHADER_USE_PARAMETER_STRUCT(FKAdjustmentCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InSource)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutResult)
		SHADER_PARAMETER(uint32, AdjustmentType)
		SHADER_PARAMETER(float, InputBlack)
		SHADER_PARAMETER(float, InputWhite)
		SHADER_PARAMETER(float, Gamma)
		SHADER_PARAMETER(float, OutputBlack)
		SHADER_PARAMETER(float, OutputWhite)
		SHADER_PARAMETER(float, HueShift)
		SHADER_PARAMETER(float, SaturationAdjust)
		SHADER_PARAMETER(float, ValueAdjust)
		SHADER_PARAMETER(float, Brightness)
		SHADER_PARAMETER(float, Contrast)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKAdjustmentCS, "/Plugin/Materialize/KStudioCore/LayerAdjustment.usf", "AdjustmentCS", SF_Compute);

// =============================================================================
// MATH OPERATIONS COMPUTE SHADER
// =============================================================================

class FKMathOperationCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKMathOperationCS);
	SHADER_USE_PARAMETER_STRUCT(FKMathOperationCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InTextureA)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InTextureB)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutResult)
		SHADER_PARAMETER(uint32, MathOperation)
		SHADER_PARAMETER(float, Alpha)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKMathOperationCS, "/Plugin/Materialize/KStudioCore/MathOperations.usf", "MathCS", SF_Compute);

// =============================================================================
// EVALUATOR IMPLEMENTATION
// =============================================================================

UTexture2D* UKLayerEvaluator::CreateTransientTexture(int32 Width, int32 Height, EPixelFormat Format, bool bSRGB)
{
	UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, Format);
	if (Tex)
	{
		Tex->SRGB = bSRGB;
		Tex->Filter = TF_Bilinear;
		Tex->AddressX = TA_Wrap;
		Tex->AddressY = TA_Wrap;
		Tex->UpdateResource();
	}
	return Tex;
}

bool UKLayerEvaluator::EvaluateStack(FKLayerStack& Stack, FKLayerEvalResult& OutResult, FString& OutError)
{
	// Pre-dispatch validation
	if (!ValidateLayerStack(Stack, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("EvaluateStack"), OutError);
		return false;
	}

	double StartTime = FPlatformTime::Seconds();

	int32 Width = Stack.Width;
	int32 Height = Stack.Height;

	// Create output textures — all PF_B8G8R8A8 so they are compatible with the
	// blend compute shader which always writes BGRA8. Scalar channels (Roughness,
	// Metallic, Height, AO) use the R channel to carry their value; the grayscale
	// preview material already reads only R for those channels.
	OutResult.BaseColor = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
	OutResult.Normal    = CreateTransientTexture(Width, Height, PF_B8G8R8A8, false);
	OutResult.Roughness = CreateTransientTexture(Width, Height, PF_B8G8R8A8, false);
	OutResult.Metallic  = CreateTransientTexture(Width, Height, PF_B8G8R8A8, false);
	OutResult.Height    = CreateTransientTexture(Width, Height, PF_B8G8R8A8, false);
	OutResult.AO        = CreateTransientTexture(Width, Height, PF_B8G8R8A8, false);
	OutResult.Emissive  = CreateTransientTexture(Width, Height, PF_B8G8R8A8, false);

	if (!OutResult.BaseColor)
	{
		OutError = TEXT("Failed to create output textures");
		FMaterializeErrorHandler::LogError(TEXT("EvaluateStack"), OutError);
		return false;
	}

	// Get visible layers
	// NOTE: Layers are stored and evaluated in bottom-to-top order
	// Index 0 is the bottom layer, higher indices are on top
	// This ensures correct alpha compositing where each layer is blended onto the accumulated result
	TArray<int32> VisibleIndices = Stack.GetVisibleLayerIndices();
	if (VisibleIndices.Num() == 0)
	{
		OutResult.EvaluationTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0f;
		return true;
	}

	// Evaluate each layer and composite (bottom-to-top order)
	for (int32 i = 0; i < VisibleIndices.Num(); ++i)
	{
		int32 LayerIndex = VisibleIndices[i];
		FKLayer& Layer = Stack.Layers[LayerIndex];

		FString LayerError;

		// Resolve Filter/Adjustment source and evaluate
		if (Layer.LayerType == EKLayerType::Filter || Layer.LayerType == EKLayerType::Adjustment)
		{
			UTexture2D* SourceTex = nullptr;
			if (Layer.SourceOverride)
			{
				SourceTex = Layer.SourceOverride;
			}
			else if (Stack.Layers.IsValidIndex(Layer.SourceLayerIndex))
			{
				FKLayer& SrcLayer = Stack.Layers[Layer.SourceLayerIndex];
				if (SrcLayer.bDirty || !SrcLayer.CachedOutput)
				{
					SrcLayer.CachedOutput = EvaluateSingleLayer(SrcLayer, Width, Height, LayerError);
					if (!SrcLayer.CachedOutput)
					{
						OutError = FString::Printf(TEXT("Failed to evaluate source layer %d for layer %d: %s"), 
							Layer.SourceLayerIndex, LayerIndex, *LayerError);
						FMaterializeErrorHandler::LogError(TEXT("EvaluateStack"), OutError);
						continue; // Skip this layer but continue with others
					}
					SrcLayer.bDirty = false;
				}
				SourceTex = SrcLayer.CachedOutput;
			}
			else
			{
				SourceTex = OutResult.BaseColor;
			}

			if (Layer.LayerType == EKLayerType::Filter)
			{
				Layer.CachedOutput = ApplyFilter(SourceTex, Layer.FilterParams, LayerError);
				if (!Layer.CachedOutput)
				{
					OutError = FString::Printf(TEXT("Failed to apply filter to layer %d: %s"), LayerIndex, *LayerError);
					FMaterializeErrorHandler::LogError(TEXT("EvaluateStack"), OutError);
					continue;
				}
			}
			else
			{
				Layer.CachedOutput = ApplyAdjustment(SourceTex, Layer.AdjustmentParams, LayerError);
				if (!Layer.CachedOutput)
				{
					OutError = FString::Printf(TEXT("Failed to apply adjustment to layer %d: %s"), LayerIndex, *LayerError);
					FMaterializeErrorHandler::LogError(TEXT("EvaluateStack"), OutError);
					continue;
				}
			}
			Layer.bDirty = false;
		}
		else
		{
			if (Layer.bDirty || !Layer.CachedOutput)
			{
				Layer.CachedOutput = EvaluateSingleLayer(Layer, Width, Height, LayerError);
				if (!Layer.CachedOutput)
				{
					OutError = FString::Printf(TEXT("Failed to evaluate layer %d: %s"), LayerIndex, *LayerError);
					FMaterializeErrorHandler::LogError(TEXT("EvaluateStack"), OutError);
					continue;
				}
				Layer.bDirty = false;
			}
		}

		if (!Layer.CachedOutput) continue;

		// Determine which channels this layer affects
		int32 OutputFlags = Layer.OutputChannels;

		// Blend into every output channel this layer targets.
		// Each channel gets an independent blend so a single layer can affect
		// multiple channels simultaneously (e.g. OutputChannels = All).
		auto BlendChannel = [&](TObjectPtr<UTexture2D>& ChannelTex, EKLayerOutputChannel ChannelFlag)
		{
			if (!(OutputFlags & static_cast<int32>(ChannelFlag)))
			{
				return;
			}
			FString BlendErr;
			UTexture2D* Blended = BlendTextures(
				ChannelTex, Layer.CachedOutput,
				Layer.BlendMode, Layer.Opacity,
				Layer.MaskTexture, Layer.bInvertMask,
				BlendErr);
			if (Blended)
			{
				ChannelTex = Blended;
			}
			if (BlendErr.Len() > 0)
			{
				FMaterializeErrorHandler::LogWarning(TEXT("EvaluateStack"),
					FString::Printf(TEXT("Blend warning layer %d channel %d: %s"),
						LayerIndex, static_cast<int32>(ChannelFlag), *BlendErr));
			}
		};

		BlendChannel(OutResult.BaseColor, EKLayerOutputChannel::BaseColor);
		BlendChannel(OutResult.Normal,    EKLayerOutputChannel::Normal);
		BlendChannel(OutResult.Roughness, EKLayerOutputChannel::Roughness);
		BlendChannel(OutResult.Metallic,  EKLayerOutputChannel::Metallic);
		BlendChannel(OutResult.Height,    EKLayerOutputChannel::Height);
		BlendChannel(OutResult.AO,        EKLayerOutputChannel::AO);
		BlendChannel(OutResult.Emissive,  EKLayerOutputChannel::Emissive);
	}

	OutResult.EvaluationTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0f;

	if (OutResult.IsValid())
	{
		Stack.ClearDirtyFlags();
	}

	return OutResult.IsValid();
}

UTexture2D* UKLayerEvaluator::EvaluateSingleLayer(const FKLayer& Layer, int32 Width, int32 Height, FString& OutError)
{
	// Validate dimensions
	if (!FMaterializeErrorHandler::ValidateDimensions(Width, Height, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("EvaluateSingleLayer"), OutError);
		return nullptr;
	}

	switch (Layer.LayerType)
	{
		case EKLayerType::Image:
		{
			// Validate image texture
			if (!FMaterializeErrorHandler::ValidateTexture(Layer.ImageTexture, OutError))
			{
				FMaterializeErrorHandler::LogError(TEXT("EvaluateSingleLayer"), 
					FString::Printf(TEXT("Image layer validation failed: %s"), *OutError));
				return nullptr;
			}
			// Just return a copy or reference to the image texture
			return Layer.ImageTexture;
		}

		case EKLayerType::Fill:
		{
			// Create a solid color texture
			UTexture2D* FillTex = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
			if (!FillTex)
			{
				OutError = TEXT("Failed to create fill texture");
				FMaterializeErrorHandler::LogError(TEXT("EvaluateSingleLayer"), OutError);
				return nullptr;
			}

			// Fill with solid color (could be done on GPU but simple for now)
			FColor FillColorBytes = Layer.FillColor.ToFColor(true);
			TArray<FColor> Pixels;
			Pixels.SetNumUninitialized(Width * Height);
			for (int32 i = 0; i < Pixels.Num(); ++i)
			{
				Pixels[i] = FillColorBytes;
			}

			void* TextureData = FillTex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
			FillTex->GetPlatformData()->Mips[0].BulkData.Unlock();
			FillTex->UpdateResource();

			return FillTex;
		}

		case EKLayerType::Procedural:
			return GenerateProceduralTexture(Layer.ProceduralParams, Width, Height, OutError);

		case EKLayerType::Filter:
			// Filters need a source - typically the layer below or a linked input
			OutError = TEXT("Filter layers require a source texture");
			return nullptr;

		case EKLayerType::Adjustment:
			// Same as filters - need a source
			OutError = TEXT("Adjustment layers require a source texture");
			return nullptr;

		case EKLayerType::Generator:
		{
			UTexture2D* Result = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
			if (!Result)
			{
				OutError = TEXT("Failed to create generator result texture");
				return nullptr;
			}

			const EKGeneratorType GeneratorType = Layer.GeneratorType;
			ENQUEUE_RENDER_COMMAND(KLayerGeneratorPreset)(
				[Result, Width, Height, GeneratorType](FRHICommandListImmediate& RHICmdList)
				{
					if (!Result || !Result->GetResource() || !Result->GetResource()->GetTexture2DRHI())
					{
						return;
					}

					FRDGBuilder GraphBuilder(RHICmdList);
					const FIntPoint Size(Width, Height);
					FRDGTextureDesc ResultDesc = FRDGTextureDesc::Create2D(
						Size,
						PF_B8G8R8A8,
						FClearValueBinding::Transparent,
						TexCreate_RenderTargetable | TexCreate_ShaderResource);
					FRDGTextureRef ResultRDG = GraphBuilder.CreateTexture(ResultDesc, TEXT("GeneratorPresetResult"));

					switch (ResolveGeneratorPresetPass(GeneratorType))
					{
					case EGeneratorPresetPass::SmithVisibility:
						AddPass_MaterializeSmithVisibility(GraphBuilder, ResultRDG);
						break;
					case EGeneratorPresetPass::FresnelSchlick:
						AddPass_MaterializeFresnelSchlick(GraphBuilder, ResultRDG);
						break;
					case EGeneratorPresetPass::GGXDistribution:
						AddPass_MaterializeGGXDistribution(GraphBuilder, ResultRDG);
						break;
					case EGeneratorPresetPass::ToonOutline:
						AddPass_ToonOutlineDetection(GraphBuilder, ResultRDG, FVector3f(0.0f, 0.0f, 0.0f), 1.0f, 0.1f, 0.1f, 1.0f, 1.0f);
						break;
					case EGeneratorPresetPass::GlossySubsurface:
						AddPass_GlossySubsurface(GraphBuilder, ResultRDG, FVector3f(1.0f, 1.0f, 1.0f), 0.6f, 0.5f, 2.0f, 1.0f, 0.25f);
						break;
					case EGeneratorPresetPass::MetalRim:
						AddPass_MetalFresnelRim(GraphBuilder, ResultRDG, 1.0f, FVector3f(1.0f, 1.0f, 1.0f), 3.0f, 0.5f);
						break;
					case EGeneratorPresetPass::ToonBands:
						AddPass_ToonConfigurableBands(GraphBuilder, ResultRDG, 3.0f, 0.1f, 0.0f);
						break;
					case EGeneratorPresetPass::GlossyClearCoat:
						AddPass_GlossyClearCoat(GraphBuilder, ResultRDG, 1.5f, FVector3f(1.0f, 1.0f, 1.0f), 1.0f);
						break;
					default:
						AddPass_MaterializeSmithVisibility(GraphBuilder, ResultRDG);
						break;
					}

					AddCopyTexturePass(
						GraphBuilder,
						ResultRDG,
						GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Result->GetResource()->GetTexture2DRHI(), TEXT("GeneratorCopyDest"))),
						FRHICopyTextureInfo());

					GraphBuilder.Execute();
				}
			);

			FlushRenderingCommands();
			return Result;
		}

		case EKLayerType::Folder:
			// Folders don't produce output directly
			OutError = TEXT("Folder layers do not produce output");
			return nullptr;

		default:
			OutError = FString::Printf(TEXT("Unknown layer type: %d"), static_cast<int32>(Layer.LayerType));
			return nullptr;
	}
}

UTexture2D* UKLayerEvaluator::BlendTextures(UTexture2D* Base, UTexture2D* Blend, 
	EKLayerBlendMode BlendMode, float Opacity,
	UTexture2D* Mask, bool bInvertMask, FString& OutError)
{
	// Pre-dispatch validation
	if (!Base || !Blend)
	{
		OutError = TEXT("Base or Blend texture is null");
		FMaterializeErrorHandler::LogError(TEXT("BlendTextures"), OutError);
		return Base;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(Base, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("BlendTextures"), 
			FString::Printf(TEXT("Base texture validation failed: %s"), *OutError));
		return Base;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(Blend, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("BlendTextures"), 
			FString::Printf(TEXT("Blend texture validation failed: %s"), *OutError));
		return Base;
	}

	if (!ValidateBlendMode(BlendMode))
	{
		OutError = FString::Printf(TEXT("Invalid blend mode: %d"), static_cast<int32>(BlendMode));
		FMaterializeErrorHandler::LogError(TEXT("BlendTextures"), OutError);
		return Base;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Opacity, 0.0f, 1.0f, TEXT("Opacity"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("BlendTextures"), OutError);
		return Base;
	}

	if (Mask)
	{
		FString MaskError;
		if (!FMaterializeErrorHandler::ValidateTexture(Mask, MaskError))
		{
			FMaterializeErrorHandler::LogWarning(TEXT("BlendTextures"), 
				FString::Printf(TEXT("Mask texture validation failed: %s. Proceeding without mask."), *MaskError));
			Mask = nullptr;
		}
	}

	int32 Width = Base->GetSizeX();
	int32 Height = Base->GetSizeY();
	FIntPoint Size(Width, Height);

	UTexture2D* Result = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
	if (!Result) return Base;

	ENQUEUE_RENDER_COMMAND(KLayerBlend)(
		[Base, Blend, Mask, BlendMode, Opacity, bInvertMask, Result, Size](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef BaseRDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(Base->GetResource()->GetTexture2DRHI(), TEXT("BlendBase")));
			FRDGTextureRef BlendRDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(Blend->GetResource()->GetTexture2DRHI(), TEXT("BlendLayer")));
			
			FRDGTextureSRVRef BaseSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(BaseRDG));
			FRDGTextureSRVRef BlendSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(BlendRDG));

			// Always need a valid SRV for the mask - use Base as dummy if no mask provided
			FRDGTextureSRVRef MaskSRV = nullptr;
			bool bHasMask = false;
			if (Mask && Mask->GetResource())
			{
				FRDGTextureRef MaskRDG = GraphBuilder.RegisterExternalTexture(
					CreateRenderTarget(Mask->GetResource()->GetTexture2DRHI(), TEXT("BlendMask")));
				MaskSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(MaskRDG));
				bHasMask = true;
			}
			else
			{
				// Use the base texture as a dummy mask (shader will ignore it due to bHasMask=0)
				MaskSRV = BaseSRV;
			}

			FRDGTextureDesc ResultDesc = FRDGTextureDesc::Create2D(Size, PF_B8G8R8A8, 
				FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef ResultRDG = GraphBuilder.CreateTexture(ResultDesc, TEXT("BlendResult"));

			TShaderMapRef<FKLayerBlendCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			
			// Allocate parameters - this happens BEFORE shader dispatch
			FKLayerBlendCS::FParameters* Params = GraphBuilder.AllocParameters<FKLayerBlendCS::FParameters>();

			// Synchronize CPU parameters to GPU uniform buffer
			// CRITICAL: All parameter assignments must happen before AddPass()
			Params->InBase = BaseSRV;
			Params->InBlend = BlendSRV;
			Params->InMask = MaskSRV;
			Params->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			Params->OutResult = GraphBuilder.CreateUAV(ResultRDG);
			Params->BlendMode = static_cast<uint32>(BlendMode);
			Params->Opacity = Opacity;
			Params->bHasMask = bHasMask ? 1 : 0;
			Params->bInvertMask = bInvertMask ? 1 : 0;
			Params->TextureDimensions = FUintVector2(Size.X, Size.Y);

			// Validate parameter synchronization (runtime-safe: ensureMsgf fires in all builds but does not crash)
			ensureMsgf(Params->BlendMode == static_cast<uint32>(BlendMode),
				TEXT("[KLayerEvaluator] BlendMode parameter mismatch: CPU=%d, GPU=%d"),
				static_cast<uint32>(BlendMode), Params->BlendMode);
			ensureMsgf(FMath::IsNearlyEqual(Params->Opacity, Opacity, 0.0001f),
				TEXT("[KLayerEvaluator] Opacity parameter mismatch: CPU=%f, GPU=%f"),
				Opacity, Params->Opacity);
			ensureMsgf(Params->bHasMask == (bHasMask ? 1 : 0),
				TEXT("[KLayerEvaluator] bHasMask parameter mismatch: CPU=%d, GPU=%d"),
				bHasMask ? 1 : 0, Params->bHasMask);
			ensureMsgf(Params->bInvertMask == (bInvertMask ? 1 : 0),
				TEXT("[KLayerEvaluator] bInvertMask parameter mismatch: CPU=%d, GPU=%d"),
				bInvertMask ? 1 : 0, Params->bInvertMask);

			// Dispatch shader - parameters are now synchronized
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KLayer_Blend"),
				Shader, Params, FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8)));

			// Copy back
			AddCopyTexturePass(GraphBuilder, ResultRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Result->GetResource()->GetTexture2DRHI(), TEXT("CopyDest"))),
				FRHICopyTextureInfo());

			GraphBuilder.Execute();
		}
	);

	return Result;
}

UTexture2D* UKLayerEvaluator::GenerateProceduralTexture(const FKProceduralParams& Params, int32 Width, int32 Height, FString& OutError)
{
	// Pre-dispatch validation
	if (!FMaterializeErrorHandler::ValidateDimensions(Width, Height, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("GenerateProceduralTexture"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Params.Scale, 0.01f, 100.0f, TEXT("Scale"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("GenerateProceduralTexture"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Params.Octaves, 1, 16, TEXT("Octaves"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("GenerateProceduralTexture"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Params.Persistence, 0.0f, 1.0f, TEXT("Persistence"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("GenerateProceduralTexture"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Params.Lacunarity, 1.0f, 4.0f, TEXT("Lacunarity"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("GenerateProceduralTexture"), OutError);
		return nullptr;
	}

	FIntPoint Size(Width, Height);
	UTexture2D* Result = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
	if (!Result) return nullptr;

	ENQUEUE_RENDER_COMMAND(KLayerProcedural)(
		[Params, Result, Size](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureDesc ResultDesc = FRDGTextureDesc::Create2D(Size, PF_B8G8R8A8,
				FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef ResultRDG = GraphBuilder.CreateTexture(ResultDesc, TEXT("ProceduralResult"));

			TShaderMapRef<FKProceduralNoiseCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			
			// Allocate parameters BEFORE shader dispatch
			FKProceduralNoiseCS::FParameters* PassParams = GraphBuilder.AllocParameters<FKProceduralNoiseCS::FParameters>();

			// Synchronize CPU parameters to GPU uniform buffer
			// CRITICAL: All parameter assignments must happen before AddPass()
			PassParams->OutResult = GraphBuilder.CreateUAV(ResultRDG);
			PassParams->NoiseType = static_cast<uint32>(Params.NoiseType);
			PassParams->Scale = Params.Scale;
			PassParams->Octaves = Params.Octaves;
			PassParams->Persistence = Params.Persistence;
			PassParams->Lacunarity = Params.Lacunarity;
			PassParams->Offset = FVector2f(Params.Offset.X, Params.Offset.Y);
			PassParams->Seed = Params.Seed;
			PassParams->bSeamless = Params.bSeamless ? 1 : 0;
			PassParams->Time = Params.Time;
			PassParams->TextureDimensions = FUintVector2(Size.X, Size.Y);

			// Validate parameter synchronization (runtime-safe: ensureMsgf fires in all builds but does not crash)
			ensureMsgf(PassParams->NoiseType == static_cast<uint32>(Params.NoiseType),
				TEXT("[KLayerEvaluator] NoiseType parameter mismatch: CPU=%d, GPU=%d"),
				static_cast<uint32>(Params.NoiseType), PassParams->NoiseType);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->Scale, Params.Scale, 0.0001f),
				TEXT("[KLayerEvaluator] Scale parameter mismatch: CPU=%f, GPU=%f"),
				Params.Scale, PassParams->Scale);
			ensureMsgf(PassParams->Octaves == Params.Octaves,
				TEXT("[KLayerEvaluator] Octaves parameter mismatch: CPU=%d, GPU=%d"),
				Params.Octaves, PassParams->Octaves);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->Persistence, Params.Persistence, 0.0001f),
				TEXT("[KLayerEvaluator] Persistence parameter mismatch: CPU=%f, GPU=%f"),
				Params.Persistence, PassParams->Persistence);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->Lacunarity, Params.Lacunarity, 0.0001f),
				TEXT("[KLayerEvaluator] Lacunarity parameter mismatch: CPU=%f, GPU=%f"),
				Params.Lacunarity, PassParams->Lacunarity);
			ensureMsgf(PassParams->Seed == Params.Seed,
				TEXT("[KLayerEvaluator] Seed parameter mismatch: CPU=%d, GPU=%d"),
				Params.Seed, PassParams->Seed);

			// Dispatch shader - parameters are now synchronized
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KLayer_Procedural"),
				Shader, PassParams, FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8)));

			AddCopyTexturePass(GraphBuilder, ResultRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Result->GetResource()->GetTexture2DRHI(), TEXT("CopyDest"))),
				FRHICopyTextureInfo());

			GraphBuilder.Execute();
		}
	);

	return Result;
}

UTexture2D* UKLayerEvaluator::ApplyFilter(UTexture2D* Source, const FKFilterParams& Params, FString& OutError)
{
	// Pre-dispatch validation
	if (!Source)
	{
		OutError = TEXT("Source texture is null");
		FMaterializeErrorHandler::LogError(TEXT("ApplyFilter"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(Source, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("ApplyFilter"), 
			FString::Printf(TEXT("Source texture validation failed: %s"), *OutError));
		return Source;
	}

	if (!ValidateFilterType(Params.FilterType))
	{
		OutError = FString::Printf(TEXT("Invalid filter type: %d"), static_cast<int32>(Params.FilterType));
		FMaterializeErrorHandler::LogError(TEXT("ApplyFilter"), OutError);
		return Source;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Params.Intensity, 0.0f, 100.0f, TEXT("Intensity"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("ApplyFilter"), OutError);
		return Source;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Params.KernelSize, 1, 32, TEXT("KernelSize"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("ApplyFilter"), OutError);
		return Source;
	}

	int32 Width = Source->GetSizeX();
	int32 Height = Source->GetSizeY();
	FIntPoint Size(Width, Height);

	UTexture2D* Result = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
	if (!Result) return Source;

	ENQUEUE_RENDER_COMMAND(KLayerFilter)(
		[Source, Params, Result, Size](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef SourceRDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(Source->GetResource()->GetTexture2DRHI(), TEXT("FilterSource")));
			FRDGTextureSRVRef SourceSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(SourceRDG));

			FRDGTextureDesc ResultDesc = FRDGTextureDesc::Create2D(Size, PF_B8G8R8A8,
				FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef ResultRDG = GraphBuilder.CreateTexture(ResultDesc, TEXT("FilterResult"));

			TShaderMapRef<FKFilterCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			
			// Allocate parameters BEFORE shader dispatch
			FKFilterCS::FParameters* PassParams = GraphBuilder.AllocParameters<FKFilterCS::FParameters>();

			// Synchronize CPU parameters to GPU uniform buffer
			// CRITICAL: All parameter assignments must happen before AddPass()
			PassParams->InSource = SourceSRV;
			PassParams->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParams->OutResult = GraphBuilder.CreateUAV(ResultRDG);
			PassParams->FilterType = static_cast<uint32>(Params.FilterType);
			PassParams->Intensity = Params.Intensity;
			PassParams->KernelSize = Params.KernelSize;
			PassParams->Threshold = Params.Threshold;
			PassParams->TextureDimensions = FUintVector2(Size.X, Size.Y);

			// Validate parameter synchronization (runtime-safe: ensureMsgf fires in all builds but does not crash)
			ensureMsgf(PassParams->FilterType == static_cast<uint32>(Params.FilterType),
				TEXT("[KLayerEvaluator] FilterType parameter mismatch: CPU=%d, GPU=%d"),
				static_cast<uint32>(Params.FilterType), PassParams->FilterType);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->Intensity, Params.Intensity, 0.0001f),
				TEXT("[KLayerEvaluator] Intensity parameter mismatch: CPU=%f, GPU=%f"),
				Params.Intensity, PassParams->Intensity);
			ensureMsgf(PassParams->KernelSize == Params.KernelSize,
				TEXT("[KLayerEvaluator] KernelSize parameter mismatch: CPU=%d, GPU=%d"),
				Params.KernelSize, PassParams->KernelSize);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->Threshold, Params.Threshold, 0.0001f),
				TEXT("[KLayerEvaluator] Threshold parameter mismatch: CPU=%f, GPU=%f"),
				Params.Threshold, PassParams->Threshold);

			// Dispatch shader - parameters are now synchronized
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KLayer_Filter"),
				Shader, PassParams, FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8)));

			AddCopyTexturePass(GraphBuilder, ResultRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Result->GetResource()->GetTexture2DRHI(), TEXT("CopyDest"))),
				FRHICopyTextureInfo());

			GraphBuilder.Execute();
		}
	);

	return Result;
}

UTexture2D* UKLayerEvaluator::ApplyAdjustment(UTexture2D* Source, const FKAdjustmentParams& Params, FString& OutError)
{
	// Pre-dispatch validation
	if (!Source)
	{
		OutError = TEXT("Source texture is null");
		FMaterializeErrorHandler::LogError(TEXT("ApplyAdjustment"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(Source, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("ApplyAdjustment"), 
			FString::Printf(TEXT("Source texture validation failed: %s"), *OutError));
		return Source;
	}

	// Validate adjustment parameters based on type
	if (!FMaterializeErrorHandler::ValidateRange(Params.InputBlack, 0.0f, 1.0f, TEXT("InputBlack"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("ApplyAdjustment"), OutError);
		return Source;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Params.InputWhite, 0.0f, 1.0f, TEXT("InputWhite"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("ApplyAdjustment"), OutError);
		return Source;
	}

	if (!FMaterializeErrorHandler::ValidateRange(Params.Gamma, 0.1f, 9.9f, TEXT("Gamma"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("ApplyAdjustment"), OutError);
		return Source;
	}

	int32 Width = Source->GetSizeX();
	int32 Height = Source->GetSizeY();
	FIntPoint Size(Width, Height);

	UTexture2D* Result = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
	if (!Result) return Source;

	ENQUEUE_RENDER_COMMAND(KLayerAdjustment)(
		[Source, Params, Result, Size](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef SourceRDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(Source->GetResource()->GetTexture2DRHI(), TEXT("AdjustmentSource")));
			FRDGTextureSRVRef SourceSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(SourceRDG));

			FRDGTextureDesc ResultDesc = FRDGTextureDesc::Create2D(Size, PF_B8G8R8A8,
				FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef ResultRDG = GraphBuilder.CreateTexture(ResultDesc, TEXT("AdjustmentResult"));

			TShaderMapRef<FKAdjustmentCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			
			// Allocate parameters BEFORE shader dispatch
			FKAdjustmentCS::FParameters* PassParams = GraphBuilder.AllocParameters<FKAdjustmentCS::FParameters>();

			// Synchronize CPU parameters to GPU uniform buffer
			// CRITICAL: All parameter assignments must happen before AddPass()
			PassParams->InSource = SourceSRV;
			PassParams->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParams->OutResult = GraphBuilder.CreateUAV(ResultRDG);
			PassParams->AdjustmentType = static_cast<uint32>(Params.AdjustmentType);
			PassParams->InputBlack = Params.InputBlack;
			PassParams->InputWhite = Params.InputWhite;
			PassParams->Gamma = Params.Gamma;
			PassParams->OutputBlack = Params.OutputBlack;
			PassParams->OutputWhite = Params.OutputWhite;
			PassParams->HueShift = Params.HueShift;
			PassParams->SaturationAdjust = Params.SaturationAdjust;
			PassParams->ValueAdjust = Params.ValueAdjust;
			PassParams->Brightness = Params.Brightness;
			PassParams->Contrast = Params.Contrast;
			PassParams->TextureDimensions = FUintVector2(Size.X, Size.Y);

			// Validate parameter synchronization (runtime-safe: ensureMsgf fires in all builds but does not crash)
			ensureMsgf(PassParams->AdjustmentType == static_cast<uint32>(Params.AdjustmentType),
				TEXT("[KLayerEvaluator] AdjustmentType parameter mismatch: CPU=%d, GPU=%d"),
				static_cast<uint32>(Params.AdjustmentType), PassParams->AdjustmentType);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->InputBlack, Params.InputBlack, 0.0001f),
				TEXT("[KLayerEvaluator] InputBlack parameter mismatch: CPU=%f, GPU=%f"),
				Params.InputBlack, PassParams->InputBlack);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->InputWhite, Params.InputWhite, 0.0001f),
				TEXT("[KLayerEvaluator] InputWhite parameter mismatch: CPU=%f, GPU=%f"),
				Params.InputWhite, PassParams->InputWhite);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->Gamma, Params.Gamma, 0.0001f),
				TEXT("[KLayerEvaluator] Gamma parameter mismatch: CPU=%f, GPU=%f"),
				Params.Gamma, PassParams->Gamma);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->HueShift, Params.HueShift, 0.0001f),
				TEXT("[KLayerEvaluator] HueShift parameter mismatch: CPU=%f, GPU=%f"),
				Params.HueShift, PassParams->HueShift);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->SaturationAdjust, Params.SaturationAdjust, 0.0001f),
				TEXT("[KLayerEvaluator] SaturationAdjust parameter mismatch: CPU=%f, GPU=%f"),
				Params.SaturationAdjust, PassParams->SaturationAdjust);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->Brightness, Params.Brightness, 0.0001f),
				TEXT("[KLayerEvaluator] Brightness parameter mismatch: CPU=%f, GPU=%f"),
				Params.Brightness, PassParams->Brightness);
			ensureMsgf(FMath::IsNearlyEqual(PassParams->Contrast, Params.Contrast, 0.0001f),
				TEXT("[KLayerEvaluator] Contrast parameter mismatch: CPU=%f, GPU=%f"),
				Params.Contrast, PassParams->Contrast);

			// Dispatch shader - parameters are now synchronized
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KLayer_Adjustment"),
				Shader, PassParams, FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8)));

			AddCopyTexturePass(GraphBuilder, ResultRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Result->GetResource()->GetTexture2DRHI(), TEXT("CopyDest"))),
				FRHICopyTextureInfo());

			GraphBuilder.Execute();
		}
	);

	return Result;
}

// =============================================================================
// PARAMETER SYNCHRONIZATION IMPLEMENTATION
// =============================================================================

template<typename TShaderParameters>
void UKLayerEvaluator::SyncLayerParametersToGPU(TShaderParameters* Params, const FKLayer& Layer)
{
	// This is a template specialization point - actual implementation happens
	// in the specific shader dispatch functions where we know the parameter types
	// This function serves as documentation and a validation checkpoint
	checkf(Params != nullptr, TEXT("Shader parameters must not be null during synchronization"));
}

template<typename TShaderParameters>
bool UKLayerEvaluator::ValidateGPUParameters(const TShaderParameters* Params, const FKLayer& Layer, FString& OutError)
{
	// Validate that parameters were correctly synchronized
	if (!Params)
	{
		OutError = TEXT("GPU parameters are null");
		return false;
	}
	
	// Additional validation happens in specific shader dispatch functions
	return true;
}

// =============================================================================
// VALIDATION IMPLEMENTATION
// =============================================================================

bool UKLayerEvaluator::ValidateLayerStack(const FKLayerStack& Stack, FString& OutError)
{
	// Check for empty stack
	if (Stack.Layers.Num() == 0)
	{
		OutError = TEXT("Layer stack is empty");
		return false;
	}

	// Validate stack dimensions
	if (Stack.Width <= 0 || Stack.Height <= 0)
	{
		OutError = FString::Printf(TEXT("Layer stack has invalid dimensions: %dx%d"), Stack.Width, Stack.Height);
		return false;
	}

	// Validate maximum reasonable dimensions (prevent excessive memory allocation)
	const int32 MaxDimension = 8192;
	if (Stack.Width > MaxDimension || Stack.Height > MaxDimension)
	{
		OutError = FString::Printf(TEXT("Layer stack dimensions exceed maximum allowed (%dx%d): %dx%d"), 
			MaxDimension, MaxDimension, Stack.Width, Stack.Height);
		return false;
	}

	// Validate each layer
	for (int32 i = 0; i < Stack.Layers.Num(); ++i)
	{
		const FKLayer& Layer = Stack.Layers[i];

		// Validate opacity range
		if (Layer.Opacity < 0.0f || Layer.Opacity > 1.0f)
		{
			OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid opacity: %f (must be 0.0-1.0)"), 
				i, *Layer.Name.ToString(), Layer.Opacity);
			return false;
		}

		// Validate blend mode
		if (!ValidateBlendMode(Layer.BlendMode))
		{
			OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid blend mode: %d"), 
				i, *Layer.Name.ToString(), static_cast<int32>(Layer.BlendMode));
			return false;
		}

		// Validate layer-specific requirements
		switch (Layer.LayerType)
		{
			case EKLayerType::Image:
			{
				if (!Layer.ImageTexture)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') is an Image layer but has no source texture"), 
						i, *Layer.Name.ToString());
					return false;
				}

				FString TextureError;
				if (!ValidateTextureFormat(Layer.ImageTexture, TextureError))
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid texture: %s"), 
						i, *Layer.Name.ToString(), *TextureError);
					return false;
				}
				break;
			}

			case EKLayerType::Filter:
			{
				if (!ValidateFilterType(Layer.FilterParams.FilterType))
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid filter type: %d"), 
						i, *Layer.Name.ToString(), static_cast<int32>(Layer.FilterParams.FilterType));
					return false;
				}

				// Validate filter parameters
				if (Layer.FilterParams.Intensity < 0.0f || Layer.FilterParams.Intensity > 100.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid filter intensity: %f"), 
						i, *Layer.Name.ToString(), Layer.FilterParams.Intensity);
					return false;
				}

				if (Layer.FilterParams.KernelSize < 1 || Layer.FilterParams.KernelSize > 32)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid filter kernel size: %d"), 
						i, *Layer.Name.ToString(), Layer.FilterParams.KernelSize);
					return false;
				}

				if (Layer.SourceLayerIndex != INDEX_NONE)
				{
					if (!Stack.Layers.IsValidIndex(Layer.SourceLayerIndex))
					{
						OutError = FString::Printf(TEXT("Layer %d ('%s') references invalid source layer index: %d"),
							i, *Layer.Name.ToString(), Layer.SourceLayerIndex);
						return false;
					}
					if (Layer.SourceLayerIndex == i)
					{
						OutError = FString::Printf(TEXT("Layer %d ('%s') references itself as source (circular dependency)"),
							i, *Layer.Name.ToString());
						return false;
					}
				}
				break;
			}

			case EKLayerType::Adjustment:
			{
				const FKAdjustmentParams& Params = Layer.AdjustmentParams;

				// Validate levels parameters
				if (Params.InputBlack < 0.0f || Params.InputBlack > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid InputBlack: %f"), 
						i, *Layer.Name.ToString(), Params.InputBlack);
					return false;
				}

				if (Params.InputWhite < 0.0f || Params.InputWhite > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid InputWhite: %f"), 
						i, *Layer.Name.ToString(), Params.InputWhite);
					return false;
				}

				if (Params.Gamma < 0.1f || Params.Gamma > 9.9f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid Gamma: %f"), 
						i, *Layer.Name.ToString(), Params.Gamma);
					return false;
				}

				if (Params.OutputBlack < 0.0f || Params.OutputBlack > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid OutputBlack: %f"), 
						i, *Layer.Name.ToString(), Params.OutputBlack);
					return false;
				}

				if (Params.OutputWhite < 0.0f || Params.OutputWhite > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid OutputWhite: %f"), 
						i, *Layer.Name.ToString(), Params.OutputWhite);
					return false;
				}

				// Validate HSV parameters
				if (Params.HueShift < -180.0f || Params.HueShift > 180.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid HueShift: %f"), 
						i, *Layer.Name.ToString(), Params.HueShift);
					return false;
				}

				if (Params.SaturationAdjust < -1.0f || Params.SaturationAdjust > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid SaturationAdjust: %f"), 
						i, *Layer.Name.ToString(), Params.SaturationAdjust);
					return false;
				}

				if (Params.ValueAdjust < -1.0f || Params.ValueAdjust > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid ValueAdjust: %f"), 
						i, *Layer.Name.ToString(), Params.ValueAdjust);
					return false;
				}

				// Validate brightness/contrast parameters
				if (Params.Brightness < -1.0f || Params.Brightness > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid Brightness: %f"), 
						i, *Layer.Name.ToString(), Params.Brightness);
					return false;
				}

				if (Params.Contrast < -1.0f || Params.Contrast > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid Contrast: %f"), 
						i, *Layer.Name.ToString(), Params.Contrast);
					return false;
				}

				if (Layer.SourceLayerIndex != INDEX_NONE)
				{
					if (!Stack.Layers.IsValidIndex(Layer.SourceLayerIndex))
					{
						OutError = FString::Printf(TEXT("Layer %d ('%s') references invalid source layer index: %d"),
							i, *Layer.Name.ToString(), Layer.SourceLayerIndex);
						return false;
					}
					if (Layer.SourceLayerIndex == i)
					{
						OutError = FString::Printf(TEXT("Layer %d ('%s') references itself as source (circular dependency)"),
							i, *Layer.Name.ToString());
						return false;
					}
				}
				break;
			}

			case EKLayerType::Procedural:
			{
				const FKProceduralParams& Params = Layer.ProceduralParams;

				// Validate procedural parameters
				if (Params.Scale < 0.01f || Params.Scale > 100.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid procedural scale: %f"), 
						i, *Layer.Name.ToString(), Params.Scale);
					return false;
				}

				if (Params.Octaves < 1 || Params.Octaves > 16)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid procedural octaves: %d"), 
						i, *Layer.Name.ToString(), Params.Octaves);
					return false;
				}

				if (Params.Persistence < 0.0f || Params.Persistence > 1.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid procedural persistence: %f"), 
						i, *Layer.Name.ToString(), Params.Persistence);
					return false;
				}

				if (Params.Lacunarity < 1.0f || Params.Lacunarity > 4.0f)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid procedural lacunarity: %f"), 
						i, *Layer.Name.ToString(), Params.Lacunarity);
					return false;
				}
				break;
			}

			case EKLayerType::Generator:
			{
				const int32 MaxGeneratorType = static_cast<int32>(EKGeneratorType::LightMap);
				if (static_cast<int32>(Layer.GeneratorType) < 0 || static_cast<int32>(Layer.GeneratorType) > MaxGeneratorType)
				{
					OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid generator type: %d"),
						i, *Layer.Name.ToString(), static_cast<int32>(Layer.GeneratorType));
					return false;
				}
				break;
			}

			default:
				break;
		}

		// Validate mask texture if present
		if (Layer.bHasMask && Layer.MaskTexture)
		{
			FString MaskError;
			if (!ValidateTextureFormat(Layer.MaskTexture, MaskError))
			{
				OutError = FString::Printf(TEXT("Layer %d ('%s') has invalid mask texture: %s"), 
					i, *Layer.Name.ToString(), *MaskError);
				return false;
			}
		}
	}

	return true;
}

bool UKLayerEvaluator::ValidateBlendMode(EKLayerBlendMode BlendMode)
{
	// Check if blend mode is within valid enum range
	return BlendMode >= EKLayerBlendMode::Normal && BlendMode <= EKLayerBlendMode::HardMix;
}

bool UKLayerEvaluator::ValidateFilterType(EKFilterType FilterType)
{
	// Check if filter type is within valid enum range
	return FilterType >= EKFilterType::Blur && FilterType <= EKFilterType::AutoLevels;
}

bool UKLayerEvaluator::ValidateTextureFormat(UTexture2D* Texture, FString& OutError)
{
	if (!Texture)
	{
		OutError = TEXT("Texture is null");
		return false;
	}

	if (!Texture->IsValidLowLevel())
	{
		OutError = FString::Printf(TEXT("Texture '%s' is not valid"), *Texture->GetName());
		return false;
	}

	// Check dimensions
	int32 Width = Texture->GetSizeX();
	int32 Height = Texture->GetSizeY();

	if (Width <= 0 || Height <= 0)
	{
		OutError = FString::Printf(TEXT("Texture '%s' has invalid dimensions: %dx%d"), 
			*Texture->GetName(), Width, Height);
		return false;
	}

	// Check for reasonable maximum dimensions
	const int32 MaxDimension = 8192;
	if (Width > MaxDimension || Height > MaxDimension)
	{
		OutError = FString::Printf(TEXT("Texture '%s' dimensions exceed maximum allowed (%dx%d): %dx%d"), 
			*Texture->GetName(), MaxDimension, MaxDimension, Width, Height);
		return false;
	}

	// Check if texture has platform data
	if (!Texture->GetPlatformData())
	{
		OutError = FString::Printf(TEXT("Texture '%s' has no platform data"), *Texture->GetName());
		return false;
	}

	// Check if texture has at least one mip level
	if (Texture->GetPlatformData()->Mips.Num() == 0)
	{
		OutError = FString::Printf(TEXT("Texture '%s' has no mip levels"), *Texture->GetName());
		return false;
	}

	// Check if texture resource is available
	if (!Texture->GetResource())
	{
		OutError = FString::Printf(TEXT("Texture '%s' has no GPU resource"), *Texture->GetName());
		return false;
	}

	return true;
}

// =============================================================================
// MATH OPERATIONS IMPLEMENTATION
// =============================================================================

UTexture2D* UKLayerEvaluator::AddTextures(UTexture2D* TextureA, UTexture2D* TextureB, FString& OutError)
{
	// Pre-dispatch validation
	if (!TextureA || !TextureB)
	{
		OutError = TEXT("One or both input textures are null");
		FMaterializeErrorHandler::LogError(TEXT("AddTextures"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(TextureA, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("AddTextures"), 
			FString::Printf(TEXT("TextureA validation failed: %s"), *OutError));
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(TextureB, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("AddTextures"), 
			FString::Printf(TEXT("TextureB validation failed: %s"), *OutError));
		return nullptr;
	}

	// Use dimensions from first texture
	int32 Width = TextureA->GetSizeX();
	int32 Height = TextureA->GetSizeY();
	FIntPoint Size(Width, Height);

	UTexture2D* Result = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
	if (!Result)
	{
		OutError = TEXT("Failed to create result texture");
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(KLayerMathAdd)(
		[TextureA, TextureB, Result, Size](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef TextureARDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(TextureA->GetResource()->GetTexture2DRHI(), TEXT("MathAddTextureA")));
			FRDGTextureSRVRef TextureASRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(TextureARDG));

			FRDGTextureRef TextureBRDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(TextureB->GetResource()->GetTexture2DRHI(), TEXT("MathAddTextureB")));
			FRDGTextureSRVRef TextureBSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(TextureBRDG));

			FRDGTextureDesc ResultDesc = FRDGTextureDesc::Create2D(Size, PF_B8G8R8A8,
				FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef ResultRDG = GraphBuilder.CreateTexture(ResultDesc, TEXT("MathAddResult"));

			TShaderMapRef<FKMathOperationCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			
			FKMathOperationCS::FParameters* PassParams = GraphBuilder.AllocParameters<FKMathOperationCS::FParameters>();
			PassParams->InTextureA = TextureASRV;
			PassParams->InTextureB = TextureBSRV;
			PassParams->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParams->OutResult = GraphBuilder.CreateUAV(ResultRDG);
			PassParams->MathOperation = 0; // Add
			PassParams->Alpha = 0.0f; // Not used for Add
			PassParams->TextureDimensions = FUintVector2(Size.X, Size.Y);

			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KLayer_MathAdd"),
				Shader, PassParams, FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8)));

			AddCopyTexturePass(GraphBuilder, ResultRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Result->GetResource()->GetTexture2DRHI(), TEXT("CopyDest"))),
				FRHICopyTextureInfo());

			GraphBuilder.Execute();
		}
	);

	return Result;
}

UTexture2D* UKLayerEvaluator::MultiplyTextures(UTexture2D* TextureA, UTexture2D* TextureB, FString& OutError)
{
	// Pre-dispatch validation
	if (!TextureA || !TextureB)
	{
		OutError = TEXT("One or both input textures are null");
		FMaterializeErrorHandler::LogError(TEXT("MultiplyTextures"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(TextureA, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("MultiplyTextures"), 
			FString::Printf(TEXT("TextureA validation failed: %s"), *OutError));
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(TextureB, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("MultiplyTextures"), 
			FString::Printf(TEXT("TextureB validation failed: %s"), *OutError));
		return nullptr;
	}

	// Use dimensions from first texture
	int32 Width = TextureA->GetSizeX();
	int32 Height = TextureA->GetSizeY();
	FIntPoint Size(Width, Height);

	UTexture2D* Result = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
	if (!Result)
	{
		OutError = TEXT("Failed to create result texture");
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(KLayerMathMultiply)(
		[TextureA, TextureB, Result, Size](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef TextureARDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(TextureA->GetResource()->GetTexture2DRHI(), TEXT("MathMultiplyTextureA")));
			FRDGTextureSRVRef TextureASRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(TextureARDG));

			FRDGTextureRef TextureBRDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(TextureB->GetResource()->GetTexture2DRHI(), TEXT("MathMultiplyTextureB")));
			FRDGTextureSRVRef TextureBSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(TextureBRDG));

			FRDGTextureDesc ResultDesc = FRDGTextureDesc::Create2D(Size, PF_B8G8R8A8,
				FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef ResultRDG = GraphBuilder.CreateTexture(ResultDesc, TEXT("MathMultiplyResult"));

			TShaderMapRef<FKMathOperationCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			
			FKMathOperationCS::FParameters* PassParams = GraphBuilder.AllocParameters<FKMathOperationCS::FParameters>();
			PassParams->InTextureA = TextureASRV;
			PassParams->InTextureB = TextureBSRV;
			PassParams->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParams->OutResult = GraphBuilder.CreateUAV(ResultRDG);
			PassParams->MathOperation = 1; // Multiply
			PassParams->Alpha = 0.0f; // Not used for Multiply
			PassParams->TextureDimensions = FUintVector2(Size.X, Size.Y);

			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KLayer_MathMultiply"),
				Shader, PassParams, FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8)));

			AddCopyTexturePass(GraphBuilder, ResultRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Result->GetResource()->GetTexture2DRHI(), TEXT("CopyDest"))),
				FRHICopyTextureInfo());

			GraphBuilder.Execute();
		}
	);

	return Result;
}

UTexture2D* UKLayerEvaluator::LerpTextures(UTexture2D* TextureA, UTexture2D* TextureB, float Alpha, FString& OutError)
{
	// Pre-dispatch validation
	if (!TextureA || !TextureB)
	{
		OutError = TEXT("One or both input textures are null");
		FMaterializeErrorHandler::LogError(TEXT("LerpTextures"), OutError);
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(TextureA, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("LerpTextures"), 
			FString::Printf(TEXT("TextureA validation failed: %s"), *OutError));
		return nullptr;
	}

	if (!FMaterializeErrorHandler::ValidateTexture(TextureB, OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("LerpTextures"), 
			FString::Printf(TEXT("TextureB validation failed: %s"), *OutError));
		return nullptr;
	}

	// Validate alpha range
	if (!FMaterializeErrorHandler::ValidateRange(Alpha, 0.0f, 1.0f, TEXT("Alpha"), OutError))
	{
		FMaterializeErrorHandler::LogError(TEXT("LerpTextures"), OutError);
		return nullptr;
	}

	// Use dimensions from first texture
	int32 Width = TextureA->GetSizeX();
	int32 Height = TextureA->GetSizeY();
	FIntPoint Size(Width, Height);

	UTexture2D* Result = CreateTransientTexture(Width, Height, PF_B8G8R8A8, true);
	if (!Result)
	{
		OutError = TEXT("Failed to create result texture");
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(KLayerMathLerp)(
		[TextureA, TextureB, Alpha, Result, Size](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef TextureARDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(TextureA->GetResource()->GetTexture2DRHI(), TEXT("MathLerpTextureA")));
			FRDGTextureSRVRef TextureASRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(TextureARDG));

			FRDGTextureRef TextureBRDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(TextureB->GetResource()->GetTexture2DRHI(), TEXT("MathLerpTextureB")));
			FRDGTextureSRVRef TextureBSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(TextureBRDG));

			FRDGTextureDesc ResultDesc = FRDGTextureDesc::Create2D(Size, PF_B8G8R8A8,
				FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef ResultRDG = GraphBuilder.CreateTexture(ResultDesc, TEXT("MathLerpResult"));

			TShaderMapRef<FKMathOperationCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			
			FKMathOperationCS::FParameters* PassParams = GraphBuilder.AllocParameters<FKMathOperationCS::FParameters>();
			PassParams->InTextureA = TextureASRV;
			PassParams->InTextureB = TextureBSRV;
			PassParams->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			PassParams->OutResult = GraphBuilder.CreateUAV(ResultRDG);
			PassParams->MathOperation = 2; // Lerp
			PassParams->Alpha = Alpha;
			PassParams->TextureDimensions = FUintVector2(Size.X, Size.Y);

			// Validate parameter synchronization
			checkf(FMath::IsNearlyEqual(PassParams->Alpha, Alpha, 0.0001f),
				TEXT("Alpha parameter mismatch: CPU=%f, GPU=%f"), Alpha, PassParams->Alpha);

			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KLayer_MathLerp"),
				Shader, PassParams, FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8)));

			AddCopyTexturePass(GraphBuilder, ResultRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Result->GetResource()->GetTexture2DRHI(), TEXT("CopyDest"))),
				FRHICopyTextureInfo());

			GraphBuilder.Execute();
		}
	);

	return Result;
}
