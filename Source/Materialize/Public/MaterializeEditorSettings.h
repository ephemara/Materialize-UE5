#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterializeEditorSettings.generated.h"

UCLASS(Config=Editor)
class MATERIALIZE_API UMaterializeEditorSettings : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, Category="Materialize")
	FName LastPresetId;

	UPROPERTY(Config, EditAnywhere, Category="Materialize")
	bool bSaveToSourceFolderDefault = true;

	static UMaterializeEditorSettings* Get()
	{
		return GetMutableDefault<UMaterializeEditorSettings>();
	}
};

