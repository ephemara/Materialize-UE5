#include "MaterializePresetRegistry.h"
#include "MaterializeValidation.h"

// Initialize static members
TMap<FName, FMaterializeMasterPreset> FMaterializePresetRegistry::PresetMap;
bool FMaterializePresetRegistry::bInitialized = false;

void FMaterializePresetRegistry::Initialize()
{
	if (bInitialized)
	{
		UE_LOG(LogMaterialize, Verbose, TEXT("Preset registry already initialized"));
		return;
	}

	UE_LOG(LogMaterialize, Log, TEXT("Initializing master material preset registry"));

	// Clear any existing presets
	PresetMap.Empty();

	// Register built-in presets
	RegisterBuiltInPresets();

	// Scan for additional presets in the presets folder
	ScanPresetsFolder();

	bInitialized = true;

	UE_LOG(LogMaterialize, Log, TEXT("Preset registry initialized with %d presets"), PresetMap.Num());
}

TArray<FMaterializeMasterPreset> FMaterializePresetRegistry::GetAllPresets()
{
	if (!bInitialized)
	{
		Initialize();
	}

	TArray<FMaterializeMasterPreset> Presets;
	PresetMap.GenerateValueArray(Presets);
	return Presets;
}

const FMaterializeMasterPreset* FMaterializePresetRegistry::GetPreset(const FName& PresetId)
{
	if (!bInitialized)
	{
		Initialize();
	}

	return PresetMap.Find(PresetId);
}

bool FMaterializePresetRegistry::RegisterPreset(const FMaterializeMasterPreset& Preset)
{
	if (Preset.PresetId.IsNone())
	{
		UE_LOG(LogMaterialize, Error, TEXT("Cannot register preset with empty PresetId"));
		return false;
	}

	if (PresetMap.Contains(Preset.PresetId))
	{
		UE_LOG(LogMaterialize, Warning, TEXT("Preset '%s' already registered, overwriting"), *Preset.PresetId.ToString());
	}

	PresetMap.Add(Preset.PresetId, Preset);
	UE_LOG(LogMaterialize, Verbose, TEXT("Registered preset: %s"), *Preset.DisplayName.ToString());
	return true;
}

const FMaterializeMasterPreset& FMaterializePresetRegistry::GetDefaultPreset()
{
	if (!bInitialized)
	{
		Initialize();
	}

	// Return Standard preset as default
	const FMaterializeMasterPreset* StandardPreset = PresetMap.Find(TEXT("Standard"));
	if (StandardPreset)
	{
		return *StandardPreset;
	}

	// Fallback: create a minimal default preset if Standard doesn't exist
	static FMaterializeMasterPreset FallbackPreset(
		TEXT("Standard"),
		TEXT("Standard PBR"),
		TEXT("Standard physically-based rendering material"),
		TEXT("/Materialize/Materials/M_Materialize_Master.M_Materialize_Master")
	);

	UE_LOG(LogMaterialize, Warning, TEXT("Standard preset not found, using fallback"));
	return FallbackPreset;
}

bool FMaterializePresetRegistry::HasPreset(const FName& PresetId)
{
	if (!bInitialized)
	{
		Initialize();
	}

	return PresetMap.Contains(PresetId);
}

void FMaterializePresetRegistry::RegisterBuiltInPresets()
{
	// Standard PBR preset
	{
		FMaterializeMasterPreset StandardPreset(
			TEXT("Standard"),
			TEXT("Standard PBR"),
			TEXT("Standard physically-based rendering material with full PBR workflow support"),
			TEXT("/Materialize/Materials/M_Materialize_Master.M_Materialize_Master")
		);
		RegisterPreset(StandardPreset);
	}

	// Metal preset
	{
		FMaterializeMasterPreset MetalPreset(
			TEXT("Metal"),
			TEXT("Metal (Enhanced Reflections)"),
			TEXT("Optimized for metallic surfaces with enhanced reflections and anisotropic specular"),
			TEXT("/Materialize/Materials/Presets/M_Materialize_Master_Metal.M_Materialize_Master_Metal")
		);
		MetalPreset.bSupportsAnisotropy = true;
		MetalPreset.DefaultScalarParams.Add(TEXT("Metallic_Mult"), 1.5f);
		MetalPreset.DefaultScalarParams.Add(TEXT("Roughness_Mult"), 0.8f);
		RegisterPreset(MetalPreset);
	}

	// Glossy preset
	{
		FMaterializeMasterPreset GlossyPreset(
			TEXT("Glossy"),
			TEXT("Glossy (Clear Coat)"),
			TEXT("High-gloss surfaces like plastic, lacquer, or ceramic with clear coat layer"),
			TEXT("/Materialize/Materials/Presets/M_Materialize_Master_Glossy.M_Materialize_Master_Glossy")
		);
		GlossyPreset.bSupportsClearCoat = true;
		GlossyPreset.bSupportsSubsurface = true;
		GlossyPreset.DefaultScalarParams.Add(TEXT("Roughness_Mult"), 0.3f);
		GlossyPreset.DefaultScalarParams.Add(TEXT("Roughness_Offset"), -0.2f);
		RegisterPreset(GlossyPreset);
	}

	// Toon preset
	{
		FMaterializeMasterPreset ToonPreset(
			TEXT("Toon"),
			TEXT("Toon (Cel-Shaded)"),
			TEXT("Stylized cel-shaded rendering for NPR workflows with configurable lighting bands"),
			TEXT("/Materialize/Materials/Presets/M_Materialize_Master_Toon.M_Materialize_Master_Toon")
		);
		ToonPreset.bSupportsToonShading = true;
		RegisterPreset(ToonPreset);
	}

	UE_LOG(LogMaterialize, Log, TEXT("Registered %d built-in presets"), 4);
}

void FMaterializePresetRegistry::ScanPresetsFolder()
{
	// TODO: Implement preset folder scanning
	// This would scan /Materialize/Content/Materials/Presets/ for additional .uasset files
	// and automatically register them as presets
	// For now, we only support the built-in presets defined above

	UE_LOG(LogMaterialize, Verbose, TEXT("Preset folder scanning not yet implemented"));
}
