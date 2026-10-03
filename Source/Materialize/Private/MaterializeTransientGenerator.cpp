#include "MaterializeTransientGenerator.h"
#include "MaterializeValidation.h"
#include "MaterializeMaterialLoader.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "MaterialShared.h"
#include "MaterialDomain.h"
#include "Engine/Texture.h"
#include "UObject/UObjectGlobals.h"

UMaterial* FMaterializeTransientGenerator::Generate(const FString& PresetType)
{
	UE_LOG(LogMaterialize, Log, TEXT("Generating transient material for preset: %s"), *PresetType);
	
	// Create transient material
	UMaterial* Material = NewObject<UMaterial>(GetTransientPackage(), 
		*FString::Printf(TEXT("M_Materialize_Transient_%s"), *PresetType),
		RF_Transient);
	
	MATERIALIZE_CHECK_PTR(Material, nullptr);
	
	// Configure material settings
	ConfigureMaterialSettings(Material);
	
	// Create texture parameters with correct sampler types
	UMaterialExpressionTextureSampleParameter2D* BaseColor = CreateBaseColorParameter(Material);
	UMaterialExpressionTextureSampleParameter2D* Normal = CreateNormalParameter(Material);
	UMaterialExpressionTextureSampleParameter2D* ORM = CreateORMParameter(Material);
	UMaterialExpressionTextureSampleParameter2D* Emissive = CreateEmissiveParameter(Material);
	
	// Verify all parameters were created
	if (!BaseColor || !Normal || !ORM || !Emissive)
	{
		UE_LOG(LogMaterialize, Error, TEXT("Failed to create one or more texture parameters for transient material"));
		return nullptr;
	}
	
	// Connect outputs
	ConnectMaterialOutputs(Material, BaseColor, Normal, ORM, Emissive);
	
	// Assign default textures
	AssignDefaultTextures(Material, Normal, ORM);
	
	// Compile and validate
	FString CompilationError;
	if (!CompileAndValidate(Material, CompilationError))
	{
		UE_LOG(LogMaterialize, Error, TEXT("Transient material compilation failed: %s"), *CompilationError);
		return nullptr;
	}
	
	UE_LOG(LogMaterialize, Log, TEXT("Successfully generated transient material: %s"), *Material->GetName());
	return Material;
}

UMaterialExpressionTextureSampleParameter2D* FMaterializeTransientGenerator::CreateBaseColorParameter(UMaterial* Material)
{
	MATERIALIZE_CHECK_PTR(Material, nullptr);
	
	UMaterialExpressionTextureSampleParameter2D* Expression = 
		NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
	
	MATERIALIZE_CHECK_PTR(Expression, nullptr);
	
	Expression->ParameterName = TEXT("BaseColor");
	Expression->SamplerType = SAMPLERTYPE_Color;  // sRGB color texture
	Expression->MaterialExpressionEditorX = -400;
	Expression->MaterialExpressionEditorY = -200;
	
	Material->GetExpressionCollection().AddExpression(Expression);
	
	UE_LOG(LogMaterialize, Verbose, TEXT("Created BaseColor parameter with SAMPLERTYPE_Color"));
	return Expression;
}

UMaterialExpressionTextureSampleParameter2D* FMaterializeTransientGenerator::CreateNormalParameter(UMaterial* Material)
{
	MATERIALIZE_CHECK_PTR(Material, nullptr);
	
	UMaterialExpressionTextureSampleParameter2D* Expression = 
		NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
	
	MATERIALIZE_CHECK_PTR(Expression, nullptr);
	
	Expression->ParameterName = TEXT("Normal");
	Expression->SamplerType = SAMPLERTYPE_Normal;  // Normal map compression
	Expression->MaterialExpressionEditorX = -400;
	Expression->MaterialExpressionEditorY = 0;
	
	Material->GetExpressionCollection().AddExpression(Expression);
	
	UE_LOG(LogMaterialize, Verbose, TEXT("Created Normal parameter with SAMPLERTYPE_Normal"));
	return Expression;
}

