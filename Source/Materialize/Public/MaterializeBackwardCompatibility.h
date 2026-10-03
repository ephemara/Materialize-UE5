// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Backward Compatibility Support for Materialize -> Materialize Rename
 * 
 * This module provides asset redirectors, config migration, and shader path aliases
 * to ensure old Materialize assets continue to work after the plugin rename.
 */
class MATERIALIZE_API FMaterializeBackwardCompatibility
{
public:
	/**
	 * Register asset redirectors for old Materialize class names.
	 * This allows old assets to load correctly with the new Materialize class names.
	 */
	static void RegisterAssetRedirectors();
	
	/**
	 * Migrate old Materialize configuration keys to new Materialize keys.
	 * This ensures user settings are preserved after the rename.
	 */
	static void MigrateConfigurationKeys();
	
	/**
	 * Register deprecated shader virtual path aliases.
	 * This allows old shader references to continue working.
	 */
	static void RegisterShaderPathAliases();
	
	/**
	 * Initialize all backward compatibility systems.
	 * Should be called during module startup.
	 */
	static void Initialize();
	
	/**
	 * Cleanup backward compatibility systems.
	 * Should be called during module shutdown.
	 */
	static void Shutdown();

private:
	// Asset redirector mappings (old class name -> new class name)
	static TMap<FString, FString> GetClassRedirectorMap();
	
	// Config key mappings (old key -> new key)
	static TMap<FString, FString> GetConfigKeyMap();
	
	// Shader path mappings (old path -> new path)
	static TMap<FString, FString> GetShaderPathMap();
};
