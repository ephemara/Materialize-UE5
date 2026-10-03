#include "MaterializeComputeEngine.h"
#include "MaterializeRDGScope.h"
#include "Engine/Texture2D.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderTargetPool.h"
#include "TextureResource.h"
#include "RHICommandList.h"
#include "RenderingThread.h"
#include "DataDrivenShaderPlatformInfo.h"

// =============================================================================
// SHADER DECLARATIONS
// =============================================================================

// --- Pass 1: Gradient Extraction ---
class FKGradientCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKGradientCS);
	SHADER_USE_PARAMETER_STRUCT(FKGradientCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InSourceTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSourceSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, OutGradient)
		SHADER_PARAMETER(float, NormalStrength)
		SHADER_PARAMETER(float, RoughnessBase)
		SHADER_PARAMETER(float, RoughnessContrast)
		SHADER_PARAMETER(uint32, bRoughnessInvert)
		SHADER_PARAMETER(float, MetallicBase)
		SHADER_PARAMETER(float, MetallicContrast)
		SHADER_PARAMETER(float, MetallicBias)
		SHADER_PARAMETER(float, MetallicSensitivity)
		SHADER_PARAMETER(float, AOIntensity)
		SHADER_PARAMETER(float, HeightContrast)
		SHADER_PARAMETER(float, BioDetail)
		SHADER_PARAMETER(float, BioFrequency)
		SHADER_PARAMETER(float, CyberDetail)
		SHADER_PARAMETER(float, CyberScale)
		SHADER_PARAMETER(float, EdgeWear)
		SHADER_PARAMETER(float, CavityDirt)
		SHADER_PARAMETER(float, Dust)
		SHADER_PARAMETER(float, Grunge)
		SHADER_PARAMETER(float, Scratches)
		SHADER_PARAMETER(float, Noise)
		SHADER_PARAMETER(float, EmissiveThreshold)
		SHADER_PARAMETER(float, EmissiveColorBoost)
		SHADER_PARAMETER(float, VarianceWeight)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
		SHADER_PARAMETER(uint32, bAdvancedNormal)
		SHADER_PARAMETER(uint32, bAdvancedAO)
		SHADER_PARAMETER(int32, NormalOctaves)
		SHADER_PARAMETER(float, NormalSigmaBase)
		SHADER_PARAMETER(float, NormalAnisotropy)
		SHADER_PARAMETER(float, AORadius)
		SHADER_PARAMETER(float, AOBias)
		SHADER_PARAMETER(float, AOContrast)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKGradientCS, "/Plugin/Materialize/PBRGenerator.usf", "GradientCS", SF_Compute);

// --- Pass 2: Height Integration (Jacobi Iteration) ---
class FKHeightIntegrationCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKHeightIntegrationCS);
	SHADER_USE_PARAMETER_STRUCT(FKHeightIntegrationCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float2>, InGradient)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InHeightPrev)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutHeightNext)
		
		SHADER_PARAMETER(float, NormalStrength)
		SHADER_PARAMETER(float, RoughnessBase)
		SHADER_PARAMETER(float, RoughnessContrast)
		SHADER_PARAMETER(uint32, bRoughnessInvert)
		SHADER_PARAMETER(float, MetallicBase)
		SHADER_PARAMETER(float, MetallicContrast)
		SHADER_PARAMETER(float, MetallicBias)
		SHADER_PARAMETER(float, MetallicSensitivity)
		SHADER_PARAMETER(float, AOIntensity)
		SHADER_PARAMETER(float, HeightContrast)
		SHADER_PARAMETER(float, BioDetail)
		SHADER_PARAMETER(float, BioFrequency)
		SHADER_PARAMETER(float, CyberDetail)
		SHADER_PARAMETER(float, CyberScale)
		SHADER_PARAMETER(float, EdgeWear)
		SHADER_PARAMETER(float, CavityDirt)
		SHADER_PARAMETER(float, Dust)
		SHADER_PARAMETER(float, Grunge)
		SHADER_PARAMETER(float, Scratches)
		SHADER_PARAMETER(float, Noise)
		SHADER_PARAMETER(float, EmissiveThreshold)
		SHADER_PARAMETER(float, EmissiveColorBoost)
		SHADER_PARAMETER(float, VarianceWeight)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
		SHADER_PARAMETER(uint32, bAdvancedNormal)
		SHADER_PARAMETER(uint32, bAdvancedAO)
		SHADER_PARAMETER(int32, NormalOctaves)
		SHADER_PARAMETER(float, NormalSigmaBase)
		SHADER_PARAMETER(float, NormalAnisotropy)
		SHADER_PARAMETER(float, AORadius)
		SHADER_PARAMETER(float, AOBias)
		SHADER_PARAMETER(float, AOContrast)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKHeightIntegrationCS, "/Plugin/Materialize/PBRGenerator.usf", "HeightIntegrationCS", SF_Compute);

