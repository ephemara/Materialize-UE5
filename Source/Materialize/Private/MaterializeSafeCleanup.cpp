#include "MaterializeSafeCleanup.h"
#include "MaterializeValidation.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpression.h"

void FMaterializeSafeCleanup::CleanupTexture(UTexture2D*& Texture)
{
	if (Texture && IsValid(Texture))
	{
		UE_LOG(LogMaterialize, Verbose, TEXT("Cleaning up texture: %s"), *Texture->GetName());
		Texture->ConditionalBeginDestroy();
		Texture = nullptr;
	}
}

void FMaterializeSafeCleanup::CleanupRenderTarget(UTextureRenderTarget2D*& RenderTarget)
{
	if (RenderTarget && IsValid(RenderTarget))
	{
		UE_LOG(LogMaterialize, Verbose, TEXT("Cleaning up render target: %s"), *RenderTarget->GetName());
		RenderTarget->ConditionalBeginDestroy();
		RenderTarget = nullptr;
	}
}

void FMaterializeSafeCleanup::CleanupMaterial(UMaterial*& Material)
{
	if (Material && IsValid(Material))
	{
		UE_LOG(LogMaterialize, Verbose, TEXT("Cleaning up material: %s"), *Material->GetName());
		Material->ConditionalBeginDestroy();
		Material = nullptr;
	}
}

void FMaterializeSafeCleanup::CleanupMaterialInstance(UMaterialInstanceDynamic*& MID)
{
	if (MID && IsValid(MID))
	{
		UE_LOG(LogMaterialize, Verbose, TEXT("Cleaning up material instance: %s"), *MID->GetName());
		MID->ConditionalBeginDestroy();
		MID = nullptr;
	}
}

void FMaterializeSafeCleanup::CleanupMaterialExpression(UMaterialExpression*& Expression)
{
	if (Expression && IsValid(Expression))
	{
		UE_LOG(LogMaterialize, Verbose, TEXT("Cleaning up material expression: %s"), *Expression->GetName());
		Expression->ConditionalBeginDestroy();
		Expression = nullptr;
	}
}
