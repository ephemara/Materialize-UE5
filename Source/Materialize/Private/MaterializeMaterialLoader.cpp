#include "MaterializeMaterialLoader.h"
#include "MaterializeValidation.h"
#include "Materials/Material.h"
#include "Engine/Texture.h"
#include "UObject/UObjectGlobals.h"

// Initialize static material cache
TMap<FString, UMaterial*> FMaterializeMaterialLoader::MaterialCache;

FMaterializeMaterialLoadResult FMaterializeMaterialLoader::LoadMasterMaterial(const FString& PresetName)
{
	FMaterializeMaterialLoadResult Result;
	
	// Construct material paths based on preset name.
	// Named presets (Metal/Glossy/Toon) live in the /Presets/ subfolder;
	// only the Standard base master lives in the Materials root.
	const FString SubDir = (PresetName == TEXT("Standard")) ? TEXT("") : TEXT("Presets/");
	FString PluginPath = FString::Printf(TEXT("/Materialize/Materials/%sM_Materialize_Master_%s.M_Materialize_Master_%s"), 
		*SubDir, *PresetName, *PresetName);
	FString GamePath = FString::Printf(TEXT("/Game/Materialize/Materials/%sM_Materialize_Master_%s.M_Materialize_Master_%s"), 
		*SubDir, *PresetName, *PresetName);
	
	// For "Standard" preset, also try the base master material path
	if (PresetName == TEXT("Standard"))
	{
		FString BasePluginPath = TEXT("/Materialize/Materials/M_Materialize_Master.M_Materialize_Master");
		FString BaseGamePath = TEXT("/Game/Materialize/Materials/M_Materialize_Master.M_Materialize_Master");
		
		// Try base plugin path first
		UMaterial* Material = TryLoadMaterialFromPath(BasePluginPath);
		if (Material)
		{
			FString ValidationError;
			if (ValidateMaterial(Material, ValidationError))
			{
				Result.Material = Material;
				Result.bSuccess = true;
				Result.LoadSource = FMaterializeMaterialLoadResult::ELoadSource::Plugin;
				LogMaterialLoadAttempt(BasePluginPath, true);
				return Result;
			}
			else
			{
				UE_LOG(LogMaterialize, Warning, TEXT("Material loaded but validation failed: %s - %s"), 
					*BasePluginPath, *ValidationError);
			}
		}
		
		// Try base game override path
		Material = TryLoadMaterialFromPath(BaseGamePath);
		if (Material)
		{
			FString ValidationError;
			if (ValidateMaterial(Material, ValidationError))
			{
				Result.Material = Material;
				Result.bSuccess = true;
				Result.LoadSource = FMaterializeMaterialLoadResult::ELoadSource::GameOverride;
				LogMaterialLoadAttempt(BaseGamePath, true);
				return Result;
			}
			else
			{
				UE_LOG(LogMaterialize, Warning, TEXT("Material loaded but validation failed: %s - %s"), 
					*BaseGamePath, *ValidationError);
			}
		}
	}
	
	// Try preset-specific plugin path
	UMaterial* Material = TryLoadMaterialFromPath(PluginPath);
	if (Material)
	{
		FString ValidationError;
		if (ValidateMaterial(Material, ValidationError))
		{
			Result.Material = Material;
			Result.bSuccess = true;
			Result.LoadSource = FMaterializeMaterialLoadResult::ELoadSource::Plugin;
			LogMaterialLoadAttempt(PluginPath, true);
			return Result;
		}
		else
		{
			UE_LOG(LogMaterialize, Warning, TEXT("Material loaded but validation failed: %s - %s"), 
				*PluginPath, *ValidationError);
			Result.CompilationErrors.Add(ValidationError);
		}
	}
	
	// Try game override path
	Material = TryLoadMaterialFromPath(GamePath);
	if (Material)
	{
		FString ValidationError;
		if (ValidateMaterial(Material, ValidationError))
		{
			Result.Material = Material;
			Result.bSuccess = true;
			Result.LoadSource = FMaterializeMaterialLoadResult::ELoadSource::GameOverride;
			LogMaterialLoadAttempt(GamePath, true);
			return Result;
		}
		else
		{
			UE_LOG(LogMaterialize, Warning, TEXT("Material loaded but validation failed: %s - %s"), 
				*GamePath, *ValidationError);
			Result.CompilationErrors.Add(ValidationError);
		}
	}
	
	// All paths failed
	Result.ErrorMessage = FString::Printf(
		TEXT("Failed to load master material for preset '%s'. Tried paths:\n  - %s\n  - %s"),
		*PresetName, *PluginPath, *GamePath);
	
	UE_LOG(LogMaterialize, Warning, TEXT("%s"), *Result.ErrorMessage);
	LogMaterialLoadAttempt(PluginPath, false);
	LogMaterialLoadAttempt(GamePath, false);
	
	return Result;
}