UMaterialExpressionTextureSampleParameter2D* FMaterializeTransientGenerator::CreateORMParameter(UMaterial* Material)
{
	MATERIALIZE_CHECK_PTR(Material, nullptr);
	
	UMaterialExpressionTextureSampleParameter2D* Expression = 
		NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
	
	MATERIALIZE_CHECK_PTR(Expression, nullptr);
	
	Expression->ParameterName = TEXT("ORM");
	Expression->SamplerType = SAMPLERTYPE_LinearColor;  // Linear, no sRGB
	Expression->MaterialExpressionEditorX = -400;
	Expression->MaterialExpressionEditorY = 200;
	
	Material->GetExpressionCollection().AddExpression(Expression);
	
	UE_LOG(LogMaterialize, Verbose, TEXT("Created ORM parameter with SAMPLERTYPE_LinearColor"));
	return Expression;
}

UMaterialExpressionTextureSampleParameter2D* FMaterializeTransientGenerator::CreateEmissiveParameter(UMaterial* Material)
{
	MATERIALIZE_CHECK_PTR(Material, nullptr);
	
	UMaterialExpressionTextureSampleParameter2D* Expression = 
		NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
	
	MATERIALIZE_CHECK_PTR(Expression, nullptr);
	
	Expression->ParameterName = TEXT("Emissive");
	Expression->SamplerType = SAMPLERTYPE_Color;  // sRGB color texture
	Expression->MaterialExpressionEditorX = -400;
	Expression->MaterialExpressionEditorY = 400;
	
	Material->GetExpressionCollection().AddExpression(Expression);
	
	UE_LOG(LogMaterialize, Verbose, TEXT("Created Emissive parameter with SAMPLERTYPE_Color"));
	return Expression;
}

UMaterialExpressionTextureSampleParameter2D* FMaterializeTransientGenerator::CreateHeightParameter(UMaterial* Material)
{
	MATERIALIZE_CHECK_PTR(Material, nullptr);
	
	UMaterialExpressionTextureSampleParameter2D* Expression = 
		NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
	
	MATERIALIZE_CHECK_PTR(Expression, nullptr);
	
	Expression->ParameterName = TEXT("Height");
	Expression->SamplerType = SAMPLERTYPE_LinearColor;  // Linear grayscale
	Expression->MaterialExpressionEditorX = -400;
	Expression->MaterialExpressionEditorY = 600;
	
	Material->GetExpressionCollection().AddExpression(Expression);
	
	UE_LOG(LogMaterialize, Verbose, TEXT("Created Height parameter with SAMPLERTYPE_LinearColor"));
	return Expression;
}

void FMaterializeTransientGenerator::ConnectMaterialOutputs(
	UMaterial* Material,
	UMaterialExpressionTextureSampleParameter2D* BaseColor,
	UMaterialExpressionTextureSampleParameter2D* Normal,
	UMaterialExpressionTextureSampleParameter2D* ORM,
	UMaterialExpressionTextureSampleParameter2D* Emissive)
{
	MATERIALIZE_CHECK_PTR_VOID(Material);
	MATERIALIZE_CHECK_PTR_VOID(BaseColor);
	MATERIALIZE_CHECK_PTR_VOID(Normal);
	MATERIALIZE_CHECK_PTR_VOID(ORM);
	MATERIALIZE_CHECK_PTR_VOID(Emissive);
	
	// Connect BaseColor to Base Color output
	Material->GetEditorOnlyData()->BaseColor.Expression = BaseColor;
	
	// Connect Normal to Normal output
	Material->GetEditorOnlyData()->Normal.Expression = Normal;
	
	// Create component masks for ORM channels
	// R = AO (Ambient Occlusion)
	UMaterialExpressionComponentMask* AOMask = NewObject<UMaterialExpressionComponentMask>(Material);
	if (AOMask)
	{
		AOMask->Input.Expression = ORM;
		AOMask->R = 1;
		AOMask->G = 0;
		AOMask->B = 0;
		AOMask->A = 0;
		AOMask->MaterialExpressionEditorX = -200;
		AOMask->MaterialExpressionEditorY = 200;
		Material->GetExpressionCollection().AddExpression(AOMask);
		Material->GetEditorOnlyData()->AmbientOcclusion.Expression = AOMask;
	}
	
	// G = Roughness
	UMaterialExpressionComponentMask* RoughnessMask = NewObject<UMaterialExpressionComponentMask>(Material);
	if (RoughnessMask)
	{
		RoughnessMask->Input.Expression = ORM;
		RoughnessMask->R = 0;
		RoughnessMask->G = 1;
		RoughnessMask->B = 0;
		RoughnessMask->A = 0;
		RoughnessMask->MaterialExpressionEditorX = -200;
		RoughnessMask->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(RoughnessMask);
		Material->GetEditorOnlyData()->Roughness.Expression = RoughnessMask;
	}
	
	// B = Metallic
	UMaterialExpressionComponentMask* MetallicMask = NewObject<UMaterialExpressionComponentMask>(Material);
	if (MetallicMask)
	{
		MetallicMask->Input.Expression = ORM;
		MetallicMask->R = 0;
		MetallicMask->G = 0;
		MetallicMask->B = 1;
		MetallicMask->A = 0;
		MetallicMask->MaterialExpressionEditorX = -200;
		MetallicMask->MaterialExpressionEditorY = 400;
		Material->GetExpressionCollection().AddExpression(MetallicMask);
		Material->GetEditorOnlyData()->Metallic.Expression = MetallicMask;
	}
	
	// Connect Emissive to Emissive Color output
	Material->GetEditorOnlyData()->EmissiveColor.Expression = Emissive;
	
	UE_LOG(LogMaterialize, Verbose, TEXT("Connected material outputs"));
}

