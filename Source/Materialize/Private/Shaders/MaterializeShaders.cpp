// Copyright K-Studio. All Rights Reserved.

#include "Shaders/MaterializeShaders.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"

// ============================================================================
// SHADER IMPLEMENTATIONS
// ============================================================================

IMPLEMENT_GLOBAL_SHADER(FMaterializeNoiseGeneratorCS, "/Plugin/Materialize/MaterializeNoiseGenerator.usf", "GenerateNoiseCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FMaterializeBlendCS, "/Plugin/Materialize/MaterializeBlend.usf", "BlendTexturesCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FMaterializeBlurHorizontalCS, "/Plugin/Materialize/MaterializeFilters.usf", "BlurHorizontalCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FMaterializeBlurVerticalCS, "/Plugin/Materialize/MaterializeFilters.usf", "BlurVerticalCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FMaterializeSharpenCS, "/Plugin/Materialize/MaterializeFilters.usf", "SharpenCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FMaterializeEdgeDetectCS, "/Plugin/Materialize/MaterializeFilters.usf", "EdgeDetectCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FMaterializeLevelsCS, "/Plugin/Materialize/MaterializeFilters.usf", "LevelsCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FMaterializeHSLAdjustCS, "/Plugin/Materialize/MaterializeFilters.usf", "HSLAdjustCS", SF_Compute);
