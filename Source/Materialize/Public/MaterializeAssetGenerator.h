// Copyright K-Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterial;
class UMaterialFunction;
class UMaterialExpressionCustom;
class UMaterialExpressionTextureSampleParameter2D;

/**
 * Asset generator for Materialize plugin
 * Programmatically creates Material Functions and Master Materials
 * Runs on plugin startup to ensure all required assets exist
 */
class MATERIALIZE_API FMaterializeAssetGenerator
{
public:
	/**
	 * Initialize and generate all required assets
	 * Called during plugin startup
	 * 
	 * @return True if all assets were created/verified successfully
	 */
	static bool Initialize();
	
	/**
	 * Generate all Material Functions for preset shaders
	 * Creates Material Functions in /Materialize/Content/Materials/Functions/
	 * 
	 * @return True if all functions were created successfully
	 */
	static bool GenerateMaterialFunctions();
	
	/**
	 * Generate all Master Materials for presets
	 * Creates master materials in /Materialize/Content/Materials/
	 * 
	 * @return True if all materials were created successfully
	 */
	static bool GenerateMasterMaterials();
	
	/**
	 * Check if all required assets exist
	 * 
	 * @return True if all assets are present
	 */
	static bool VerifyAssets();

private:
	// Material Function generators
	static UMaterialFunction* CreateMetalAnisotropicSpecularFunction();
	static UMaterialFunction* CreateMetalFresnelRimFunction();
	static UMaterialFunction* CreateGlossyClearCoatFunction();
	static UMaterialFunction* CreateGlossySubsurfaceFunction();
	static UMaterialFunction* CreateGlossyDualLobeFunction();
	static UMaterialFunction* CreateToonCelShadingFunction();
	static UMaterialFunction* CreateToonSpecularFunction();
	static UMaterialFunction* CreateToonRimLightFunction();
	
	// Master Material generators
	static UMaterial* CreateStandardMasterMaterial();
	static UMaterial* CreateMetalMasterMaterial();
	static UMaterial* CreateGlossyMasterMaterial();
	static UMaterial* CreateToonMasterMaterial();
	
	// Helper methods
	static UMaterialFunction* CreateMaterialFunction(
		const FString& FunctionName,
		const FString& ShaderCode,
		const TArray<TPair<FString, FString>>& Inputs, // Name, Type
		const TArray<TPair<FString, float>>& ScalarParams, // Name, Default
		const TArray<TPair<FString, FLinearColor>>& VectorParams // Name, Default
	);
	
	static UMaterialExpressionCustom* AddCustomExpression(
		UMaterial* Material,
		const FString& Code,
		const TArray<FString>& InputNames
	);
	
	static UMaterialExpressionTextureSampleParameter2D* AddTextureParameter(
		UMaterial* Material,
		const FString& ParameterName,
		uint8 SamplerType
	);
	
	static bool SaveAsset(UObject* Asset, const FString& PackagePath);
	
	static FString GetPluginContentPath();
};