// --- Pass 3: Final PBR Generation ---
class FKFinalPBRCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKFinalPBRCS);
	SHADER_USE_PARAMETER_STRUCT(FKFinalPBRCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InSourceTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSourceSampler)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InHeightPrev)
		
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutNormal)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutRoughness)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutMetallic)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutAO)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutHeight)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutEmissive)
		
		SHADER_PARAMETER(float, NormalStrength)
		SHADER_PARAMETER(float, RoughnessBase)
		SHADER_PARAMETER(float, RoughnessContrast)
		SHADER_PARAMETER(uint32, bRoughnessInvert)
		SHADER_PARAMETER(float, MetallicBase)
		SHADER_PARAMETER(float, MetallicContrast)
		SHADER_PARAMETER(float, MetallicBias)
		SHADER_PARAMETER(float, MetallicSensitivity)
		SHADER_PARAMETER(float, AOIntensity)
		SHADER_PARAMETER(float, HeightContrast)
		
		SHADER_PARAMETER(float, BioDetail)
		SHADER_PARAMETER(float, BioFrequency)
		SHADER_PARAMETER(float, CyberDetail)
		SHADER_PARAMETER(float, CyberScale)
		
		SHADER_PARAMETER(float, EdgeWear)
		SHADER_PARAMETER(float, CavityDirt)
		SHADER_PARAMETER(float, Dust)
		SHADER_PARAMETER(float, Grunge)
		SHADER_PARAMETER(float, Scratches)
		SHADER_PARAMETER(float, Noise)
		
		SHADER_PARAMETER(float, EmissiveThreshold)
		SHADER_PARAMETER(float, EmissiveColorBoost)
		SHADER_PARAMETER(float, VarianceWeight)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
		SHADER_PARAMETER(uint32, bAdvancedNormal)
		SHADER_PARAMETER(uint32, bAdvancedAO)
		SHADER_PARAMETER(int32, NormalOctaves)
		SHADER_PARAMETER(float, NormalSigmaBase)
		SHADER_PARAMETER(float, NormalAnisotropy)
		SHADER_PARAMETER(float, AORadius)
		SHADER_PARAMETER(float, AOBias)
		SHADER_PARAMETER(float, AOContrast)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKFinalPBRCS, "/Plugin/Materialize/PBRGenerator.usf", "FinalPBRCS", SF_Compute);

// --- Legacy Single-Pass (for fast preview) ---
class FKPBRGeneratorCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKPBRGeneratorCS);
	SHADER_USE_PARAMETER_STRUCT(FKPBRGeneratorCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InSourceTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSourceSampler)
		
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutNormal)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutRoughness)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutMetallic)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutAO)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutHeight)
		
		SHADER_PARAMETER(float, NormalStrength)
		SHADER_PARAMETER(float, RoughnessBase)
		SHADER_PARAMETER(float, RoughnessContrast)
		SHADER_PARAMETER(uint32, bRoughnessInvert)
		SHADER_PARAMETER(float, MetallicBase)
		SHADER_PARAMETER(float, MetallicContrast)
		SHADER_PARAMETER(float, MetallicBias)
		SHADER_PARAMETER(float, MetallicSensitivity)
		SHADER_PARAMETER(float, AOIntensity)
		SHADER_PARAMETER(float, HeightContrast)
		SHADER_PARAMETER(float, BioDetail)
		SHADER_PARAMETER(float, BioFrequency)
		SHADER_PARAMETER(float, CyberDetail)
		SHADER_PARAMETER(float, CyberScale)
		SHADER_PARAMETER(float, EdgeWear)
		SHADER_PARAMETER(float, CavityDirt)
		SHADER_PARAMETER(float, Dust)
		SHADER_PARAMETER(float, Grunge)
		SHADER_PARAMETER(float, Scratches)
		SHADER_PARAMETER(float, Noise)
		SHADER_PARAMETER(float, EmissiveThreshold)
		SHADER_PARAMETER(float, EmissiveColorBoost)
		SHADER_PARAMETER(float, VarianceWeight)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
		SHADER_PARAMETER(uint32, bAdvancedNormal)
		SHADER_PARAMETER(uint32, bAdvancedAO)
		SHADER_PARAMETER(int32, NormalOctaves)
		SHADER_PARAMETER(float, NormalSigmaBase)
		SHADER_PARAMETER(float, NormalAnisotropy)
		SHADER_PARAMETER(float, AORadius)
		SHADER_PARAMETER(float, AOBias)
		SHADER_PARAMETER(float, AOContrast)
	END_SHADER_PARAMETER_STRUCT()
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKPBRGeneratorCS, "/Plugin/Materialize/PBRGenerator.usf", "MainCS", SF_Compute);

// --- Seamless Tiling ---
class FKSeamlessCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKSeamlessCS);
	SHADER_USE_PARAMETER_STRUCT(FKSeamlessCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InSource)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutSeamless)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
		SHADER_PARAMETER(float, BlendWidth)
		SHADER_PARAMETER(uint32, TileMode)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKSeamlessCS, "/Plugin/Materialize/SeamlessAndPacking.usf", "SeamlessCS", SF_Compute);

// --- ORM Packing ---
class FKPackORMCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKPackORMCS);
	SHADER_USE_PARAMETER_STRUCT(FKPackORMCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InAO)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InRoughness)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InMetallic)
		SHADER_PARAMETER_SAMPLER(SamplerState, InSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutORM)
		SHADER_PARAMETER(FUintVector2, TextureDimensions)
		// MATCHING SeamlessAndPacking.usf cbuffer FParameters (Shared with SeamlessCS)
		SHADER_PARAMETER(float, BlendWidth) 
		SHADER_PARAMETER(uint32, TileMode)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	}
};

IMPLEMENT_GLOBAL_SHADER(FKPackORMCS, "/Plugin/Materialize/SeamlessAndPacking.usf", "PackORMCS", SF_Compute);

// =============================================================================
// ENGINE IMPLEMENTATION
// =============================================================================