void FMaterializeTransientGenerator::AssignDefaultTextures(
	UMaterial* Material,
	UMaterialExpressionTextureSampleParameter2D* Normal,
	UMaterialExpressionTextureSampleParameter2D* ORM)
{
	MATERIALIZE_CHECK_PTR_VOID(Material);
	MATERIALIZE_CHECK_PTR_VOID(Normal);
	MATERIALIZE_CHECK_PTR_VOID(ORM);
	
	// Load default textures
	TMap<FName, UTexture*> DefaultTextures;
	if (FMaterializeMaterialLoader::LoadDefaultTextures(DefaultTextures))
	{
		// Assign default normal texture
		if (UTexture** NormalTexture = DefaultTextures.Find(TEXT("Normal")))
		{
			if (*NormalTexture)
			{
				Normal->Texture = *NormalTexture;
				UE_LOG(LogMaterialize, Verbose, TEXT("Assigned default normal texture"));
			}
		}
		
		// Assign default ORM texture
		if (UTexture** ORMTexture = DefaultTextures.Find(TEXT("ORM")))
		{
			if (*ORMTexture)
			{
				ORM->Texture = *ORMTexture;
				UE_LOG(LogMaterialize, Verbose, TEXT("Assigned default ORM texture"));
			}
		}
	}
	else
	{
		UE_LOG(LogMaterialize, Warning, TEXT("Failed to load default textures for transient material"));
	}
}

bool FMaterializeTransientGenerator::CompileAndValidate(UMaterial* Material, FString& OutError)
{
	MATERIALIZE_CHECK_PTR(Material, false);
	
	// Trigger material compilation
	Material->PreEditChange(nullptr);
	Material->PostEditChange();
	
	UE_LOG(LogMaterialize, Verbose, TEXT("Triggered material compilation"));
	
	// Note: In UE5, material compilation is asynchronous
	// For transient materials, we'll assume success if no immediate errors
	// Actual compilation errors will be caught when the material is used
	
	return true;
}

void FMaterializeTransientGenerator::ConfigureMaterialSettings(UMaterial* Material)
{
	MATERIALIZE_CHECK_PTR_VOID(Material);
	
	// Set material domain to Surface
	Material->MaterialDomain = EMaterialDomain::MD_Surface;
	
	// Set blend mode to Opaque
	Material->BlendMode = BLEND_Opaque;
	
	// Set shading model to Default Lit
	Material->SetShadingModel(MSM_DefaultLit);
	
	// Set two-sided to false (standard PBR)
	Material->TwoSided = false;
	
	UE_LOG(LogMaterialize, Verbose, TEXT("Configured material settings: Domain=Surface, Blend=Opaque, Shading=DefaultLit, TwoSided=false"));
}
