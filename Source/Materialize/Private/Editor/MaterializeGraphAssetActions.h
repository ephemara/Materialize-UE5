#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_KSampleGraph : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_KSampleGraph(EAssetTypeCategories::Type InAssetCategory);

	//~ Begin IAssetTypeActions Interface
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;
	//~ End IAssetTypeActions Interface

private:
	EAssetTypeCategories::Type MyAssetCategory;
};
