#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MaterializeTypes.h"
#include "KLayerStack.h"
#include "MaterializeToolModel.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnKSampleModelChanged);

UCLASS(Transient)
class MATERIALIZE_API UMaterializeToolModel : public UObject {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayThumbnail = "true", AllowedClasses = "/Script/Engine.Texture2D"))
  TObjectPtr<UTexture2D> SourceTexture;

  UPROPERTY(EditAnywhere, Category = "Input", meta = (DisplayThumbnail = "true", AllowedClasses = "/Script/Engine.StaticMesh"))
  TObjectPtr<UStaticMesh> SourceStaticMesh;

  UPROPERTY(EditAnywhere, Category = "Settings", meta = (ShowOnlyInnerProperties))
  FMaterializeParams Params;

  UPROPERTY(EditAnywhere, Category = "Layers")
  bool bUseLayers = false;

  UPROPERTY(EditAnywhere, Category = "Layers", meta = (EditCondition = "bUseLayers", EditConditionHides))
  FKLayerStack LayerStack;

  UPROPERTY(EditAnywhere, Category = "Output", meta = (ContentDir))
  FDirectoryPath OutputBasePath;

  UPROPERTY(EditAnywhere, Category = "Output")
  bool bCreateSubfolder = true;

  UPROPERTY(EditAnywhere, Category = "Output")
  FString NameSuffix;

  UPROPERTY(EditAnywhere, Category = "Output", meta = (ClampMin = "0", ClampMax = "8192"))
  int32 OutputResolution = 0;

  FOnKSampleModelChanged OnModelChanged;

  virtual void PostLoad() override
  {
    Super::PostLoad();
    const int32 OldVersion = LayerStack.Version;
    if (LayerStack.MigrateFromOldVersion())
    {
      UE_LOG(LogTemp, Log, TEXT("[MaterializeToolModel] LayerStack migrated from version %d to %d"),
        OldVersion, EKLayerStackVersion::Latest);
    }
  }

  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override {
    Super::PostEditChangeProperty(PropertyChangedEvent);
    OnModelChanged.Broadcast();
  }

  FString GetComputedOutputPath() const {
    FString BasePath = OutputBasePath.Path;
    if (BasePath.IsEmpty() && SourceTexture) {
      BasePath = FPackageName::GetLongPackagePath(SourceTexture->GetOutermost()->GetName());
    }
    if (BasePath.IsEmpty()) {
      BasePath = TEXT("/Game");
    }
    if (bCreateSubfolder && SourceTexture) {
      BasePath = BasePath / SourceTexture->GetName();
    }
    return BasePath;
  }

  FString GetComputedBaseName() const {
    FString Name = SourceTexture ? SourceTexture->GetName() : TEXT("Untitled");
    if (!NameSuffix.IsEmpty()) {
      Name += NameSuffix;
    }
    return Name;
  }
};