bool UMaterializeComputeEngine::GeneratePBRMapsGPU(UTexture2D* SourceTexture, const FMaterializeParams& Params, FMaterializeResult& OutResult)
{
	// [DEBUG] FALLBACK: If Source isn't ready or valid, use Engine White Texture to test pipeline stability
	UTexture2D* SafeSource = SourceTexture;
	if (!SourceTexture || !SourceTexture->GetResource())
	{
		SafeSource = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
		if (!SafeSource) return false; // Should never happen
		UE_LOG(LogTemp, Warning, TEXT("Materialize: Using WhiteSquareTexture as fallback input"));
	}
	else
	{
		SafeSource->WaitForStreaming();
	}
	
	FlushRenderingCommands();
	
	FTextureResource* SourceResource = SafeSource->GetResource();
	if (!SourceResource || !SourceResource->GetTexture2DRHI())
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: No valid RHI for source"));
		return false;
	}

	double StartTime = FPlatformTime::Seconds();

	int32 Width = SafeSource->GetSizeX();
	int32 Height = SafeSource->GetSizeY();

	if (Width <= 0 || Height <= 0) return false;

	FIntPoint Size(Width, Height);
	FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8));
	int32 HeightIterations = FMath::Clamp(Params.HeightIterations, 4, 64);
	bool bUseMultiPass = Params.bUseMultiPassHeight;

	// --- SMART TEXTURE REUSE ---
	// Instead of thrashing VRAM with CreateTransient every frame, reuse existing textures if valid.
	// NOTE: FMaterializeResult uses raw UTexture2D*, so lambda must match.
	auto GetOrResize = [&](TObjectPtr<UTexture2D>& Tex, EPixelFormat Format, bool bSRGB) {
		if (Tex && Tex->GetSizeX() == Width && Tex->GetSizeY() == Height && Tex->GetPixelFormat() == Format)
		{
			return; // Reuse
		}
		Tex = UTexture2D::CreateTransient(Width, Height, Format);
		Tex->SRGB = bSRGB;
		Tex->UpdateResource();
	};

	// NOTE: D3D12 UAVs do not support PF_B8G8R8A8 (BGRA). Must use PF_R8G8B8A8 (RGBA).
	GetOrResize(OutResult.Normal, PF_R8G8B8A8, false);
	// NOTE: Using PF_R32_FLOAT to match RWTexture2D<float> in shader and avoid D3D12 mismatch
	GetOrResize(OutResult.Roughness, PF_R32_FLOAT, false); 
	GetOrResize(OutResult.Metallic, PF_R32_FLOAT, false);
	GetOrResize(OutResult.AO, PF_R32_FLOAT, false);
	GetOrResize(OutResult.Height, PF_R32_FLOAT, false);
	GetOrResize(OutResult.Emissive, PF_R32_FLOAT, false);
	
	if (Params.bPackORM) 
	{
		GetOrResize(OutResult.ORM, PF_R8G8B8A8, false);
	}

	// Ensure all new/reused resources are ready
	FlushRenderingCommands();

	// Capture RHIs
	FRHITexture* SourceRHI = SourceResource->GetTexture2DRHI();
	
	// Helper to get RHI for output
	auto GetTargetRHI = [](UTexture2D* Tex) -> FRHITexture* {
		return (Tex && Tex->GetResource()) ? Tex->GetResource()->GetTexture2DRHI() : nullptr;
	};

	FRHITexture* NormalRHI = GetTargetRHI(OutResult.Normal);
	FRHITexture* RoughRHI = GetTargetRHI(OutResult.Roughness);
	FRHITexture* MetalRHI = GetTargetRHI(OutResult.Metallic);
	FRHITexture* AORHI = GetTargetRHI(OutResult.AO);
	FRHITexture* HeightRHI = GetTargetRHI(OutResult.Height);
	FRHITexture* EmissiveRHI = GetTargetRHI(OutResult.Emissive);
	FRHITexture* ORMRHI = GetTargetRHI(OutResult.ORM);

	ENQUEUE_RENDER_COMMAND(KSampleGenGPU_MultiPass)(
		[SourceRHI, NormalRHI, RoughRHI, MetalRHI, AORHI, HeightRHI, EmissiveRHI, ORMRHI, Params, Width, Height, Size, GroupCount, HeightIterations, bUseMultiPass](FRHICommandListImmediate& RHICmdList)
		{
			if (!SourceRHI || !SourceRHI->GetNativeResource()) return;

			// Transitions for SOURCE (SRV)
			RHICmdList.Transition(FRHITransitionInfo(SourceRHI, ERHIAccess::Unknown, ERHIAccess::SRVCompute));

			FRDGBuilder ComputeGraphBuilder(RHICmdList);
			
			// Register Input directly (RDG handles external registration safely usually, but we check)
			FRDGTextureRef  ComputeInputRDG = ComputeGraphBuilder.RegisterExternalTexture(CreateRenderTarget(SourceRHI, TEXT("SourceInput")));
			FRDGTextureSRVRef InputSRV = ComputeGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(ComputeInputRDG));

			// --- INTERMEDIATE BUFFERS (Pure RDG) ---
			// PF_G32R32F for Gradient (float2)
			FRDGTextureDesc GradientDesc = FRDGTextureDesc::Create2D(Size, PF_G32R32F, FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef GradientRDG = ComputeGraphBuilder.CreateTexture(GradientDesc, TEXT("GradientMap"));

			// PF_R32_FLOAT for Height (float)
			FRDGTextureDesc HeightDesc = FRDGTextureDesc::Create2D(Size, PF_R32_FLOAT, FClearValueBinding::Transparent, TexCreate_UAV | TexCreate_ShaderResource);
			FRDGTextureRef HeightPingRDG = ComputeGraphBuilder.CreateTexture(HeightDesc, TEXT("HeightPing"));
			FRDGTextureRef HeightPongRDG = ComputeGraphBuilder.CreateTexture(HeightDesc, TEXT("HeightPong"));

			// Helper for Output RDG creation
			auto CreateOutputRDG = [&](const TCHAR* Name, EPixelFormat Format) {
				FRDGTextureDesc OutDesc = FRDGTextureDesc::Create2D(Size, Format, FClearValueBinding::Black, TexCreate_UAV | TexCreate_ShaderResource | TexCreate_RenderTargetable);
				return ComputeGraphBuilder.CreateTexture(OutDesc, Name);
			};

			FRDGTextureRef NormalRDG = CreateOutputRDG(TEXT("OutNormal"), PF_R8G8B8A8);
			FRDGTextureRef RoughRDG = CreateOutputRDG(TEXT("OutRough"), PF_R32_FLOAT);
			FRDGTextureRef MetalRDG = CreateOutputRDG(TEXT("OutMetal"), PF_R32_FLOAT);
			FRDGTextureRef AORDG = CreateOutputRDG(TEXT("OutAO"), PF_R32_FLOAT);
			FRDGTextureRef HeightOutRDG = CreateOutputRDG(TEXT("OutHeight"), PF_R32_FLOAT);
			FRDGTextureRef EmissiveRDG = CreateOutputRDG(TEXT("OutEmissive"), PF_R32_FLOAT);

			// --- PASS 1: Gradient Extraction ---
			{
				TShaderMapRef<FKGradientCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				FKGradientCS::FParameters* PassParams = ComputeGraphBuilder.AllocParameters<FKGradientCS::FParameters>();
				PassParams->InSourceTexture = InputSRV;
				PassParams->InSourceSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParams->OutGradient = ComputeGraphBuilder.CreateUAV(GradientRDG);
				PassParams->NormalStrength = Params.NormalStrength;
				PassParams->TextureDimensions = FUintVector2(Width, Height);
				PassParams->bAdvancedNormal = Params.bAdvancedNormal ? 1u : 0u;
				PassParams->bAdvancedAO = Params.bAdvancedAO ? 1u : 0u;
				PassParams->NormalOctaves = Params.NormalOctaves;
				PassParams->NormalSigmaBase = Params.NormalSigmaBase;
				PassParams->NormalAnisotropy = Params.NormalAnisotropy;
				PassParams->AORadius = Params.AORadius;
				PassParams->AOBias = Params.AOBias;
				PassParams->AOContrast = Params.AOContrast;

				// Initialize Padding for PBRGenerator.usf shared cbuffer
				PassParams->RoughnessBase = 0.0f; PassParams->RoughnessContrast = 0.0f; PassParams->bRoughnessInvert = 0;
				PassParams->MetallicBase = 0.0f; PassParams->MetallicContrast = 0.0f; PassParams->MetallicBias = 0.0f;
				PassParams->MetallicSensitivity = 0.0f;
				PassParams->AOIntensity = 0.0f; PassParams->HeightContrast = 0.0f; PassParams->BioDetail = 0.0f;
				PassParams->BioFrequency = 0.0f; PassParams->CyberDetail = 0.0f; PassParams->CyberScale = 0.0f;
				PassParams->EdgeWear = 0.0f; PassParams->CavityDirt = 0.0f; PassParams->Dust = 0.0f;
				PassParams->Grunge = 0.0f; PassParams->Scratches = 0.0f; PassParams->Noise = 0.0f;
				PassParams->EmissiveThreshold = 0.0f; PassParams->EmissiveColorBoost = 0.0f; PassParams->VarianceWeight = 0.0f;
				
				FComputeShaderUtils::AddPass(ComputeGraphBuilder, RDG_EVENT_NAME("KSample_Gradient"), Shader, PassParams, GroupCount);
			}

			// --- PASS 2: Height Integration ---
			if (bUseMultiPass)
			{
				AddClearUAVPass(ComputeGraphBuilder, ComputeGraphBuilder.CreateUAV(HeightPingRDG), 0.5f);
				
				for(int32 i = 0; i < HeightIterations; i++)
				{
					TShaderMapRef<FKHeightIntegrationCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
					FKHeightIntegrationCS::FParameters* PassParams = ComputeGraphBuilder.AllocParameters<FKHeightIntegrationCS::FParameters>();
					PassParams->InGradient = ComputeGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(GradientRDG));
					PassParams->InHeightPrev = ComputeGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create( (i%2==0) ? HeightPingRDG : HeightPongRDG ));
					PassParams->OutHeightNext = ComputeGraphBuilder.CreateUAV( (i%2==0) ? HeightPongRDG : HeightPingRDG );
				PassParams->TextureDimensions = FUintVector2(Width, Height);

				// Initialize Padding for PBRGenerator.usf shared cbuffer
				PassParams->NormalStrength = Params.NormalStrength;
				PassParams->bAdvancedNormal = Params.bAdvancedNormal ? 1u : 0u;
				PassParams->bAdvancedAO = Params.bAdvancedAO ? 1u : 0u;
				PassParams->NormalOctaves = Params.NormalOctaves;
				PassParams->NormalSigmaBase = Params.NormalSigmaBase;
				PassParams->NormalAnisotropy = Params.NormalAnisotropy;
				PassParams->AORadius = Params.AORadius;
				PassParams->AOBias = Params.AOBias;
				PassParams->AOContrast = Params.AOContrast;
				PassParams->RoughnessBase = 0.0f; PassParams->RoughnessContrast = 0.0f; PassParams->bRoughnessInvert = 0;
					PassParams->MetallicBase = 0.0f; PassParams->MetallicContrast = 0.0f; PassParams->MetallicBias = 0.0f;
					PassParams->MetallicSensitivity = 0.0f;
					PassParams->AOIntensity = 0.0f; PassParams->HeightContrast = 1.0f; PassParams->BioDetail = 0.0f;
					PassParams->BioFrequency = 0.0f; PassParams->CyberDetail = 0.0f; PassParams->CyberScale = 0.0f;
					PassParams->EdgeWear = 0.0f; PassParams->CavityDirt = 0.0f; PassParams->Dust = 0.0f;
					PassParams->Grunge = 0.0f; PassParams->Scratches = 0.0f; PassParams->Noise = 0.0f;
					PassParams->EmissiveThreshold = 0.0f; PassParams->EmissiveColorBoost = 0.0f; PassParams->VarianceWeight = 0.0f;
					
					FComputeShaderUtils::AddPass(ComputeGraphBuilder, RDG_EVENT_NAME("KSample_HeightIter_%d", i), Shader, PassParams, GroupCount);
				}
			}

			// --- PASS 3: Final PBR Generation ---
			{
				TShaderMapRef<FKFinalPBRCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				FKFinalPBRCS::FParameters* PassParams = ComputeGraphBuilder.AllocParameters<FKFinalPBRCS::FParameters>();
				PassParams->InSourceTexture = InputSRV;
				PassParams->InSourceSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				// Fix Ping-Pong Logic: If 24 iterations, last write is to PING. Read PING.
				// (HeightIterations % 2 == 0) -> Read Ping.
				FRDGTextureRef InputHeight = (HeightIterations % 2 == 0) ? HeightPingRDG : HeightPongRDG;
				PassParams->InHeightPrev = ComputeGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputHeight));
				
				PassParams->OutNormal = ComputeGraphBuilder.CreateUAV(NormalRDG);
				PassParams->OutRoughness = ComputeGraphBuilder.CreateUAV(RoughRDG);
				PassParams->OutMetallic = ComputeGraphBuilder.CreateUAV(MetalRDG);
				PassParams->OutAO = ComputeGraphBuilder.CreateUAV(AORDG);
				PassParams->OutHeight = ComputeGraphBuilder.CreateUAV(HeightOutRDG);
				PassParams->OutEmissive = ComputeGraphBuilder.CreateUAV(EmissiveRDG);
				
				PassParams->NormalStrength = Params.NormalStrength;
				PassParams->RoughnessBase = Params.RoughnessBase; // FIXED NAME
				PassParams->RoughnessContrast = Params.RoughnessContrast;
				PassParams->bRoughnessInvert = Params.bRoughnessInvert ? 1 : 0;
				PassParams->MetallicBase = Params.MetallicBase;
				PassParams->MetallicContrast = Params.MetallicContrast;
				PassParams->MetallicBias = Params.MetallicBias;
				PassParams->MetallicSensitivity = Params.MetallicSensitivity;
				PassParams->AOIntensity = Params.AOIntensity;
				PassParams->HeightContrast = Params.HeightContrast;
				
				PassParams->BioDetail = Params.BioDetail;
				PassParams->BioFrequency = Params.BioFrequency;
				PassParams->CyberDetail = Params.CyberDetail;
				PassParams->CyberScale = Params.CyberScale;
				
				PassParams->EdgeWear = Params.EdgeWear;
				PassParams->CavityDirt = Params.CavityDirt;
				PassParams->Dust = Params.Dust;
				PassParams->Grunge = Params.Grunge;
				PassParams->Scratches = Params.Scratches;
				PassParams->Noise = Params.Noise;

				PassParams->EmissiveThreshold = Params.EmissiveThreshold;
				PassParams->EmissiveColorBoost = Params.EmissiveColorBoost;
				PassParams->VarianceWeight = Params.VarianceWeight;
				PassParams->TextureDimensions = FUintVector2(Width, Height);
				PassParams->bAdvancedNormal = Params.bAdvancedNormal ? 1u : 0u;
				PassParams->bAdvancedAO = Params.bAdvancedAO ? 1u : 0u;
				PassParams->NormalOctaves = Params.NormalOctaves;
				PassParams->NormalSigmaBase = Params.NormalSigmaBase;
				PassParams->NormalAnisotropy = Params.NormalAnisotropy;
				PassParams->AORadius = Params.AORadius;
				PassParams->AOBias = Params.AOBias;
				PassParams->AOContrast = Params.AOContrast;

				// [DEBUG] Log parameters to ensure no garbage values
				UE_LOG(LogTemp, Log, TEXT("Materialize GPU Dispatch: Dim:%dx%d, Norm:%f, Bio:%f, Edge:%f, Emis:%f"),
					Width, Height, Params.NormalStrength, Params.BioDetail, Params.EdgeWear, Params.EmissiveThreshold);
				
				FComputeShaderUtils::AddPass(ComputeGraphBuilder, RDG_EVENT_NAME("KSample_FinalPBR"), Shader, PassParams, GroupCount);
			}

			// --- OPTIONAL: Seamless Tiling ---
			FRDGTextureRef FinalNormal = NormalRDG;
			FRDGTextureRef FinalRough = RoughRDG;
			FRDGTextureRef FinalMetal = MetalRDG;
			FRDGTextureRef FinalAO = AORDG;
			FRDGTextureRef FinalHeight = HeightOutRDG;
			FRDGTextureRef FinalEmissive = EmissiveRDG;

			if (Params.bMakeSeamless)
			{
				auto MakeSeamlessPass = [&](FRDGTextureRef Input, const TCHAR* Name) -> FRDGTextureRef
				{
					FRDGTextureRef Output = CreateOutputRDG(Name, Input->Desc.Format);
					TShaderMapRef<FKSeamlessCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
					FKSeamlessCS::FParameters* PassParams = ComputeGraphBuilder.AllocParameters<FKSeamlessCS::FParameters>();
					PassParams->InSource = ComputeGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(Input));
					PassParams->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
					PassParams->OutSeamless = ComputeGraphBuilder.CreateUAV(Output);
					PassParams->TextureDimensions = FUintVector2(Width, Height);
					PassParams->BlendWidth = Params.SeamlessBlendWidth;
					
					uint32 ModeInt = 0;
					if (Params.SeamlessMode == EKSeamlessMode::MirrorBlend) ModeInt = 1;
					else if (Params.SeamlessMode == EKSeamlessMode::Histogram) ModeInt = 2;
					PassParams->TileMode = ModeInt;

					FComputeShaderUtils::AddPass(ComputeGraphBuilder, RDG_EVENT_NAME("KSample_Seamless"), Shader, PassParams, GroupCount);
					return Output;
				};

				FinalNormal = MakeSeamlessPass(NormalRDG, TEXT("SeamlessNormal"));
				FinalRough = MakeSeamlessPass(RoughRDG, TEXT("SeamlessRoughness"));
				FinalMetal = MakeSeamlessPass(MetalRDG, TEXT("SeamlessMetallic"));
				FinalAO = MakeSeamlessPass(AORDG, TEXT("SeamlessAO"));
				FinalHeight = MakeSeamlessPass(HeightOutRDG, TEXT("SeamlessHeight"));
				FinalEmissive = MakeSeamlessPass(EmissiveRDG, TEXT("SeamlessEmissive"));
			}

			// --- OPTIONAL: ORM Packing ---
			FRDGTextureRef ORMRDG = nullptr;
			if (Params.bPackORM && ORMRHI)
			{
				ORMRDG = CreateOutputRDG(TEXT("ORMMap"), PF_R8G8B8A8);
				TShaderMapRef<FKPackORMCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				FKPackORMCS::FParameters* PassParams = ComputeGraphBuilder.AllocParameters<FKPackORMCS::FParameters>();
				PassParams->InAO = ComputeGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(FinalAO));
				PassParams->InRoughness = ComputeGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(FinalRough));
				PassParams->InMetallic = ComputeGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(FinalMetal));
				PassParams->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				PassParams->OutORM = ComputeGraphBuilder.CreateUAV(ORMRDG);
				PassParams->TextureDimensions = FUintVector2(Width, Height);
				PassParams->BlendWidth = 0.0f; // Padding
				PassParams->TileMode = 0;      // Padding

				FComputeShaderUtils::AddPass(ComputeGraphBuilder, RDG_EVENT_NAME("KSample_PackORM"), Shader, PassParams, GroupCount);
			}

			// --- EXTRACTION QUEUE ---
			TRefCountPtr<IPooledRenderTarget> ExtNormal, ExtRough, ExtMetal, ExtAO, ExtHeight, ExtEmissive, ExtORM;
			
			ComputeGraphBuilder.QueueTextureExtraction(FinalNormal, &ExtNormal);
			ComputeGraphBuilder.QueueTextureExtraction(FinalRough, &ExtRough);
			ComputeGraphBuilder.QueueTextureExtraction(FinalMetal, &ExtMetal);
			ComputeGraphBuilder.QueueTextureExtraction(FinalAO, &ExtAO);
			ComputeGraphBuilder.QueueTextureExtraction(FinalHeight, &ExtHeight);
			ComputeGraphBuilder.QueueTextureExtraction(FinalEmissive, &ExtEmissive);
			if (ORMRDG) ComputeGraphBuilder.QueueTextureExtraction(ORMRDG, &ExtORM);

			// EXECUTE GRAPH
			ComputeGraphBuilder.Execute();

			// --- SAFE COPY TO EXTERNAL ---
			auto SafeCopy = [&](TRefCountPtr<IPooledRenderTarget> Src, FRHITexture* Dst) {
				if (Src.IsValid() && Dst && Dst->GetNativeResource()) // CRITICAL validity check
				{
					RHICmdList.Transition(FRHITransitionInfo(Dst, ERHIAccess::Unknown, ERHIAccess::CopyDest));
					FRHICopyTextureInfo CopyInfo;
					RHICmdList.CopyTexture(Src->GetRHI(), Dst, CopyInfo);
					RHICmdList.Transition(FRHITransitionInfo(Dst, ERHIAccess::CopyDest, ERHIAccess::SRVGraphics)); // Ready for UI
				}
			};

			SafeCopy(ExtNormal, NormalRHI);
			SafeCopy(ExtRough, RoughRHI);
			SafeCopy(ExtMetal, MetalRHI);
			SafeCopy(ExtAO, AORHI);
			SafeCopy(ExtHeight, HeightRHI);
			SafeCopy(ExtEmissive, EmissiveRHI);
			if (ORMRDG) SafeCopy(ExtORM, ORMRHI);
		}
	);

	FlushRenderingCommands(); // Ensure completion before UI access

	double EndTime = FPlatformTime::Seconds();
	OutResult.GenerationTimeMs = (EndTime - StartTime) * 1000.0f;

	return true;
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

