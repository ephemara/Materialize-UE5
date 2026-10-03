#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "Engine/Texture2D.h"

/**
 * Asset actions for Texture2D - adds "Materialize: Generate PBR Material" context menu
 */
class MATERIALIZE_API FMaterializeTextureAssetActions : public FAssetTypeActions_Base
{
public:
	FMaterializeTextureAssetActions(EAssetTypeCategories::Type InCategory = EAssetTypeCategories::Textures)
		: Category(InCategory)
	{}
	// FAssetTypeActions_Base interface
	virtual FText GetName() const override { return NSLOCTEXT("Materialize", "TextureActions", "Texture"); }
	virtual FColor GetTypeColor() const override { return FColor(255, 128, 0); }
	virtual UClass* GetSupportedClass() const override { return UTexture2D::StaticClass(); }
	virtual uint32 GetCategories() override { return Category; }
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return true; }
	
	virtual void GetActions(const TArray<UObject*>& InObjects, struct FToolMenuSection& Section) override;
	// End interface

private:
	void OpenKSampleEditor(TArray<TWeakObjectPtr<UTexture2D>> Textures);
	
private:
	EAssetTypeCategories::Type Category;
};
