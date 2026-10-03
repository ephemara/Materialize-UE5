#pragma once

#include "CoreMinimal.h"
#include "Materials/Material.h"

class UTexture;
class UMaterialExpressionTextureSampleParameter2D;

/**
 * Transient material generator with correct sampler types
 * Generates runtime materials when master material cannot be loaded
 * Ensures all texture parameters have correct sampler types to prevent compilation errors
 */
class MATERIALIZE_API FMaterializeTransientGenerator
{
public:
	/**
	 * Generate a transient material for the specified preset type
	 * Creates a material in the transient package with correct sampler types
	 * 
	 * @param PresetType Type of preset to generate (Standard, Metal, Glossy, Toon)
	 * @return Generated transient material or nullptr if generation failed
	 */
	static UMaterial* Generate(const FString& PresetType = TEXT("Standard"));

private:
	/**
	 * Create BaseColor texture parameter with SAMPLERTYPE_Color
	 * 
	 * @param Material Material to add parameter to
	 * @return Created texture parameter expression
	 */
	static UMaterialExpressionTextureSampleParameter2D* CreateBaseColorParameter(UMaterial* Material);
	
	/**
	 * Create Normal texture parameter with SAMPLERTYPE_Normal
	 * 
	 * @param Material Material to add parameter to
	 * @return Created texture parameter expression
	 */
	static UMaterialExpressionTextureSampleParameter2D* CreateNormalParameter(UMaterial* Material);
	
	/**
	 * Create ORM texture parameter with SAMPLERTYPE_LinearColor
	 * 
	 * @param Material Material to add parameter to
	 * @return Created texture parameter expression
	 */
	static UMaterialExpressionTextureSampleParameter2D* CreateORMParameter(UMaterial* Material);
	
	/**
	 * Create Emissive texture parameter with SAMPLERTYPE_Color
	 * 
	 * @param Material Material to add parameter to
	 * @return Created texture parameter expression
	 */
	static UMaterialExpressionTextureSampleParameter2D* CreateEmissiveParameter(UMaterial* Material);
	
	/**
	 * Create Height texture parameter with SAMPLERTYPE_LinearColor
	 * 
	 * @param Material Material to add parameter to
	 * @return Created texture parameter expression
	 */
	static UMaterialExpressionTextureSampleParameter2D* CreateHeightParameter(UMaterial* Material);
	
	/**
	 * Connect material expressions to material outputs
	 * Wires BaseColor, Normal, ORM channels, and Emissive to appropriate outputs
	 * 
	 * @param Material Material to connect outputs for
	 * @param BaseColor BaseColor expression
	 * @param Normal Normal expression
	 * @param ORM ORM expression
	 * @param Emissive Emissive expression
	 */
	static void ConnectMaterialOutputs(
		UMaterial* Material,
		UMaterialExpressionTextureSampleParameter2D* BaseColor,
		UMaterialExpressionTextureSampleParameter2D* Normal,
		UMaterialExpressionTextureSampleParameter2D* ORM,
		UMaterialExpressionTextureSampleParameter2D* Emissive);
	
	/**
	 * Assign default textures to material parameters
	 * Sets T_Default_Normal and T_Default_ORM to prevent null texture errors
	 * 
	 * @param Material Material to assign defaults to
	 * @param Normal Normal parameter expression
	 * @param ORM ORM parameter expression
	 */
	static void AssignDefaultTextures(
		UMaterial* Material,
		UMaterialExpressionTextureSampleParameter2D* Normal,
		UMaterialExpressionTextureSampleParameter2D* ORM);
	
	/**
	 * Compile material and validate for errors
	 * Triggers PostEditChange and checks compilation errors
	 * 
	 * @param Material Material to compile
	 * @param OutError Error message if compilation fails
	 * @return True if compilation succeeded without errors
	 */
	static bool CompileAndValidate(UMaterial* Material, FString& OutError);
	
	/**
	 * Configure material domain and blend mode settings
	 * Sets MaterialDomain, BlendMode, ShadingModel, and TwoSided properties
	 * 
	 * @param Material Material to configure
	 */
	static void ConfigureMaterialSettings(UMaterial* Material);
};
