// Copyright K-Studio. All Rights Reserved.
// Master header for all KAIN-generated preset shaders

#pragma once

#include "CoreMinimal.h"

// Metal Preset Shaders
#include "Shaders/MetalAnisotropicSpecular.h"
#include "Shaders/MetalFresnelRim.h"

// Glossy Preset Shaders
#include "Shaders/GlossyClearCoat.h"
#include "Shaders/GlossySubsurface.h"
#include "Shaders/GlossyDualLobe.h"

// Toon Preset Shaders
#include "Shaders/ToonCelShading.h"
#include "Shaders/ToonSpecular.h"
#include "Shaders/ToonRimLight.h"
#include "Shaders/ToonOutlineDetection.h"
#include "Shaders/ToonConfigurableBands.h"

// Shared Utility Shaders
#include "Shaders/MaterializeFresnelSchlick.h"
#include "Shaders/MaterializeGGXDistribution.h"
#include "Shaders/MaterializeSmithVisibility.h"

/**
 * Preset Shader Overview:
 * 
 * METAL PRESET:
 * - MetalAnisotropicSpecular: Enhanced metallic reflections with anisotropic specular
 * - MetalFresnelRim: Fresnel-based edge highlights for metals
 * 
 * GLOSSY PRESET:
 * - GlossyClearCoat: Clear coat layer with IOR-based reflections
 * - GlossySubsurface: Subsurface scattering approximation for translucent materials
 * - GlossyDualLobe: Dual-lobe specular (base + coat) with energy conservation
 * 
 * TOON PRESET:
 * - ToonCelShading: Cel-shaded lighting with configurable bands
 * - ToonSpecular: Stepped specular highlights
 * - ToonRimLight: Hard-edge rim lighting
 * - ToonOutlineDetection: Depth/normal-based outline detection
 * - ToonConfigurableBands: Custom band positions and colors
 * 
 * SHARED UTILITIES:
 * - MaterializeFresnelSchlick: Schlick approximation for Fresnel
 * - MaterializeGGXDistribution: GGX normal distribution function
 * - MaterializeSmithVisibility: Smith visibility term for PBR
 * 
 * Usage:
 * These shaders are designed to be called from Material Functions in Unreal Editor.
 * They provide GPU-accelerated lighting calculations for preset materials.
 * 
 * Integration:
 * 1. Create Material Functions that call these shaders
 * 2. Use Material Functions in master materials (M_Materialize_Master_Metal, etc.)
 * 3. Preset system automatically loads correct master material based on user selection
 */
