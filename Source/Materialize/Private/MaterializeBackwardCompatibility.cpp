// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterializeBackwardCompatibility.h"
#include "UObject/CoreRedirects.h"
#include "Misc/ConfigCacheIni.h"
#include "ShaderCore.h"

void FMaterializeBackwardCompatibility::Initialize()
{
	RegisterAssetRedirectors();
	MigrateConfigurationKeys();
	RegisterShaderPathAliases();
}

void FMaterializeBackwardCompatibility::Shutdown()
{
	// Cleanup if needed
}

void FMaterializeBackwardCompatibility::RegisterAssetRedirectors()
{
	// Get all class redirector mappings
	TMap<FString, FString> Redirectors = GetClassRedirectorMap();
	
	// Register each redirector with Unreal's CoreRedirects system
	TArray<FCoreRedirect> CoreRedirects;
	
	for (const auto& Pair : Redirectors)
	{
		const FString& OldName = Pair.Key;
		const FString& NewName = Pair.Value;
		
		// Class redirector
		FCoreRedirect ClassRedirect(ECoreRedirectFlags::Type_Class, OldName, NewName);
		CoreRedirects.Add(ClassRedirect);
		
		UE_LOG(LogTemp, Log, TEXT("Materialize: Registered class redirector: %s -> %s"), *OldName, *NewName);
	}
	
	// Register all redirectors at once
	FCoreRedirects::AddRedirectList(CoreRedirects, TEXT("MaterializeBackwardCompatibility"));
}

void FMaterializeBackwardCompatibility::MigrateConfigurationKeys()
{
	// Get all config key mappings
	TMap<FString, FString> ConfigMappings = GetConfigKeyMap();
	
	// Migrate each config key
	for (const auto& Pair : ConfigMappings)
	{
		const FString& OldKey = Pair.Key;
		const FString& NewKey = Pair.Value;
		
		// Check if old key exists in config
		FString OldValue;
		if (GConfig->GetString(TEXT("Materialize"), *OldKey, OldValue, GEditorPerProjectIni))
		{
			// Migrate to new key
			GConfig->SetString(TEXT("Materialize"), *NewKey, *OldValue, GEditorPerProjectIni);
			
			// Remove old key
			GConfig->RemoveKey(TEXT("Materialize"), *OldKey, GEditorPerProjectIni);
			
			UE_LOG(LogTemp, Log, TEXT("Materialize: Migrated config key: [Materialize]%s -> [Materialize]%s"), *OldKey, *NewKey);
		}
	}
	
	// Save config changes
	GConfig->Flush(false, GEditorPerProjectIni);
}

void FMaterializeBackwardCompatibility::RegisterShaderPathAliases()
{
	// Get all shader path mappings
	TMap<FString, FString> ShaderPaths = GetShaderPathMap();
	
	// Register shader path aliases
	for (const auto& Pair : ShaderPaths)
	{
		const FString& OldPath = Pair.Key;
		const FString& NewPath = Pair.Value;
		
		// Note: Unreal's shader system doesn't have a direct API for path aliases
		// The shader virtual paths are registered in the .uplugin file
		// This function serves as documentation and can be extended if needed
		
		UE_LOG(LogTemp, Log, TEXT("Materialize: Shader path mapping: %s -> %s"), *OldPath, *NewPath);
	}
}

