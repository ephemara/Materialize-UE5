#include "MaterializeErrorHandler.h"
#include "MaterializeValidation.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"

// Define the log category (declared in MaterializeValidation.h)
DEFINE_LOG_CATEGORY(LogMaterialize);

void FMaterializeErrorHandler::LogError(const FString& Context, const FString& Message)
{
	FString FormattedMessage = FormatError(Context, Message);
	UE_LOG(LogMaterialize, Error, TEXT("%s"), *FormattedMessage);
}

void FMaterializeErrorHandler::LogWarning(const FString& Context, const FString& Message)
{
	FString FormattedMessage = FormatError(Context, Message);
	UE_LOG(LogMaterialize, Warning, TEXT("%s"), *FormattedMessage);
}

bool FMaterializeErrorHandler::ValidateTexture(UTexture2D* Texture, FString& OutError)
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

	if (!ValidateDimensions(Width, Height, OutError))
	{
		OutError = FString::Printf(TEXT("Texture '%s' has invalid dimensions: %s"), 
			*Texture->GetName(), *OutError);
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
	if (!HasValidGPUResource(Texture, OutError))
	{
		OutError = FString::Printf(TEXT("Texture '%s': %s"), *Texture->GetName(), *OutError);
		return false;
	}

	return true;
}

bool FMaterializeErrorHandler::ValidateRenderTarget(UTextureRenderTarget2D* RenderTarget, FString& OutError)
{
	if (!RenderTarget)
	{
		OutError = TEXT("Render target is null");
		return false;
	}

	if (!RenderTarget->IsValidLowLevel())
	{
		OutError = FString::Printf(TEXT("Render target '%s' is not valid"), *RenderTarget->GetName());
		return false;
	}

	// Check dimensions
	int32 Width = RenderTarget->SizeX;
	int32 Height = RenderTarget->SizeY;

	if (!ValidateDimensions(Width, Height, OutError))
	{
		OutError = FString::Printf(TEXT("Render target '%s' has invalid dimensions: %s"), 
			*RenderTarget->GetName(), *OutError);
		return false;
	}

	// Check if render target resource is available
	if (!RenderTarget->GetResource())
	{
		OutError = FString::Printf(TEXT("Render target '%s' has no GPU resource"), *RenderTarget->GetName());
		return false;
	}

	return true;
}

bool FMaterializeErrorHandler::ValidateDimensions(int32 Width, int32 Height, FString& OutError)
{
	if (Width <= 0 || Height <= 0)
	{
		OutError = FString::Printf(TEXT("Invalid dimensions: %dx%d (must be positive)"), Width, Height);
		return false;
	}

	if (Width > MaxTextureDimension || Height > MaxTextureDimension)
	{
		OutError = FString::Printf(TEXT("Dimensions exceed maximum allowed (%dx%d): %dx%d"), 
			MaxTextureDimension, MaxTextureDimension, Width, Height);
		return false;
	}

	return true;
}

bool FMaterializeErrorHandler::ValidateRange(float Value, float Min, float Max, const FString& ParamName, FString& OutError)
{
	if (Value < Min || Value > Max)
	{
		OutError = FString::Printf(TEXT("%s is out of range: %f (must be between %f and %f)"), 
			*ParamName, Value, Min, Max);
		return false;
	}

	return true;
}

bool FMaterializeErrorHandler::ValidateRange(int32 Value, int32 Min, int32 Max, const FString& ParamName, FString& OutError)
{
	if (Value < Min || Value > Max)
	{
		OutError = FString::Printf(TEXT("%s is out of range: %d (must be between %d and %d)"), 
			*ParamName, Value, Min, Max);
		return false;
	}

	return true;
}

FString FMaterializeErrorHandler::FormatError(const FString& Context, const FString& Message)
{
	if (Context.IsEmpty())
	{
		return Message;
	}

	return FString::Printf(TEXT("[%s] %s"), *Context, *Message);
}

bool FMaterializeErrorHandler::HasValidGPUResource(UTexture2D* Texture, FString& OutError)
{
	if (!Texture)
	{
		OutError = TEXT("Texture is null");
		return false;
	}

	if (!Texture->GetResource())
	{
		OutError = TEXT("Texture has no GPU resource");
		return false;
	}

	return true;
}

bool FMaterializeErrorHandler::ValidateCompatibleDimensions(UTexture2D* Texture1, UTexture2D* Texture2, FString& OutError)
{
	if (!Texture1 || !Texture2)
	{
		OutError = TEXT("One or both textures are null");
		return false;
	}

	int32 Width1 = Texture1->GetSizeX();
	int32 Height1 = Texture1->GetSizeY();
	int32 Width2 = Texture2->GetSizeX();
	int32 Height2 = Texture2->GetSizeY();

	if (Width1 != Width2 || Height1 != Height2)
	{
		OutError = FString::Printf(TEXT("Texture dimensions do not match: '%s' (%dx%d) vs '%s' (%dx%d)"),
			*Texture1->GetName(), Width1, Height1,
			*Texture2->GetName(), Width2, Height2);
		return false;
	}

	return true;
}
