#include "MaterializePresets.h"

TArray<FMaterializePreset> FMaterializePresets::Presets;
bool FMaterializePresets::bInitialized = false;

void FMaterializePresets::Initialize()
{
	if (bInitialized) return;
	bInitialized = true;

	// ============================================================================
	// ORGANIC PRESETS
	// ============================================================================
	{
		FMaterializeParams P;
		P.NormalStrength = 0.02f;
		P.RoughnessContrast = 1.2f;
		P.RoughnessBrightness = 20.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 0.8f;
		P.BioDetail = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("skin_basic"), TEXT("Basic Skin"), EMaterializeCategory::Organic, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.05f;
		P.RoughnessContrast = 1.5f;
		P.RoughnessBrightness = -10.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.2f;
		P.MetallicBias = -80.0f;
		P.AOIntensity = 1.2f;
		P.EdgeWear = 0.1f;
		P.CavityDirt = 0.2f;
		P.Scratches = 0.2f;
		P.Vignette = 0.2f;
		Presets.Add(FMaterializePreset(TEXT("leather_worn"), TEXT("Worn Leather"), EMaterializeCategory::Organic, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.12f;
		P.RoughnessContrast = 1.0f;
		P.RoughnessBrightness = -20.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 1.5f;
		P.MetallicBias = 20.0f;
		P.AOIntensity = 1.2f;
		P.BioDetail = 0.6f;
		P.BioFrequency = 0.3f;
		P.CavityDirt = 0.3f;
		P.Gamma = 1.1f;
		Presets.Add(FMaterializePreset(TEXT("alien_bio"), TEXT("Alien Flesh"), EMaterializeCategory::Organic, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.15f;
		P.RoughnessContrast = 2.0f;
		P.RoughnessBrightness = 40.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 1.5f;
		P.Dust = 0.2f;
		P.Grunge = 0.3f;
		P.CavityDirt = 0.4f;
		P.Gamma = 0.9f;
		Presets.Add(FMaterializePreset(TEXT("bark"), TEXT("Tree Bark"), EMaterializeCategory::Organic, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.08f;
		P.RoughnessContrast = 1.5f;
		P.RoughnessBrightness = 10.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.1f;
		P.MetallicBias = -90.0f;
		P.AOIntensity = 1.3f;
		P.BioDetail = 0.4f;
		P.CavityDirt = 0.5f;
		P.Gamma = 1.2f;
		P.Vignette = 0.3f;
		Presets.Add(FMaterializePreset(TEXT("zombie"), TEXT("Zombie Skin"), EMaterializeCategory::Organic, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.2f;
		P.RoughnessContrast = 1.8f;
		P.RoughnessBrightness = -20.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.5f;
		P.MetallicBias = -50.0f;
		P.AOIntensity = 1.6f;
		P.BioDetail = 0.2f;
		P.EdgeWear = 0.3f;
		P.CavityDirt = 0.2f;
		Presets.Add(FMaterializePreset(TEXT("dragon_scale"), TEXT("Dragon Scale"), EMaterializeCategory::Organic, P));
	}

	// ============================================================================
	// RUBBER / SYNTH PRESETS
	// ============================================================================
	{
		FMaterializeParams P;
		P.NormalStrength = 0.005f;
		P.RoughnessBase = 0.8f;
		P.RoughnessContrast = 0.5f;
		P.RoughnessBrightness = 20.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 0.5f;
		P.Dust = 0.2f;
		Presets.Add(FMaterializePreset(TEXT("rubber_matte"), TEXT("Matte Rubber"), EMaterializeCategory::Rubber, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.01f;
		P.RoughnessBase = 0.1f;
		P.RoughnessContrast = 0.2f;
		P.RoughnessBrightness = -60.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.5f;
		P.MetallicBias = -40.0f;
		P.AOIntensity = 0.4f;
		Presets.Add(FMaterializePreset(TEXT("latex_shiny"), TEXT("Shiny Latex"), EMaterializeCategory::Rubber, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.08f;
		P.RoughnessContrast = 1.5f;
		P.RoughnessBrightness = 10.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.2f;
		P.MetallicBias = -90.0f;
		P.AOIntensity = 1.2f;
		P.EdgeWear = 0.1f;
		P.CavityDirt = 0.2f;
		P.Scratches = 0.3f;
		P.Dust = 0.4f;
		P.Gamma = 0.9f;
		P.Vignette = 0.2f;
		Presets.Add(FMaterializePreset(TEXT("tire_worn"), TEXT("Worn Tire"), EMaterializeCategory::Rubber, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.03f;
		P.RoughnessContrast = 1.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.1f;
		P.MetallicBias = -80.0f;
		P.AOIntensity = 0.8f;
		P.CavityDirt = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("plastic_rough"), TEXT("Rough Plastic"), EMaterializeCategory::Rubber, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.02f;
		P.RoughnessContrast = 0.8f;
		P.RoughnessBrightness = 10.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 0.6f;
		P.CavityDirt = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("gasket"), TEXT("Gasket"), EMaterializeCategory::Rubber, P));
	}

	// ============================================================================
	// GROUND / ROCK PRESETS
	// ============================================================================
	{
		FMaterializeParams P;
		P.NormalStrength = 0.08f;
		P.RoughnessBase = 0.3f;
		P.RoughnessContrast = 2.5f;
		P.RoughnessBrightness = -30.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.8f;
		P.MetallicBias = -60.0f;
		P.AOIntensity = 1.5f;
		P.BioDetail = 0.1f;
		P.BioFrequency = 0.5f;
		P.CavityDirt = 0.3f;
		P.Dust = 0.1f;
		P.Gamma = 0.9f;
		Presets.Add(FMaterializePreset(TEXT("ground_wet"), TEXT("Wet Mud"), EMaterializeCategory::Ground, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.15f;
		P.RoughnessContrast = 1.8f;
		P.RoughnessBrightness = 60.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 1.8f;
		P.EdgeWear = 0.2f;
		P.CavityDirt = 0.4f;
		P.Dust = 0.3f;
		Presets.Add(FMaterializePreset(TEXT("rock_rough"), TEXT("Rough Rock"), EMaterializeCategory::Ground, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.05f;
		P.RoughnessContrast = 1.2f;
		P.RoughnessBrightness = 30.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 1.0f;
		P.EdgeWear = 0.1f;
		P.CavityDirt = 0.1f;
		P.Dust = 0.1f;
		P.Grunge = 0.1f;
		P.Vignette = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("concrete"), TEXT("Concrete"), EMaterializeCategory::Ground, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.04f;
		P.RoughnessBase = 0.2f;
		P.RoughnessContrast = 0.8f;
		P.RoughnessBrightness = 10.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.5f;
		P.MetallicBias = -50.0f;
		P.AOIntensity = 0.5f;
		P.Gamma = 1.1f;
		Presets.Add(FMaterializePreset(TEXT("snow"), TEXT("Snow"), EMaterializeCategory::Ground, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.1f;
		P.RoughnessContrast = 1.5f;
		P.RoughnessBrightness = 20.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.1f;
		P.MetallicBias = -90.0f;
		P.AOIntensity = 1.2f;
		P.EdgeWear = 0.1f;
		P.CavityDirt = 0.2f;
		P.Noise = 0.2f;
		P.Grunge = 0.2f;
		P.Gamma = 0.9f;
		P.Vignette = 0.2f;
		Presets.Add(FMaterializePreset(TEXT("asphalt"), TEXT("Asphalt"), EMaterializeCategory::Ground, P));
	}

	// ============================================================================
	// FABRIC PRESETS
	// ============================================================================
	{
		FMaterializeParams P;
		P.NormalStrength = 0.06f;
		P.RoughnessContrast = 1.5f;
		P.RoughnessBrightness = 40.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 1.2f;
		P.EdgeWear = 0.1f;
		P.Vignette = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("denim"), TEXT("Denim"), EMaterializeCategory::Fabric, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.02f;
		P.RoughnessBase = 0.2f;
		P.RoughnessContrast = 0.5f;
		P.RoughnessBrightness = -20.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 1.0f;
		P.MetallicBias = -20.0f;
		P.AOIntensity = 0.6f;
		Presets.Add(FMaterializePreset(TEXT("silk"), TEXT("Silk"), EMaterializeCategory::Fabric, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.1f;
		P.RoughnessContrast = 2.0f;
		P.RoughnessBrightness = 50.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 1.5f;
		P.CavityDirt = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("wool"), TEXT("Wool"), EMaterializeCategory::Fabric, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.08f;
		P.RoughnessContrast = 1.2f;
		P.RoughnessBrightness = 30.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 1.0f;
		P.CavityDirt = 0.1f;
		P.Grunge = 0.1f;
		P.Vignette = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("canvas"), TEXT("Canvas"), EMaterializeCategory::Fabric, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.05f;
		P.RoughnessContrast = 1.0f;
		P.RoughnessBrightness = 10.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.5f;
		P.MetallicBias = -60.0f;
		P.AOIntensity = 0.8f;
		P.Gamma = 1.1f;
		P.Vignette = 0.2f;
		Presets.Add(FMaterializePreset(TEXT("velvet"), TEXT("Velvet"), EMaterializeCategory::Fabric, P));
	}

	// ============================================================================
	// METAL PRESETS
	// ============================================================================
	{
		FMaterializeParams P;
		P.NormalStrength = 0.1f;
		P.RoughnessContrast = 2.0f;
		P.RoughnessBrightness = 40.0f;
		P.bRoughnessInvert = true;
		P.MetallicBase = 0.8f;
		P.MetallicContrast = 1.0f;
		P.MetallicBias = 0.0f;
		P.AOIntensity = 1.6f;
		P.CavityDirt = 0.3f;
		P.Scratches = 0.4f;
		P.Gamma = 0.9f;
		P.Vignette = 0.2f;
		Presets.Add(FMaterializePreset(TEXT("iron_rusty"), TEXT("Rusted Iron"), EMaterializeCategory::Metal, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.02f;
		P.RoughnessBase = 0.3f;
		P.RoughnessContrast = 1.5f;
		P.RoughnessBrightness = 20.0f;
		P.bRoughnessInvert = true;
		P.MetallicBase = 1.0f;
		P.MetallicContrast = 0.5f;
		P.MetallicBias = 90.0f;
		P.AOIntensity = 1.0f;
		P.EdgeWear = 0.1f;
		P.CavityDirt = 0.2f;
		P.Scratches = 0.1f;
		P.Vignette = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("gold_dirty"), TEXT("Aged Gold"), EMaterializeCategory::Metal, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.05f;
		P.RoughnessBase = 0.4f;
		P.RoughnessContrast = 1.0f;
		P.RoughnessBrightness = -10.0f;
		P.bRoughnessInvert = true;
		P.MetallicBase = 1.0f;
		P.MetallicContrast = 1.0f;
		P.MetallicBias = 80.0f;
		P.AOIntensity = 0.8f;
		P.EdgeWear = 0.2f;
		P.CavityDirt = 0.1f;
		P.Scratches = 0.5f;
		Presets.Add(FMaterializePreset(TEXT("aluminum_brushed"), TEXT("Brushed Aluminum"), EMaterializeCategory::Metal, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.05f;
		P.RoughnessBase = 0.3f;
		P.RoughnessContrast = 1.2f;
		P.RoughnessBrightness = -10.0f;
		P.bRoughnessInvert = true;
		P.MetallicBase = 0.9f;
		P.MetallicContrast = 2.0f;
		P.MetallicBias = 40.0f;
		P.AOIntensity = 1.2f;
		P.EdgeWear = 0.3f;
		P.CavityDirt = 0.2f;
		P.CyberDetail = 0.35f;
		P.CyberScale = 0.15f;
		P.Scratches = 0.2f;
		P.Vignette = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("scifi_panel"), TEXT("Sci-Fi Panel"), EMaterializeCategory::Metal, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.03f;
		P.RoughnessContrast = 1.2f;
		P.RoughnessBrightness = 10.0f;
		P.bRoughnessInvert = true;
		P.MetallicBase = 0.9f;
		P.MetallicContrast = 0.8f;
		P.MetallicBias = 60.0f;
		P.AOIntensity = 0.9f;
		P.EdgeWear = 0.1f;
		P.CavityDirt = 0.2f;
		P.Vignette = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("copper"), TEXT("Copper"), EMaterializeCategory::Metal, P));
	}

	// ============================================================================
	// PLASTIC PRESETS
	// ============================================================================
	{
		FMaterializeParams P;
		P.NormalStrength = 0.01f;
		P.RoughnessBase = 0.05f;
		P.RoughnessContrast = 0.3f;
		P.RoughnessBrightness = -50.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.2f;
		P.MetallicBias = -80.0f;
		P.AOIntensity = 0.5f;
		Presets.Add(FMaterializePreset(TEXT("plastic_glossy"), TEXT("Glossy Plastic"), EMaterializeCategory::Plastic, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.02f;
		P.RoughnessBase = 0.6f;
		P.RoughnessContrast = 0.8f;
		P.RoughnessBrightness = 10.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.1f;
		P.MetallicBias = -90.0f;
		P.AOIntensity = 0.7f;
		P.CavityDirt = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("plastic_matte"), TEXT("Matte Plastic"), EMaterializeCategory::Plastic, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.01f;
		P.RoughnessBase = 0.2f;
		P.RoughnessContrast = 0.4f;
		P.RoughnessBrightness = -30.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.3f;
		P.MetallicBias = -70.0f;
		P.AOIntensity = 0.6f;
		P.EdgeWear = 0.1f;
		P.CavityDirt = 0.1f;
		P.Scratches = 0.1f;
		P.Gamma = 0.9f;
		P.Vignette = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("bakelite"), TEXT("Bakelite"), EMaterializeCategory::Plastic, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.02f;
		P.RoughnessBase = 0.5f;
		P.RoughnessContrast = 0.6f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.1f;
		P.MetallicBias = -80.0f;
		P.AOIntensity = 0.6f;
		P.CavityDirt = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("pvc"), TEXT("PVC Pipe"), EMaterializeCategory::Plastic, P));
	}

	// ============================================================================
	// PAPER / CARD PRESETS
	// ============================================================================
	{
		FMaterializeParams P;
		P.NormalStrength = 0.06f;
		P.RoughnessContrast = 1.5f;
		P.RoughnessBrightness = 40.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 1.0f;
		P.CavityDirt = 0.1f;
		P.Grunge = 0.1f;
		P.Vignette = 0.1f;
		Presets.Add(FMaterializePreset(TEXT("cardboard"), TEXT("Cardboard"), EMaterializeCategory::Paper, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.02f;
		P.RoughnessContrast = 0.8f;
		P.RoughnessBrightness = 20.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 0.5f;
		Presets.Add(FMaterializePreset(TEXT("paper_clean"), TEXT("Clean Paper"), EMaterializeCategory::Paper, P));
	}
	{
		FMaterializeParams P;
		P.NormalStrength = 0.05f;
		P.RoughnessContrast = 1.8f;
		P.RoughnessBrightness = 30.0f;
		P.bRoughnessInvert = true;
		P.MetallicContrast = 0.0f;
		P.MetallicBias = -100.0f;
		P.AOIntensity = 1.2f;
		P.CavityDirt = 0.2f;
		P.Dust = 0.2f;
		P.Grunge = 0.4f;
		P.Gamma = 0.9f;
		P.Vignette = 0.3f;
		Presets.Add(FMaterializePreset(TEXT("parchment"), TEXT("Old Parchment"), EMaterializeCategory::Paper, P));
	}
}

const TArray<FMaterializePreset>& FMaterializePresets::GetAllPresets()
{
	Initialize();
	return Presets;
}

TArray<FMaterializePreset> FMaterializePresets::GetPresetsByCategory(EMaterializeCategory Category)
{
	Initialize();
	TArray<FMaterializePreset> Result;
	for (const FMaterializePreset& P : Presets)
	{
		if (P.Category == Category)
		{
			Result.Add(P);
		}
	}
	return Result;
}

const FMaterializePreset* FMaterializePresets::GetPresetById(FName Id)
{
	Initialize();
	for (const FMaterializePreset& P : Presets)
	{
		if (P.Id == Id)
		{
			return &P;
		}
	}
	return nullptr;
}

FMaterializeParams FMaterializePresets::GetDefaultParams()
{
	return FMaterializeParams();
}