UTexture2D* UMaterializeComputeEngine::MakeSeamless(UTexture2D* SourceTexture, EKSeamlessMode Mode, float BlendWidth)
{
	if (!SourceTexture || !SourceTexture->GetResource()) return nullptr;

	int32 Width = SourceTexture->GetSizeX();
	int32 Height = SourceTexture->GetSizeY();

	UTexture2D* Output = UTexture2D::CreateTransient(Width, Height, SourceTexture->GetPixelFormat());
	Output->SRGB = SourceTexture->SRGB;
	Output->UpdateResource();

	// CRITICAL: wait for transient texture GPU init before capturing RHI refs
	FlushRenderingCommands();

	// Capture RHI reference safely
	FRHITexture* SourceRHI = SourceTexture->GetResource()->GetTexture2DRHI();
	FString ValidationError;
	if (!ValidateRHIResource(SourceRHI, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: MakeSeamless source validation failed: %s"), *ValidationError);
		return nullptr;
	}

	// Capture output RHI
	FRHITexture* DestRHI = Output->GetResource()->GetTexture2DRHI();
	if (!ValidateRHIResource(DestRHI, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: MakeSeamless output validation failed: %s"), *ValidationError);
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(KSampleSeamless)(
		[SourceRHI, DestRHI, Width, Height, Mode, BlendWidth](FRHICommandListImmediate& RHICmdList)
		{
			if (!SourceRHI || !DestRHI) return;

			// Use RAII wrapper for automatic RDG execution
			FMaterializeRDGScope RDGScope(RHICmdList);
			FRDGBuilder& GraphBuilder = RDGScope.GetGraphBuilder();

			FRDGTextureRef InputRDG = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(SourceRHI, TEXT("SeamlessInput")));
			FRDGTextureSRVRef InputSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputRDG));

			FRDGTextureDesc OutDesc = FRDGTextureDesc::Create2D(
				FIntPoint(Width, Height),
				SourceRHI->GetFormat(), // Use source format from RHI
				FClearValueBinding::Transparent,
				TexCreate_UAV | TexCreate_ShaderResource
			);
			FRDGTextureRef OutputRDG = GraphBuilder.CreateTexture(OutDesc, TEXT("SeamlessOutput"));
			
			TShaderMapRef<FKSeamlessCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FKSeamlessCS::FParameters* Params = GraphBuilder.AllocParameters<FKSeamlessCS::FParameters>();

			Params->InSource = InputSRV;
			Params->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			Params->OutSeamless = GraphBuilder.CreateUAV(OutputRDG);
			Params->TextureDimensions = FUintVector2(Width, Height);
			// Mode 0=CrossBlend, 1=MirrorBlend, 2=Histogram
			uint32 ModeInt = 0;
			if (Mode == EKSeamlessMode::MirrorBlend) ModeInt = 1;
			else if (Mode == EKSeamlessMode::Histogram) ModeInt = 2;
			
			Params->TileMode = ModeInt;
			Params->BlendWidth = BlendWidth;

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("KSample_Seamless"),
				Shader,
				Params,
				FIntVector(FMath::DivideAndRoundUp(Width, 8), FMath::DivideAndRoundUp(Height, 8), 1)
			);

			AddCopyTexturePass(GraphBuilder, OutputRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DestRHI, TEXT("Dest"))),
				FRHICopyTextureInfo());

			// RDGScope destructor will automatically call GraphBuilder.Execute()
		}
	);

	FlushRenderingCommands();
	Output->UpdateResource();
	return Output;
}

