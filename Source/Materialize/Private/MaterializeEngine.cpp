#include "MaterializeEngine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/Texture2D.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "IAssetTools.h"
#include "MaterializeComputeEngine.h"
#include "MaterializeMaterialLoader.h"
#include "MaterializeTransientGenerator.h"
#include "MaterializeValidation.h"
#include "MaterialDomain.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/DateTime.h"
#include "Misc/PackageName.h"
#include "TextureResource.h"
#include "UObject/SavePackage.h"

bool UMaterializeEngine::GeneratePBRMaps(UTexture2D *SourceTexture,
                                     const FMaterializeParams &Params,
                                     FMaterializeResult &OutResult) {
  if (!SourceTexture)
    return false;

  double StartTime = FPlatformTime::Seconds();

  // Read source texture
  TArray<FColor> SourcePixels;
  int32 Width, Height;
  if (!ReadTexturePixels(SourceTexture, SourcePixels, Width, Height)) {
    UE_LOG(LogTemp, Error,
           TEXT("Materialize: Failed to read source texture pixels"));
    return false;
  }

  // Create grayscale buffer
  TArray<float> GrayBuffer;
  CreateGrayscaleBuffer(SourcePixels, GrayBuffer);

  // Compute Sobel edges
  TArray<float> EdgeDx, EdgeDy, EdgeMagnitude;
  ComputeSobelEdges(GrayBuffer, Width, Height, EdgeDx, EdgeDy, EdgeMagnitude);

  // Generate all maps
  OutResult.Normal = GenerateNormalMap(SourcePixels, Width, Height,
                                       Params.NormalStrength, Params);
  OutResult.Roughness = GenerateRoughnessMap(
      SourcePixels, GrayBuffer, EdgeMagnitude, Width, Height, Params);
  OutResult.Metallic = GenerateMetallicMap(
      SourcePixels, GrayBuffer, EdgeMagnitude, Width, Height, Params);
  OutResult.AO = GenerateAOMap(GrayBuffer, Width, Height, Params);
  OutResult.Height = GenerateHeightMap(GrayBuffer, Width, Height, Params);

  if (Params.EmissiveThreshold > 0.01f) {
    OutResult.Emissive =
        GenerateEmissiveMap(SourcePixels, GrayBuffer, Width, Height, Params);
  }

  double EndTime = FPlatformTime::Seconds();
  OutResult.GenerationTimeMs = (EndTime - StartTime) * 1000.0f;

  UE_LOG(LogTemp, Log, TEXT("Materialize: Generated PBR maps in %.1f ms"),
         OutResult.GenerationTimeMs);

  return OutResult.IsValid();
}