TMap<FString, FString> FMaterializeBackwardCompatibility::GetClassRedirectorMap()
{
	TMap<FString, FString> Redirectors;
	
	// UObject classes
	Redirectors.Add(TEXT("UMaterializeGraph"), TEXT("UMaterializeGraph"));
	Redirectors.Add(TEXT("UMaterializeGraphSchema"), TEXT("UMaterializeGraphSchema"));
	Redirectors.Add(TEXT("UMaterializeGraphNode"), TEXT("UMaterializeGraphNode"));
	Redirectors.Add(TEXT("UMaterializeGraphNode_Adjustment"), TEXT("UMaterializeGraphNode_Adjustment"));
	Redirectors.Add(TEXT("UMaterializeGraphNode_Blend"), TEXT("UMaterializeGraphNode_Blend"));
	Redirectors.Add(TEXT("UMaterializeGraphNode_ChannelOutput"), TEXT("UMaterializeGraphNode_ChannelOutput"));
	Redirectors.Add(TEXT("UMaterializeGraphNode_Filter"), TEXT("UMaterializeGraphNode_Filter"));
	Redirectors.Add(TEXT("UMaterializeGraphNode_Knot"), TEXT("UMaterializeGraphNode_Knot"));
	Redirectors.Add(TEXT("UMaterializeGraphNode_Math"), TEXT("UMaterializeGraphNode_Math"));
	Redirectors.Add(TEXT("UMaterializeGraphNode_Noise"), TEXT("UMaterializeGraphNode_Noise"));
	Redirectors.Add(TEXT("UMaterializeGraphNode_Output"), TEXT("UMaterializeGraphNode_Output"));
	Redirectors.Add(TEXT("UMaterializeComputeEngine"), TEXT("UMaterializeComputeEngine"));
	Redirectors.Add(TEXT("UMaterializeBatchProcessor"), TEXT("UMaterializeBatchProcessor"));
	Redirectors.Add(TEXT("UMaterializeEditorSettings"), TEXT("UMaterializeEditorSettings"));
	Redirectors.Add(TEXT("UMaterializeDeveloperSettings"), TEXT("UMaterializeDeveloperSettings"));
	Redirectors.Add(TEXT("UMaterializeToolModel"), TEXT("UMaterializeToolModel"));
	Redirectors.Add(TEXT("UKLayerPropertyEditor"), TEXT("UMaterializeLayerPropertyEditor"));
	
	// Struct redirectors (if needed for serialized structs)
	Redirectors.Add(TEXT("FMaterializeParams"), TEXT("FMaterializeParams"));
	Redirectors.Add(TEXT("FMaterializeResult"), TEXT("FMaterializeResult"));
	Redirectors.Add(TEXT("FMaterializePreset"), TEXT("FMaterializePreset"));
	Redirectors.Add(TEXT("FMaterializeGraphContext"), TEXT("FMaterializeGraphContext"));
	Redirectors.Add(TEXT("FMaterializeGraphOutput"), TEXT("FMaterializeGraphOutput"));
	Redirectors.Add(TEXT("FMaterializeLayerStack"), TEXT("FMaterializeLayerStack"));
	Redirectors.Add(TEXT("FMaterializeLayer"), TEXT("FMaterializeLayer"));
	Redirectors.Add(TEXT("FMaterializeProceduralParams"), TEXT("FMaterializeProceduralParams"));
	Redirectors.Add(TEXT("FMaterializeFilterParams"), TEXT("FMaterializeFilterParams"));
	Redirectors.Add(TEXT("FMaterializeAdjustmentParams"), TEXT("FMaterializeAdjustmentParams"));
	
	// Enum redirectors
	Redirectors.Add(TEXT("EMaterializeWorkflowMode"), TEXT("EMaterializeWorkflowMode"));
	Redirectors.Add(TEXT("EMaterializeViewMode"), TEXT("EMaterializeViewMode"));
	Redirectors.Add(TEXT("EMaterializeBlendMode"), TEXT("EMaterializeBlendMode"));
	Redirectors.Add(TEXT("EMaterializeFilterType"), TEXT("EMaterializeFilterType"));
	Redirectors.Add(TEXT("EMaterializeAdjustmentType"), TEXT("EMaterializeAdjustmentType"));
	Redirectors.Add(TEXT("EMaterializeNodeType"), TEXT("EMaterializeNodeType"));
	Redirectors.Add(TEXT("EMaterializePinType"), TEXT("EMaterializePinType"));
	Redirectors.Add(TEXT("EMaterializeLayerType"), TEXT("EMaterializeLayerType"));
	Redirectors.Add(TEXT("EKLayerType"), TEXT("EMaterializeLayerType"));
	
	return Redirectors;
}

TMap<FString, FString> FMaterializeBackwardCompatibility::GetConfigKeyMap()
{
	TMap<FString, FString> ConfigKeys;
	
	// Editor settings
	ConfigKeys.Add(TEXT("DefaultOutputPath"), TEXT("DefaultOutputPath"));
	ConfigKeys.Add(TEXT("DefaultResolution"), TEXT("DefaultResolution"));
	ConfigKeys.Add(TEXT("AutoSaveAssets"), TEXT("AutoSaveAssets"));
	ConfigKeys.Add(TEXT("ShowPreviewGrid"), TEXT("ShowPreviewGrid"));
	ConfigKeys.Add(TEXT("PreviewEnvironment"), TEXT("PreviewEnvironment"));
	
	// Developer settings
	ConfigKeys.Add(TEXT("BugReportURL"), TEXT("BugReportURL"));
	ConfigKeys.Add(TEXT("FeatureRequestURL"), TEXT("FeatureRequestURL"));
	ConfigKeys.Add(TEXT("DiscordURL"), TEXT("DiscordURL"));
	ConfigKeys.Add(TEXT("DocsURL"), TEXT("DocsURL"));
	
	// Batch processor settings
	ConfigKeys.Add(TEXT("BatchOutputPath"), TEXT("BatchOutputPath"));
	ConfigKeys.Add(TEXT("BatchResolution"), TEXT("BatchResolution"));
	ConfigKeys.Add(TEXT("BatchParallelJobs"), TEXT("BatchParallelJobs"));
	
	return ConfigKeys;
}

TMap<FString, FString> FMaterializeBackwardCompatibility::GetShaderPathMap()
{
	TMap<FString, FString> ShaderPaths;
	
	// Shader virtual path mappings
	ShaderPaths.Add(TEXT("/Plugin/Materialize/"), TEXT("/Plugin/Materialize/"));
	ShaderPaths.Add(TEXT("/Materialize/"), TEXT("/Materialize/"));
	
	// Specific shader file mappings
	ShaderPaths.Add(TEXT("/Plugin/Materialize/PBRGenerator.usf"), TEXT("/Plugin/Materialize/PBRGenerator.usf"));
	ShaderPaths.Add(TEXT("/Plugin/Materialize/KSampleProceduralCommon.ush"), TEXT("/Plugin/Materialize/MaterializeProceduralCommon.ush"));
	ShaderPaths.Add(TEXT("/Plugin/Materialize/KSampleBlend.usf"), TEXT("/Plugin/Materialize/MaterializeBlend.usf"));
	ShaderPaths.Add(TEXT("/Plugin/Materialize/KSampleFilters.usf"), TEXT("/Plugin/Materialize/MaterializeFilters.usf"));
	ShaderPaths.Add(TEXT("/Plugin/Materialize/KSampleNoiseGenerator.usf"), TEXT("/Plugin/Materialize/MaterializeNoiseGenerator.usf"));
	ShaderPaths.Add(TEXT("/Plugin/Materialize/SeamlessAndPacking.usf"), TEXT("/Plugin/Materialize/SeamlessAndPacking.usf"));
	
	return ShaderPaths;
}