UTexture2D* UMaterializeComputeEngine::PackORM(UTexture2D* AO, UTexture2D* Roughness, UTexture2D* Metallic)
{
	if (!AO || !Roughness || !Metallic) return nullptr;
	if (!AO->GetResource() || !Roughness->GetResource() || !Metallic->GetResource()) return nullptr;

	int32 Width = AO->GetSizeX();
	int32 Height = AO->GetSizeY();

	UTexture2D* Output = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	Output->SRGB = false;
	Output->UpdateResource();

	// CRITICAL: wait for transient texture GPU init before capturing RHI refs
	FlushRenderingCommands();

	// Capture RHI references with validation
	FRHITexture* AORHI = AO->GetResource()->GetTexture2DRHI();
	FRHITexture* RoughRHI = Roughness->GetResource()->GetTexture2DRHI();
	FRHITexture* MetalRHI = Metallic->GetResource()->GetTexture2DRHI();

	FString ValidationError;
	if (!ValidateRHIResource(AORHI, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: PackORM AO validation failed: %s"), *ValidationError);
		return nullptr;
	}
	if (!ValidateRHIResource(RoughRHI, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: PackORM Roughness validation failed: %s"), *ValidationError);
		return nullptr;
	}
	if (!ValidateRHIResource(MetalRHI, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: PackORM Metallic validation failed: %s"), *ValidationError);
		return nullptr;
	}

	// Capture Output RHI
	FRHITexture* DestRHI = Output->GetResource()->GetTexture2DRHI();
	if (!ValidateRHIResource(DestRHI, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize: PackORM output validation failed: %s"), *ValidationError);
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(KSamplePackORM)(
		[AORHI, RoughRHI, MetalRHI, DestRHI, Width, Height](FRHICommandListImmediate& RHICmdList)
		{
			if (!AORHI || !RoughRHI || !MetalRHI || !DestRHI) return;

			// Use RAII wrapper for automatic RDG execution
			FMaterializeRDGScope RDGScope(RHICmdList);
			FRDGBuilder& GraphBuilder = RDGScope.GetGraphBuilder();

			auto Reg = [&](FRHITexture* TexRHI, const TCHAR* Name) {
				return GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(
					GraphBuilder.RegisterExternalTexture(CreateRenderTarget(TexRHI, Name))));
			};

			FRDGTextureDesc OutDesc = FRDGTextureDesc::Create2D(
				FIntPoint(Width, Height),
				PF_B8G8R8A8,
				FClearValueBinding::Transparent,
				TexCreate_UAV | TexCreate_ShaderResource
			);
			FRDGTextureRef OutputRDG = GraphBuilder.CreateTexture(OutDesc, TEXT("ORMOutput"));

			TShaderMapRef<FKPackORMCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FKPackORMCS::FParameters* Params = GraphBuilder.AllocParameters<FKPackORMCS::FParameters>();

			Params->InAO = Reg(AORHI, TEXT("InAO"));
			Params->InRoughness = Reg(RoughRHI, TEXT("InRough"));
			Params->InMetallic = Reg(MetalRHI, TEXT("InMetal"));
			Params->InSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			Params->OutORM = GraphBuilder.CreateUAV(OutputRDG);
			Params->TextureDimensions = FUintVector2(Width, Height);
			// Initialize Padding from SeamlessAndPacking.usf shared cbuffer
			Params->BlendWidth = 0.0f;
			Params->TileMode = 0;

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("KSample_PackORM"),
				Shader,
				Params,
				FIntVector(FMath::DivideAndRoundUp(Width, 8), FMath::DivideAndRoundUp(Height, 8), 1)
			);

			AddCopyTexturePass(GraphBuilder, OutputRDG,
				GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DestRHI, TEXT("Dest"))),
				FRHICopyTextureInfo());

			// RDGScope destructor will automatically call GraphBuilder.Execute()
		}
	);

	FlushRenderingCommands();
	Output->UpdateResource();
	return Output;
}

bool UMaterializeComputeEngine::ReadbackTexture(UTexture2D* Texture, TArray<FColor>& OutPixels)
{
	if (!Texture || !Texture->GetResource()) return false;
	FRHITexture* TextureRHI = Texture->GetResource()->GetTexture2DRHI();
	if (!TextureRHI) return false;

	struct FReadbackContext
	{
		FRHITexture* TexRHI;
		TArray<FColor>* OutPixels;
		FIntPoint Size;
		bool bSuccess = false;
	} Context;

	Context.TexRHI = TextureRHI;
	Context.OutPixels = &OutPixels;
	Context.Size = FIntPoint(Texture->GetSizeX(), Texture->GetSizeY());

	ENQUEUE_RENDER_COMMAND(KSampleReadback)(
		[&Context](FRHICommandListImmediate& RHICmdList)
		{
			if (Context.TexRHI)
			{
				EPixelFormat Format = Context.TexRHI->GetFormat();
				if (Format == PF_R16F || Format == PF_R32_FLOAT)
				{
					TArray<FLinearColor> LinearPixels;
					RHICmdList.ReadSurfaceData(Context.TexRHI, FIntRect(0, 0, Context.Size.X, Context.Size.Y), LinearPixels, FReadSurfaceDataFlags());
					Context.OutPixels->SetNumUninitialized(LinearPixels.Num());
					// Force Grayscale for single channel formats to avoid Red/Yellow textures
					for (int32 i = 0; i < LinearPixels.Num(); ++i)
					{
						float Val = LinearPixels[i].R;
						// Replicate R to G and B
						// Usually these maps are Linear, but FColor is 8bit. ToFColor(false) means linear->linear 8bit.
						// Use ToFColor(false) because we want raw data values (0-1 mapped to 0-255 linear).
						(*Context.OutPixels)[i] = FLinearColor(Val, Val, Val, 1.0f).ToFColor(false);
					}
				}
				else
				{
					RHICmdList.ReadSurfaceData(Context.TexRHI, FIntRect(0, 0, Context.Size.X, Context.Size.Y), *Context.OutPixels, FReadSurfaceDataFlags());
				}
				Context.bSuccess = true;
			}
		}
	);

	FlushRenderingCommands();
	return Context.bSuccess;
}

void UMaterializeComputeEngine::ReadbackResult(const FMaterializeResult& Result, TMap<FString, TArray<FColor>>& OutMap)
{
	auto DoRead = [&](UTexture2D* Tex, const FString& Name) {
		if (Tex) ReadbackTexture(Tex, OutMap.FindOrAdd(Name));
	};

	DoRead(Result.Normal, TEXT("Normal"));
	DoRead(Result.Roughness, TEXT("Roughness"));
	DoRead(Result.Metallic, TEXT("Metallic"));
	DoRead(Result.AO, TEXT("AO"));
	DoRead(Result.Height, TEXT("Height"));
	DoRead(Result.Emissive, TEXT("Emissive"));
	DoRead(Result.ORM, TEXT("ORM"));
}

// =============================================================================
// RESOURCE MANAGEMENT
// =============================================================================

void UMaterializeComputeEngine::CleanupTransientResources(FMaterializeResult& Result)
{
	// Clear transient texture references
	// Note: UTexture2D::CreateTransient creates textures that are automatically
	// garbage collected, but we can explicitly clear references to help GC
	
	auto ClearTexture = [](TObjectPtr<UTexture2D>& Tex) {
		if (Tex && Tex->IsValidLowLevel())
		{
			// Mark for pending kill if it's a transient texture
			if (Tex->HasAnyFlags(RF_Transient))
			{
				Tex->ConditionalBeginDestroy();
			}
			Tex = nullptr;
		}
	};

	ClearTexture(Result.Normal);
	ClearTexture(Result.Roughness);
	ClearTexture(Result.Metallic);
	ClearTexture(Result.AO);
	ClearTexture(Result.Height);
	ClearTexture(Result.Emissive);
	ClearTexture(Result.ORM);
}

bool UMaterializeComputeEngine::ValidateRHIResource(FRHITexture* TextureRHI, FString& OutError)
{
	if (!TextureRHI)
	{
		OutError = TEXT("RHI texture reference is invalid");
		return false;
	}

	if (!TextureRHI->GetNativeResource())
	{
		OutError = TEXT("RHI texture has no native resource");
		return false;
	}

	// Check if texture dimensions are valid
	FIntPoint Size = TextureRHI->GetSizeXY();
	if (Size.X <= 0 || Size.Y <= 0)
	{
		OutError = FString::Printf(TEXT("RHI texture has invalid dimensions: %dx%d"), Size.X, Size.Y);
		return false;
	}

	return true;
}
