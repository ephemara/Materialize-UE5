#pragma once

#include "CoreMinimal.h"
#include "MaterializeTypes.h"

/**
 * 30+ Material Presets ported from autopbr KAutopbrpresets.tsx
 */
class MATERIALIZE_API FMaterializePresets
{
public:
	/** Get all available presets */
	static const TArray<FMaterializePreset>& GetAllPresets();

	/** Get presets by category */
	static TArray<FMaterializePreset> GetPresetsByCategory(EMaterializeCategory Category);

	/** Get a preset by ID */
	static const FMaterializePreset* GetPresetById(FName Id);

	/** Get default params (neutral settings) */
	static FMaterializeParams GetDefaultParams();

private:
	static TArray<FMaterializePreset> Presets;
	static bool bInitialized;
	static void Initialize();
};
