#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"

class MATERIALIZE_API FMaterializeEditorContext
{
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTextureChanged, UTexture2D*);
	static FOnTextureChanged OnTextureChanged;

	static void SetCurrentTexture(UTexture2D* InTexture)
	{
		CurrentTexture = InTexture;
		OnTextureChanged.Broadcast(InTexture);
	}

	static UTexture2D* GetCurrentTexture()
	{
		return CurrentTexture.Get();
	}

private:
	static TWeakObjectPtr<UTexture2D> CurrentTexture;
};

