#pragma once

#include "CoreMinimal.h"
#include "MaterializeTypes.h"

class UMaterial;
class UTexture;

/**
 * Result structure for material loading operations
 * Contains the loaded material, success status, and diagnostic information
 */
struct FMaterializeMaterialLoadResult
{
	// Loaded material (may be null if loading failed)
	UMaterial* Material = nullptr;
	
	// Success flag
	bool bSuccess = false;
	
	// Error message if loading failed
	FString ErrorMessage;
	
	// Source from which the material was loaded
	enum class ELoadSource
	{
		Plugin,          // Loaded from plugin content path
		GameOverride,    // Loaded from game content override path
		Transient,       // Generated as transient material
		Failed           // All loading attempts failed
	} LoadSource = ELoadSource::Failed;
	
	// Compilation errors (if any occurred during validation)
	TArray<FString> CompilationErrors;
	
	// Check if the result is valid and usable
	bool IsValid() const { return bSuccess && Material != nullptr; }
};

/**
 * Centralized material loading system with robust error handling
 * Handles master material loading with fallback chain and validation
 */
class MATERIALIZE_API FMaterializeMaterialLoader
{
public:
	/**
	 * Load master material with fallback chain
	 * Tries: Plugin path → Game override path → Returns null
	 * 
	 * @param PresetName Name of the preset to load (e.g., "Standard", "Metal")
	 * @return Material load result with diagnostic information
	 */
	static FMaterializeMaterialLoadResult LoadMasterMaterial(const FString& PresetName = TEXT("Standard"));
	
	/**
	 * Validate that a material is fully loaded and ready for use
	 * Checks for RF_NeedLoad and RF_NeedPostLoad flags
	 * 
	 * @param Material Material to validate
	 * @param OutError Error message if validation fails
	 * @return True if material is valid and ready to use
	 */
	static bool ValidateMaterial(UMaterial* Material, FString& OutError);
	
	/**
	 * Load default textures for material parameters
	 * Loads T_Default_Normal and T_Default_ORM from plugin content
	 * 
	 * @param OutTextures Map to populate with loaded textures (key = texture name)
	 * @return True if at least some default textures were loaded successfully
	 */
	static bool LoadDefaultTextures(TMap<FName, UTexture*>& OutTextures);

private:
	/**
	 * Attempt to load material from a specific path
	 * 
	 * @param Path Full asset path to material
	 * @return Loaded material or nullptr if loading failed
	 */
	static UMaterial* TryLoadMaterialFromPath(const FString& Path);
	
	/**
	 * Check if a material is fully loaded (no pending load flags)
	 * 
	 * @param Material Material to check
	 * @return True if material is fully loaded
	 */
	static bool IsMaterialFullyLoaded(UMaterial* Material);
	
	/**
	 * Log a material load attempt with result
	 * 
	 * @param Path Path that was attempted
	 * @param bSuccess Whether the load succeeded
	 */
	static void LogMaterialLoadAttempt(const FString& Path, bool bSuccess);
	
	// Cache of successfully loaded materials to avoid repeated disk access
	static TMap<FString, UMaterial*> MaterialCache;
};