bool FMaterializeMaterialLoader::ValidateMaterial(UMaterial* Material, FString& OutError)
{
	if (!Material)
	{
		OutError = TEXT("Material is null");
		return false;
	}
	
	// Check if material is fully loaded
	if (!IsMaterialFullyLoaded(Material))
	{
		OutError = TEXT("Material has pending load flags (RF_NeedLoad or RF_NeedPostLoad)");
		return false;
	}
	
	// Material is valid
	return true;
}

bool FMaterializeMaterialLoader::LoadDefaultTextures(TMap<FName, UTexture*>& OutTextures)
{
	OutTextures.Empty();
	
	// Define default texture paths
	TMap<FName, FString> TexturePaths;
	TexturePaths.Add(TEXT("Normal"), TEXT("/Materialize/Textures/Defaults/T_Default_Normal.T_Default_Normal"));
	TexturePaths.Add(TEXT("ORM"), TEXT("/Materialize/Textures/Defaults/T_Default_ORM.T_Default_ORM"));
	TexturePaths.Add(TEXT("BaseColor"), TEXT("/Materialize/Textures/Defaults/T_Default_BaseColor.T_Default_BaseColor"));
	TexturePaths.Add(TEXT("Black"), TEXT("/Materialize/Textures/Defaults/T_Default_Black.T_Default_Black"));
	TexturePaths.Add(TEXT("White"), TEXT("/Materialize/Textures/Defaults/T_Default_White.T_Default_White"));
	TexturePaths.Add(TEXT("Height"), TEXT("/Materialize/Textures/Defaults/T_Default_Height.T_Default_Height"));
	TexturePaths.Add(TEXT("Emissive"), TEXT("/Materialize/Textures/Defaults/T_Default_Emissive.T_Default_Emissive"));
	
	int32 SuccessCount = 0;
	
	// Attempt to load each texture
	for (const auto& Pair : TexturePaths)
	{
		UTexture* Texture = LoadObject<UTexture>(nullptr, *Pair.Value, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (Texture)
		{
			OutTextures.Add(Pair.Key, Texture);
			SuccessCount++;
			UE_LOG(LogMaterialize, Verbose, TEXT("Loaded default texture: %s from %s"), 
				*Pair.Key.ToString(), *Pair.Value);
		}
		else
		{
			UE_LOG(LogMaterialize, Warning, TEXT("Failed to load default texture: %s from %s"), 
				*Pair.Key.ToString(), *Pair.Value);
		}
	}
	
	if (SuccessCount == 0)
	{
		UE_LOG(LogMaterialize, Error, TEXT("Failed to load any default textures. Plugin content may not be mounted."));
		return false;
	}
	
	UE_LOG(LogMaterialize, Log, TEXT("Loaded %d/%d default textures"), SuccessCount, TexturePaths.Num());
	return true;
}

UMaterial* FMaterializeMaterialLoader::TryLoadMaterialFromPath(const FString& Path)
{
	// Check cache first
	if (UMaterial** CachedMaterial = MaterialCache.Find(Path))
	{
		if (*CachedMaterial && IsValid(*CachedMaterial))
		{
			UE_LOG(LogMaterialize, Verbose, TEXT("Using cached material: %s"), *Path);
			return *CachedMaterial;
		}
		else
		{
			// Remove invalid cached entry
			MaterialCache.Remove(Path);
		}
	}
	
	// Attempt to load material
	UMaterial* Material = LoadObject<UMaterial>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	
	if (Material && IsValid(Material))
	{
		// Cache successful load
		MaterialCache.Add(Path, Material);
		return Material;
	}
	
	return nullptr;
}

bool FMaterializeMaterialLoader::IsMaterialFullyLoaded(UMaterial* Material)
{
	if (!Material)
	{
		return false;
	}
	
	// Check for pending load flags
	if (Material->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
	{
		return false;
	}
	
	return true;
}

void FMaterializeMaterialLoader::LogMaterialLoadAttempt(const FString& Path, bool bSuccess)
{
	if (bSuccess)
	{
		UE_LOG(LogMaterialize, Log, TEXT("Successfully loaded material from: %s"), *Path);
	}
	else
	{
		UE_LOG(LogMaterialize, Verbose, TEXT("Failed to load material from: %s"), *Path);
	}
}
