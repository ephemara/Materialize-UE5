// Copyright K-Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHI.h"
#include "DataDrivenShaderPlatformInfo.h"

// Forward declarations
class FRDGBuilder;

/**
 * Noise generation compute shader
 * GPU-accelerated procedural noise generation
 */
class FMaterializeNoiseGeneratorCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMaterializeNoiseGeneratorCS);
	SHADER_USE_PARAMETER_STRUCT(FMaterializeNoiseGeneratorCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FVector4f, NoiseParams)  // x=Scale, y=Octaves, z=Persistence, w=Lacunarity
		SHADER_PARAMETER(FVector4f, NoiseParams2) // x=Seed, y=Contrast, z=Brightness, w=Invert
		SHADER_PARAMETER(FVector4f, TransformParams) // x=Rotation, y=ScaleX, z=ScaleY, w=OffsetX
		SHADER_PARAMETER(FVector4f, TransformParams2) // x=OffsetY, y=TilingMode, z=unused, w=unused
		SHADER_PARAMETER(FUintVector2, TextureSize)
		SHADER_PARAMETER(uint32, NoiseType)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), 8);
	}
};

/**
 * Blend textures compute shader
 * GPU-accelerated texture blending with Photoshop blend modes
 */
class FMaterializeBlendCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMaterializeBlendCS);
	SHADER_USE_PARAMETER_STRUCT(FMaterializeBlendCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, BaseTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, BaseSampler)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, BlendTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, BlendSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(uint32, BlendMode)
		SHADER_PARAMETER(float, Opacity)
		SHADER_PARAMETER(FUintVector2, TextureSize)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

/**
 * Blur horizontal pass compute shader
 */
class FMaterializeBlurHorizontalCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMaterializeBlurHorizontalCS);
	SHADER_USE_PARAMETER_STRUCT(FMaterializeBlurHorizontalCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, TextureSize)
		SHADER_PARAMETER(FVector4f, FilterParams) // x=Strength, y=Radius, z=Threshold, w=unused
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

/**
 * Blur vertical pass compute shader
 */
class FMaterializeBlurVerticalCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMaterializeBlurVerticalCS);
	SHADER_USE_PARAMETER_STRUCT(FMaterializeBlurVerticalCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, TextureSize)
		SHADER_PARAMETER(FVector4f, FilterParams)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

/**
 * Sharpen compute shader
 */
class FMaterializeSharpenCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMaterializeSharpenCS);
	SHADER_USE_PARAMETER_STRUCT(FMaterializeSharpenCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, TextureSize)
		SHADER_PARAMETER(FVector4f, FilterParams)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

/**
 * Edge detection compute shader
 */
class FMaterializeEdgeDetectCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMaterializeEdgeDetectCS);
	SHADER_USE_PARAMETER_STRUCT(FMaterializeEdgeDetectCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, TextureSize)
		SHADER_PARAMETER(FVector4f, FilterParams)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

/**
 * Levels adjustment compute shader
 */
class FMaterializeLevelsCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMaterializeLevelsCS);
	SHADER_USE_PARAMETER_STRUCT(FMaterializeLevelsCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, TextureSize)
		SHADER_PARAMETER(FVector4f, FilterParams2) // x=InBlack, y=InWhite, z=OutBlack, w=OutWhite
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

/**
 * HSL adjustment compute shader
 */
class FMaterializeHSLAdjustCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMaterializeHSLAdjustCS);
	SHADER_USE_PARAMETER_STRUCT(FMaterializeHSLAdjustCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FUintVector2, TextureSize)
		SHADER_PARAMETER(FVector4f, FilterParams) // x=HueShift, y=SaturationMult, z=LightnessMult, w=unused
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

// ============================================================================
// NOISE TYPE ENUM (matches shader defines)
// ============================================================================

UENUM(BlueprintType)
enum class EMaterializeNoiseType : uint8
{
	Perlin = 0,
	Voronoi = 1,
	Worley = 2,
	Ridged = 3,
	Turbulence = 4
};

// ============================================================================
// BLEND MODE ENUM (matches shader defines)
// ============================================================================

UENUM(BlueprintType)
enum class EMaterializeBlendMode : uint8
{
	Normal = 0,
	Add = 1,
	Subtract = 2,
	Multiply = 3,
	Screen = 4,
	Overlay = 5,
	SoftLight = 6,
	HardLight = 7,
	Darken = 8,
	Lighten = 9,
	Difference = 10,
	Exclusion = 11,
	ColorDodge = 12,
	ColorBurn = 13,
	LinearLight = 14,
	VividLight = 15
};
