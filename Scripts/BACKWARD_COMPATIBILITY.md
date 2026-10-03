# Backward Compatibility Guide

## Overview

This document explains the backward compatibility support for the Materialize → Materialize plugin rename. The system ensures that existing Materialize assets, configurations, and references continue to work seamlessly after the rename.

## Compatibility Systems

### 1. Asset Redirectors

**Location**: `Config/DefaultMaterialize.ini` and `MaterializeBackwardCompatibility.cpp`

**Purpose**: Automatically redirect old class names to new class names when loading assets.

**How it works**:
- Unreal's CoreRedirects system intercepts asset loading
- When an old class name (e.g., `UMaterializeGraph`) is encountered, it's automatically redirected to the new name (`UMaterializeGraph`)
- This happens transparently - users don't need to manually update their assets

**Supported redirects**:
- All UObject classes (UMaterialize* → UMaterialize*)
- All structs (FMaterialize* → FMaterialize*)
- All enums (EMaterialize* → EMaterialize*)
- Package paths (/Materialize → /Materialize)

### 2. Configuration Migration

**Location**: `MaterializeBackwardCompatibility.cpp::MigrateConfigurationKeys()`

**Purpose**: Migrate user settings from old Materialize config keys to new Materialize keys.

**How it works**:
- On plugin startup, checks for old config keys in `[Materialize]` section
- Copies values to new `[Materialize]` section
- Removes old keys to prevent confusion
- Saves updated configuration

**Migrated settings**:
- Editor settings (output path, resolution, auto-save, preview options)
- Developer settings (support URLs)
- Batch processor settings (output path, resolution, parallel jobs)

### 3. Shader Path Aliases

**Location**: `MaterializeBackwardCompatibility.cpp::RegisterShaderPathAliases()`

**Purpose**: Document shader path mappings for reference.

**How it works**:
- Shader virtual paths are registered in the .uplugin file
- The new plugin registers `/Plugin/Materialize/` as the shader path
- Old references to `/Plugin/Materialize/` in existing materials will need manual update or shader recompilation

**Note**: Shader path aliases are primarily for documentation. Existing materials using the old shader paths should be regenerated or manually updated.

## Testing Backward Compatibility

### Test Scenario 1: Loading Old Graph Assets

1. Create a Materialize graph asset before the rename
2. Rename the plugin to Materialize
3. Load the old graph asset
4. **Expected**: Asset loads without errors, all nodes and connections preserved

### Test Scenario 2: Configuration Preservation

1. Configure Materialize settings (output path, resolution, etc.)
2. Rename the plugin to Materialize
3. Open Materialize editor
4. **Expected**: All settings are preserved and migrated to new config section

### Test Scenario 3: Material Instance Loading

1. Create a material instance using Materialize master material
2. Rename the plugin to Materialize
3. Load the material instance
4. **Expected**: Material loads correctly with redirected class references

## Manual Migration Steps

While most migration is automatic, some manual steps may be required:

### 1. Shader References in Materials

If you have custom materials that directly reference Materialize shaders:
- Open the material in the Material Editor
- Update shader virtual paths from `/Plugin/Materialize/` to `/Plugin/Materialize/`
- Recompile the material

### 2. Blueprint References

If you have blueprints that reference Materialize classes:
- Open the blueprint
- Check for any compilation errors
- The CoreRedirects system should handle most cases automatically
- If errors persist, manually update the class references

### 3. C++ Code References

If you have custom C++ code that references Materialize classes:
- Update `#include` statements from `Materialize*.h` to `Materialize*.h`
- Update class names from `UMaterialize*` to `UMaterialize*`
- Recompile your project

## Troubleshooting

### Issue: "Class not found" errors when loading assets

**Solution**: 
- Check that `Config/DefaultMaterialize.ini` exists and contains the CoreRedirects
- Verify the plugin is properly loaded (check Plugins window)
- Try restarting the editor to ensure CoreRedirects are registered

### Issue: Settings not migrated

**Solution**:
- Check the output log for migration messages
- Manually copy settings from `[Materialize]` to `[Materialize]` in `DefaultEditor.ini`
- Restart the editor

### Issue: Shader compilation errors

**Solution**:
- Clear shader cache: Delete `Saved/ShaderDebugInfo/`
- Force shader recompilation: Console command `recompileshaders changed`
- Verify shader virtual path is registered correctly in the .uplugin file

## Implementation Details

### CoreRedirects Registration

The backward compatibility system uses Unreal's built-in CoreRedirects feature:

```cpp
FCoreRedirect ClassRedirect;
ClassRedirect.RedirectName = ECoreRedirectFlags::Type_Class;
ClassRedirect.OldName = "UMaterializeGraph";
ClassRedirect.NewName = "UMaterializeGraph";
FCoreRedirects::AddRedirectList({ClassRedirect}, TEXT("MaterializeBackwardCompatibility"));
```

### Config Migration

Configuration migration happens during module startup:

```cpp
FString OldValue;
if (GConfig->GetString(TEXT("Materialize"), *OldKey, OldValue, GEditorPerProjectIni))
{
    GConfig->SetString(TEXT("Materialize"), *NewKey, *OldValue, GEditorPerProjectIni);
    GConfig->RemoveKey(TEXT("Materialize"), *OldKey, GEditorPerProjectIni);
}
GConfig->Flush(false, GEditorPerProjectIni);
```

## Deprecation Timeline

- **Current**: Full backward compatibility support active
- **Future**: Backward compatibility will be maintained for at least 2 major versions
- **Removal**: CoreRedirects may be removed in a future major version with advance notice

## Support

If you encounter issues with backward compatibility:
1. Check the output log for error messages
2. Verify your assets are from a compatible Materialize version
3. Report issues with detailed reproduction steps
4. Include the output log and asset details

## Additional Resources

- Unreal Engine CoreRedirects Documentation: https://docs.unrealengine.com/en-US/ProgrammingAndScripting/ProgrammingWithCPP/Assets/CoreRedirects/
- Plugin Migration Guide: See README.md
- Support: Check the plugin's developer settings for support URLs
