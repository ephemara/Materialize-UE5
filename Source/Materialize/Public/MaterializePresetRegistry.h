#pragma once

#include "CoreMinimal.h"
#include "MaterializeTypes.h"

/**
 * Registry for master material presets
 * Manages available master material presets and provides access to preset descriptors
 * Supports both built-in presets and user-defined presets
 */
class MATERIALIZE_API FMaterializePresetRegistry
{
public:
	/**
	 * Initialize the preset registry
	 * Scans the presets folder and registers all available master materials
	 * Should be called during plugin startup
	 */
	static void Initialize();

	/**
	 * Get all available presets
	 * 
	 * @return Array of all registered preset descriptors
	 */
	static TArray<FMaterializeMasterPreset> GetAllPresets();

	/**
	 * Get a specific preset by ID
	 * 
	 * @param PresetId Unique identifier for the preset
	 * @return Pointer to preset descriptor, or nullptr if not found
	 */
	static const FMaterializeMasterPreset* GetPreset(const FName& PresetId);

	/**
	 * Register a new preset (for extensibility)
	 * Allows users to add custom presets at runtime
	 * 
	 * @param Preset Preset descriptor to register
	 * @return True if registration succeeded
	 */
	static bool RegisterPreset(const FMaterializeMasterPreset& Preset);

	/**
	 * Get the default preset (Standard PBR)
	 * 
	 * @return Reference to the default preset descriptor
	 */
	static const FMaterializeMasterPreset& GetDefaultPreset();

	/**
	 * Check if a preset exists
	 * 
	 * @param PresetId Unique identifier for the preset
	 * @return True if the preset is registered
	 */
	static bool HasPreset(const FName& PresetId);

private:
	/**
	 * Register built-in presets
	 * Creates descriptors for Standard, Metal, Glossy, and Toon presets
	 */
	static void RegisterBuiltInPresets();

	/**
	 * Scan the presets folder for additional preset assets
	 * Looks in /Materialize/Content/Materials/Presets/
	 */
	static void ScanPresetsFolder();

	// Map of preset ID to preset descriptor
	static TMap<FName, FMaterializeMasterPreset> PresetMap;

	// Flag to track if registry has been initialized
	static bool bInitialized;
};
