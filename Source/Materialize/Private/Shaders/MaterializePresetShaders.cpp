// Copyright K-Studio. All Rights Reserved.
// Master registration file for all KAIN-generated preset shaders

#include "Shaders/MaterializePresetShaders.h"

// Include all generated shader implementations
#include "Shaders/MetalAnisotropicSpecular.h"
#include "Shaders/MetalFresnelRim.h"
#include "Shaders/GlossyClearCoat.h"
#include "Shaders/GlossySubsurface.h"
#include "Shaders/GlossyDualLobe.h"
#include "Shaders/ToonCelShading.h"
#include "Shaders/ToonSpecular.h"
#include "Shaders/ToonRimLight.h"
#include "Shaders/ToonOutlineDetection.h"
#include "Shaders/ToonConfigurableBands.h"
#include "Shaders/MaterializeFresnelSchlick.h"
#include "Shaders/MaterializeGGXDistribution.h"
#include "Shaders/MaterializeSmithVisibility.h"

// Note: Individual shader implementations are in their respective .cpp files
// This file serves as a central include point for all preset shaders