bool UMaterializeEngine::GenerateAndSavePBRMaps(UTexture2D *SourceTexture,
                                            const FMaterializeParams &Params,
                                            const FString &OutputPath,
                                            const FString &BaseName,
                                            FMaterializeResult &OutResult) {
  if (!SourceTexture)
    return false;

  // Generate maps first using high-performance GPU pipe
  if (!UMaterializeComputeEngine::GeneratePBRMapsGPU(SourceTexture, Params,
                                                 OutResult)) {
    return false;
  }

  // Readback results from GPU to CPU for saving
  TMap<FString, TArray<FColor>> PixelData;
  UMaterializeComputeEngine::ReadbackResult(OutResult, PixelData);

  // Determine output path - use source texture location if not specified
  FString PackagePath = OutputPath;
  FString AssetBaseName = BaseName;

  if (PackagePath.IsEmpty()) {
    PackagePath = FPackageName::GetLongPackagePath(
        SourceTexture->GetOutermost()->GetName());
  }
  if (AssetBaseName.IsEmpty()) {
    AssetBaseName = SourceTexture->GetName();
  }

  UE_LOG(LogTemp, Log, TEXT("Materialize: Saving PBR maps to %s/%s_*"),
         *PackagePath, *AssetBaseName);

  // Save each texture as a persistent asset
  int32 Width = SourceTexture->GetSizeX();
  int32 Height = SourceTexture->GetSizeY();

  auto SaveTextureAsset = [&](UTexture2D *TransientTexture,
                              const FString &Suffix,
                              bool bNormalMap) -> UTexture2D * {
    if (!TransientTexture)
      return nullptr;

    FString AssetName = FString::Printf(TEXT("%s_%s"), *AssetBaseName, *Suffix);
    FString PackageName = PackagePath / AssetName;

    // Create package
    UPackage *Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    // Create new texture in the package
    UTexture2D *NewTexture =
        NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);

    // Copy texture data
    const TArray<FColor> *ReadbackPixels = PixelData.Find(Suffix);
    if (!ReadbackPixels || ReadbackPixels->Num() == 0)
      return nullptr;

    NewTexture->Source.Init(Width, Height, 1, 1, TSF_BGRA8);
    uint8 *DestData = NewTexture->Source.LockMip(0);
    FMemory::Memcpy(DestData, ReadbackPixels->GetData(), Width * Height * 4);
    NewTexture->Source.UnlockMip(0);

    // Configure texture settings
    NewTexture->SRGB = !bNormalMap && Suffix != TEXT("Roughness") &&
                       Suffix != TEXT("Metallic") && Suffix != TEXT("AO") &&
                       Suffix != TEXT("Height") && Suffix != TEXT("ORM");
    NewTexture->CompressionSettings = bNormalMap ? TC_Normalmap : TC_Default;
    NewTexture->MipGenSettings = TMGS_FromTextureGroup;
    NewTexture->LODGroup =
        bNormalMap ? TEXTUREGROUP_WorldNormalMap : TEXTUREGROUP_World;

    NewTexture->UpdateResource();
    NewTexture->PostEditChange();
    NewTexture->MarkPackageDirty();

    // Save package
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.Error = GError;

    FString PackageFilename = FPackageName::LongPackageNameToFilename(
        PackageName, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Package, NewTexture, *PackageFilename, SaveArgs);

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewTexture);

    UE_LOG(LogTemp, Log, TEXT("Materialize: Saved %s"), *PackageName);
    return NewTexture;
  };

  // Save all textures
  OutResult.Normal = SaveTextureAsset(OutResult.Normal, TEXT("Normal"), true);
  OutResult.Roughness =
      SaveTextureAsset(OutResult.Roughness, TEXT("Roughness"), false);
  OutResult.Metallic =
      SaveTextureAsset(OutResult.Metallic, TEXT("Metallic"), false);
  OutResult.AO = SaveTextureAsset(OutResult.AO, TEXT("AO"), false);
  OutResult.Height = SaveTextureAsset(OutResult.Height, TEXT("Height"), false);

  if (OutResult.Emissive) {
    OutResult.Emissive =
        SaveTextureAsset(OutResult.Emissive, TEXT("Emissive"), false);
  }

  // Use GPU generated ORM if available
  UTexture2D *SavedORM = SaveTextureAsset(OutResult.ORM, TEXT("ORM"), false);

  // Create Material Instance
  FString MaterialAssetName = FString::Printf(TEXT("MI_%s"), *AssetBaseName);
  FString MaterialPackageName = PackagePath / MaterialAssetName;

  UPackage *MaterialPackage = CreatePackage(*MaterialPackageName);
  MaterialPackage->FullyLoad();

  // Try plugin base material, fallback to engine default
  // Try to load standard plugin master material (Preferred)
  // Use the new material loading system with fallback chain
  FMaterializeMaterialLoadResult LoadResult = FMaterializeMaterialLoader::LoadMasterMaterial(TEXT("Standard"));
  
  UMaterial *ParentMaterial = nullptr;
  
  if (LoadResult.IsValid()) {
    ParentMaterial = LoadResult.Material;
    UE_LOG(LogMaterialize, Log, TEXT("Materialize: Loaded master material successfully from %s"), 
      LoadResult.LoadSource == FMaterializeMaterialLoadResult::ELoadSource::Plugin ? TEXT("Plugin") : TEXT("Game Override"));
  } else {
    // Fallback to generating transient material if master doesn't exist
    UE_LOG(LogMaterialize, Warning,
           TEXT("Materialize: Master material not found, generating transient material. Error: %s"), 
           *LoadResult.ErrorMessage);
    ParentMaterial = FMaterializeTransientGenerator::Generate(TEXT("Standard"));
  }

  // Final fallback to engine default material
  if (!ParentMaterial) {
    UE_LOG(LogMaterialize, Error, TEXT("Materialize: Failed to generate transient material, falling back to engine default"));
    ParentMaterial = LoadObject<UMaterial>(
        nullptr,
        TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
  }

  if (ParentMaterial) {
    UMaterialInstanceConstant *NewMaterialInstance =
        NewObject<UMaterialInstanceConstant>(
            MaterialPackage, *MaterialAssetName, RF_Public | RF_Standalone);
    NewMaterialInstance->Parent = ParentMaterial;

    // Set texture parameters
    if (SourceTexture) {
      FTextureParameterValue Param;
      Param.ParameterInfo.Name = TEXT("BaseColor");
      Param.ParameterValue = SourceTexture;
      NewMaterialInstance->TextureParameterValues.Add(Param);
    }
    if (SavedORM) {
      FTextureParameterValue Param;
      Param.ParameterInfo.Name = TEXT("ORM");
      Param.ParameterValue = SavedORM;
      NewMaterialInstance->TextureParameterValues.Add(Param);
    }
    if (OutResult.Normal) {
      FTextureParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Normal");
      Param.ParameterValue = OutResult.Normal;
      NewMaterialInstance->TextureParameterValues.Add(Param);
    }
    if (OutResult.Roughness) {
      FTextureParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Roughness");
      Param.ParameterValue = OutResult.Roughness;
      NewMaterialInstance->TextureParameterValues.Add(Param);
    }
    if (OutResult.Metallic) {
      FTextureParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Metallic");
      Param.ParameterValue = OutResult.Metallic;
      NewMaterialInstance->TextureParameterValues.Add(Param);
    }
    if (OutResult.AO) {
      FTextureParameterValue Param;
      Param.ParameterInfo.Name = TEXT("AO");
      Param.ParameterValue = OutResult.AO;
      NewMaterialInstance->TextureParameterValues.Add(Param);
    }
    if (OutResult.Height) {
      FTextureParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Height");
      Param.ParameterValue = OutResult.Height;
      NewMaterialInstance->TextureParameterValues.Add(Param);
    }
    if (OutResult.Emissive) {
      FTextureParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Emissive");
      Param.ParameterValue = OutResult.Emissive;
      NewMaterialInstance->TextureParameterValues.Add(Param);
    }

    // Set scalar parameters from input params
    // These match the parameter names in M_Materialize_Master
    {
      FScalarParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Tiling");
      Param.ParameterValue = 1.0f;
      NewMaterialInstance->ScalarParameterValues.Add(Param);
    }
    {
      FScalarParameterValue Param;
      Param.ParameterInfo.Name = TEXT("AO_Power");
      Param.ParameterValue = Params.AOIntensity;
      NewMaterialInstance->ScalarParameterValues.Add(Param);
    }
    {
      FScalarParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Roughness_Mult");
      Param.ParameterValue = Params.RoughnessContrast;
      NewMaterialInstance->ScalarParameterValues.Add(Param);
    }
    {
      FScalarParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Roughness_Offset");
      Param.ParameterValue = Params.RoughnessBrightness / 255.0f;
      NewMaterialInstance->ScalarParameterValues.Add(Param);
    }
    {
      FScalarParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Metallic_Mult");
      Param.ParameterValue = Params.MetallicContrast;
      NewMaterialInstance->ScalarParameterValues.Add(Param);
    }
    {
      FScalarParameterValue Param;
      Param.ParameterInfo.Name = TEXT("Emissive_Power");
      Param.ParameterValue = Params.EmissiveThreshold;
      NewMaterialInstance->ScalarParameterValues.Add(Param);
    }

    NewMaterialInstance->PostEditChange();
    NewMaterialInstance->MarkPackageDirty();

    // Save material package
    FSavePackageArgs MaterialSaveArgs;
    MaterialSaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    FString MaterialFilename = FPackageName::LongPackageNameToFilename(
        MaterialPackageName, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(MaterialPackage, NewMaterialInstance,
                          *MaterialFilename, MaterialSaveArgs);

    FAssetRegistryModule::AssetCreated(NewMaterialInstance);

    UE_LOG(LogTemp, Log, TEXT("Materialize: Created Material Instance %s"),
           *MaterialPackageName);
  }

  return true;
}

bool UMaterializeEngine::ReadTexturePixels(UTexture2D *Texture,
                                       TArray<FColor> &OutPixels,
                                       int32 &OutWidth, int32 &OutHeight) {
  if (!Texture)
    return false;

  // Force texture to be readable
  Texture->SRGB = true;
  Texture->CompressionSettings = TC_VectorDisplacementmap;
  Texture->MipGenSettings = TMGS_NoMipmaps;
  Texture->UpdateResource();

  FTexture2DMipMap &Mip = Texture->GetPlatformData()->Mips[0];
  OutWidth = Mip.SizeX;
  OutHeight = Mip.SizeY;

  void *Data = Mip.BulkData.Lock(LOCK_READ_ONLY);
  if (!Data) {
    Mip.BulkData.Unlock();
    return false;
  }

  const int32 NumPixels = OutWidth * OutHeight;
  OutPixels.SetNumUninitialized(NumPixels);

  const FColor *SrcColors = static_cast<const FColor *>(Data);
  FMemory::Memcpy(OutPixels.GetData(), SrcColors, NumPixels * sizeof(FColor));

  Mip.BulkData.Unlock();
  return true;
}

void UMaterializeEngine::CreateGrayscaleBuffer(const TArray<FColor> &Colors,
                                           TArray<float> &OutGray) {
  OutGray.SetNumUninitialized(Colors.Num());
  for (int32 i = 0; i < Colors.Num(); ++i) {
    const FColor &C = Colors[i];
    // Standard luminance formula
    OutGray[i] = C.R * 0.299f + C.G * 0.587f + C.B * 0.114f;
  }
}

void UMaterializeEngine::ComputeSobelEdges(const TArray<float> &GrayBuffer,
                                       int32 Width, int32 Height,
                                       TArray<float> &OutDx,
                                       TArray<float> &OutDy,
                                       TArray<float> &OutMagnitude) {
  const int32 NumPixels = Width * Height;
  OutDx.SetNumZeroed(NumPixels);
  OutDy.SetNumZeroed(NumPixels);
  OutMagnitude.SetNumZeroed(NumPixels);

  auto GetIdx = [Width, Height](int32 X, int32 Y) -> int32 {
    X = FMath::Clamp(X, 0, Width - 1);
    Y = FMath::Clamp(Y, 0, Height - 1);
    return Y * Width + X;
  };

  for (int32 Y = 0; Y < Height; ++Y) {
    for (int32 X = 0; X < Width; ++X) {
      // Sobel kernel
      float TL = GrayBuffer[GetIdx(X - 1, Y - 1)];
      float T = GrayBuffer[GetIdx(X, Y - 1)];
      float TR = GrayBuffer[GetIdx(X + 1, Y - 1)];
      float L = GrayBuffer[GetIdx(X - 1, Y)];
      float R = GrayBuffer[GetIdx(X + 1, Y)];
      float BL = GrayBuffer[GetIdx(X - 1, Y + 1)];
      float B = GrayBuffer[GetIdx(X, Y + 1)];
      float BR = GrayBuffer[GetIdx(X + 1, Y + 1)];

      float Dx = (TR + 2.0f * R + BR) - (TL + 2.0f * L + BL);
      float Dy = (BL + 2.0f * B + BR) - (TL + 2.0f * T + TR);

      int32 Idx = Y * Width + X;
      OutDx[Idx] = Dx;
      OutDy[Idx] = Dy;
      OutMagnitude[Idx] = FMath::Sqrt(Dx * Dx + Dy * Dy);
    }
  }
}

UTexture2D *
UMaterializeEngine::GenerateNormalMap(const TArray<FColor> &SourcePixels,
                                  int32 Width, int32 Height, float Strength,
                                  const FMaterializeParams &Params) {
  TArray<float> GrayBuffer;
  CreateGrayscaleBuffer(SourcePixels, GrayBuffer);

  TArray<float> Dx, Dy, Mag;
  ComputeSobelEdges(GrayBuffer, Width, Height, Dx, Dy, Mag);

  TArray<FColor> OutPixels;
  OutPixels.SetNumUninitialized(Width * Height);

  const float Dz = 1.0f / FMath::Max(0.001f, Strength);

  for (int32 i = 0; i < Width * Height; ++i) {
    int32 X = i % Width;
    int32 Y = i / Width;

    float Vx = Dx[i];
    float Vy = Dy[i];

    // Add scratches noise
    if (Params.Scratches > 0.0f) {
      Vx += (FMath::FRand() - 0.5f) * Params.Scratches * 100.0f;
      Vy += (FMath::FRand() - 0.5f) * Params.Scratches * 100.0f;
    }

    // Add cyber detail
    if (Params.CyberDetail > 0.0f &&
        CyberNoise(X, Y, Params.CyberScale) > 0.5f) {
      Vx += 50.0f * Params.CyberDetail;
    }

    // Normalize
    float Len = FMath::Sqrt(Vx * Vx + Vy * Vy + Dz * Dz);
    float Nx = (Vx / Len) * 0.5f + 0.5f;
    float Ny = (Vy / Len) * 0.5f + 0.5f;
    float Nz = (Dz / Len) * 0.5f + 0.5f;

    OutPixels[i] = FColor(ClampByte(Nx * 255.0f), ClampByte(Ny * 255.0f),
                          ClampByte(Nz * 255.0f), 255);
  }

  return CreateTextureFromPixels(OutPixels, Width, Height, TEXT("Normal"),
                                 false);
}

UTexture2D *UMaterializeEngine::GenerateRoughnessMap(
    const TArray<FColor> &SourcePixels, const TArray<float> &GrayBuffer,
    const TArray<float> &EdgeMagnitude, int32 Width, int32 Height,
    const FMaterializeParams &Params) {
  TArray<FColor> OutPixels;
  OutPixels.SetNumUninitialized(Width * Height);

  for (int32 i = 0; i < Width * Height; ++i) {
    int32 X = i % Width;
    int32 Y = i / Width;

    float BaseRough = Params.RoughnessBase * 255.0f;
    float Lum = GrayBuffer[i];
    float Edge = EdgeMagnitude[i];

    // Start from base, apply contrast around midpoint
    float Val = BaseRough + (Lum - 128.0f) * Params.RoughnessContrast * 0.3f +
                Params.RoughnessBrightness;

    if (Params.bRoughnessInvert) {
      Val = 255.0f - Val;
    }

    // Dust makes rougher
    if (Params.Dust > 0.0f) {
      float DustNoise = FMath::FRand() * 0.5f + 0.5f;
      Val = Lerp(Val, 255.0f, Params.Dust * DustNoise);
    }

    // Bio detail
    if (Params.BioDetail > 0.0f) {
      float Bio = BioNoise(X, Y, Params.BioFrequency);
      Val = Lerp(Val, 255.0f, Bio * Params.BioDetail);
    }

    // Edge wear (lightens edges - makes them shinier = less rough)
    if (Params.EdgeWear > 0.0f && Edge > (255.0f - Params.EdgeWear * 200.0f)) {
      Val = Lerp(Val, 0.0f, 0.5f);
    }

    // Cavity dirt (darkens crevices = rougher)
    if (Params.CavityDirt > 0.0f &&
        Edge > (255.0f - Params.CavityDirt * 200.0f)) {
      Val = Lerp(Val, 255.0f, 0.5f);
    }

    // Cyber detail
    if (Params.CyberDetail > 0.0f &&
        CyberNoise(X, Y, Params.CyberScale) > 0.5f) {
      Val = 10.0f; // Very smooth cyber lines
    }

    // Noise
    if (Params.Noise > 0.0f) {
      Val += (FMath::FRand() - 0.5f) * Params.Noise * 50.0f;
    }

    uint8 Final = ClampByte(Val);
    OutPixels[i] = FColor(Final, Final, Final, 255);
  }

  return CreateTextureFromPixels(OutPixels, Width, Height, TEXT("Roughness"),
                                 false);
}

UTexture2D *UMaterializeEngine::GenerateMetallicMap(
    const TArray<FColor> &SourcePixels, const TArray<float> &GrayBuffer,
    const TArray<float> &EdgeMagnitude, int32 Width, int32 Height,
    const FMaterializeParams &Params) {
  TArray<FColor> OutPixels;
  OutPixels.SetNumUninitialized(Width * Height);

  for (int32 i = 0; i < Width * Height; ++i) {
    int32 X = i % Width;
    int32 Y = i / Width;

    float BaseMetal = Params.MetallicBase * 255.0f;
    float Lum = GrayBuffer[i];
    float Edge = EdgeMagnitude[i];

    // Start from base, apply contrast
    float Val = BaseMetal + (Lum - 128.0f) * Params.MetallicContrast * 0.2f +
                Params.MetallicBias;

    // Edge wear exposes metal (white)
    if (Params.EdgeWear > 0.0f && Edge > (255.0f - Params.EdgeWear * 200.0f)) {
      Val = Lerp(Val, 255.0f, 0.7f);
    }

    // Cavity dirt covers metal (black)
    if (Params.CavityDirt > 0.0f &&
        Edge > (255.0f - Params.CavityDirt * 200.0f)) {
      Val = Lerp(Val, 0.0f, 0.7f);
    }

    // Grunge reduces metallic
    if (Params.Grunge > 0.0f) {
      Val = Lerp(Val, 0.0f, FMath::FRand() * Params.Grunge);
    }

    // Cyber detail = full metal
    if (Params.CyberDetail > 0.0f &&
        CyberNoise(X, Y, Params.CyberScale) > 0.5f) {
      Val = 255.0f;
    }

    uint8 Final = ClampByte(Val);
    OutPixels[i] = FColor(Final, Final, Final, 255);
  }

  return CreateTextureFromPixels(OutPixels, Width, Height, TEXT("Metallic"),
                                 false);
}

UTexture2D *UMaterializeEngine::GenerateAOMap(const TArray<float> &GrayBuffer,
                                          int32 Width, int32 Height,
                                          const FMaterializeParams &Params) {
  TArray<FColor> OutPixels;
  OutPixels.SetNumUninitialized(Width * Height);

  for (int32 i = 0; i < Width * Height; ++i) {
    float Lum = GrayBuffer[i];

    // Simple AO approximation from luminance
    float Val =
        (Lum * Params.AOIntensity) + (255.0f * (1.0f - Params.AOIntensity));

    // Grunge darkens
    if (Params.Grunge > 0.0f) {
      Val -= FMath::FRand() * Params.Grunge * 50.0f;
    }

    uint8 Final = ClampByte(Val);
    OutPixels[i] = FColor(Final, Final, Final, 255);
  }

  return CreateTextureFromPixels(OutPixels, Width, Height, TEXT("AO"), false);
}

UTexture2D *UMaterializeEngine::GenerateHeightMap(const TArray<float> &GrayBuffer,
                                              int32 Width, int32 Height,
                                              const FMaterializeParams &Params) {
  TArray<FColor> OutPixels;
  OutPixels.SetNumUninitialized(Width * Height);

  for (int32 i = 0; i < Width * Height; ++i) {
    int32 X = i % Width;
    int32 Y = i / Width;

    float Lum = GrayBuffer[i];

    // Apply contrast around midpoint
    float Val = (Lum - 128.0f) * Params.HeightContrast + 128.0f;

    // Cyber detail adds height
    if (Params.CyberDetail > 0.0f &&
        CyberNoise(X, Y, Params.CyberScale) > 0.5f) {
      Val += 50.0f;
    }

    uint8 Final = ClampByte(Val);
    OutPixels[i] = FColor(Final, Final, Final, 255);
  }

  return CreateTextureFromPixels(OutPixels, Width, Height, TEXT("Height"),
                                 false);
}

UTexture2D *UMaterializeEngine::GenerateEmissiveMap(
    const TArray<FColor> &SourcePixels, const TArray<float> &GrayBuffer,
    int32 Width, int32 Height, const FMaterializeParams &Params) {
  TArray<FColor> OutPixels;
  OutPixels.SetNumUninitialized(Width * Height);

  const float Threshold = 255.0f * (1.0f - Params.EmissiveThreshold);

  for (int32 i = 0; i < Width * Height; ++i) {
    int32 X = i % Width;
    int32 Y = i / Width;

    float Lum = GrayBuffer[i];
    const FColor &Src = SourcePixels[i];

    if (Lum > Threshold) {
      // Emit original color
      OutPixels[i] = Src;
    } else if (Params.CyberDetail > 0.2f &&
               CyberNoise(X, Y, Params.CyberScale) > 0.5f) {
      // Cyber glow
      OutPixels[i] = FColor(0, 255, 255, 255);
    } else {
      // Black (no emission)
      OutPixels[i] = FColor(0, 0, 0, 255);
    }
  }

  return CreateTextureFromPixels(OutPixels, Width, Height, TEXT("Emissive"),
                                 true);
}

UTexture2D *UMaterializeEngine::CreateTextureFromPixels(
    const TArray<FColor> &Pixels, int32 Width, int32 Height,
    const FString &TextureName, bool bSRGB) {
  UTexture2D *Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
  if (!Texture)
    return nullptr;

  Texture->SRGB = bSRGB;
  Texture->Filter = TF_Trilinear;
  Texture->AddressX = TA_Wrap;
  Texture->AddressY = TA_Wrap;

  void *TextureData =
      Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
  FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
  Texture->GetPlatformData()->Mips[0].BulkData.Unlock();

  Texture->UpdateResource();

  return Texture;
}

UTexture2D *UMaterializeEngine::PackORM(UTexture2D *AO, UTexture2D *Roughness,
                                    UTexture2D *Metallic) {
  UTexture2D *Ref = AO ? AO : (Roughness ? Roughness : Metallic);
  if (!Ref)
    return nullptr;
  FTexture2DMipMap &RefMip = Ref->GetPlatformData()->Mips[0];
  int32 Width = RefMip.SizeX;
  int32 Height = RefMip.SizeY;
  int32 Num = Width * Height;

  const FColor *AOPixels = nullptr;
  const FColor *RoughPixels = nullptr;
  const FColor *MetalPixels = nullptr;
  void *AData = nullptr;
  void *RData = nullptr;
  void *MData = nullptr;
  if (AO) {
    AData = AO->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY);
    AOPixels = static_cast<const FColor *>(AData);
  }
  if (Roughness) {
    RData = Roughness->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY);
    RoughPixels = static_cast<const FColor *>(RData);
  }
  if (Metallic) {
    MData = Metallic->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY);
    MetalPixels = static_cast<const FColor *>(MData);
  }

  TArray<FColor> Packed;
  Packed.SetNumUninitialized(Num);
  for (int32 i = 0; i < Num; ++i) {
    uint8 R = RoughPixels ? RoughPixels[i].R : 128; // Roughness default mid
    uint8 G = AOPixels ? AOPixels[i].R : 255;       // AO default full
    uint8 B = MetalPixels ? MetalPixels[i].R : 0;   // Metallic default none
    Packed[i] = FColor(G, R, B, 255); // R=AO, G=Roughness, B=Metallic
  }

  if (AO)
    AO->GetPlatformData()->Mips[0].BulkData.Unlock();
  if (Roughness)
    Roughness->GetPlatformData()->Mips[0].BulkData.Unlock();
  if (Metallic)
    Metallic->GetPlatformData()->Mips[0].BulkData.Unlock();

  return CreateTextureFromPixels(Packed, Width, Height, TEXT("ORM"), false);
}
uint8 UMaterializeEngine::ClampByte(float Value) {
  return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Value), 0, 255));
}

float UMaterializeEngine::Lerp(float A, float B, float T) {
  return A * (1.0f - T) + B * T;
}

float UMaterializeEngine::CyberNoise(int32 X, int32 Y, float Scale) {
  int32 CX = FMath::FloorToInt(X * Scale);
  int32 CY = FMath::FloorToInt(Y * Scale);
  return ((CX ^ CY) % 13) == 0 ? 1.0f : 0.0f;
}

float UMaterializeEngine::BioNoise(int32 X, int32 Y, float Frequency) {
  return (FMath::Sin(X * Frequency * 0.1f) * FMath::Cos(Y * Frequency * 0.1f)) *
             0.5f +
         0.5f;
}
