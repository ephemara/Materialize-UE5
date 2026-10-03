# Remaining Build Fixes

## Fixed Issues

### 1. ✅ KAIN Shader Includes
- Fixed all 13 shader .cpp files to include `Shaders/` prefix
- Script: `Scripts/fix_shader_includes.py`

### 2. ✅ MaterializeShaderBindings Files
- Deleted incorrect KAIN-generated module files
- These were conflicting with main module definition

### 3. ✅ SMaterializeEditor.cpp Missing Brace
- Added missing closing brace after `SetPreviewMaterialFromResult()`

## Remaining Issues to Fix

### 4. MaterializeNotifications.cpp - Missing Include
**Error**: `use of undefined type 'SNotificationItem'`
**Fix**: Add `#include "Widgets/Notifications/SNotificationItem.h"` to header

### 5. MaterializeTransientGenerator.cpp - Missing Enum
**Error**: `'MD_Surface': undeclared identifier`
**Fix**: Change `MD_Surface` to `EMaterialDomain::MD_Surface`

### 6. MaterializeGraphValidator.cpp - Wrong Include Path
**Error**: `Cannot open include file: 'Graph/MaterializeGraphNode.h'`
**Fix**: Change to `#include "MaterializeGraphNode.h"` (file is in same directory)

## Quick Fixes

Run these commands to fix the remaining issues:

```powershell
# Fix MaterializeNotifications.h
(Get-Content Source/Materialize/Public/MaterializeNotifications.h) -replace '#include "CoreMinimal.h"', '#include "CoreMinimal.h"`n#include "Widgets/Notifications/SNotificationItem.h"' | Set-Content Source/Materialize/Public/MaterializeNotifications.h

# Fix MaterializeTransientGenerator.cpp
(Get-Content Source/Materialize/Private/MaterializeTransientGenerator.cpp) -replace 'MD_Surface', 'EMaterialDomain::MD_Surface' | Set-Content Source/Materialize/Private/MaterializeTransientGenerator.cpp

# Fix MaterializeGraphValidator.cpp
(Get-Content Source/Materialize/Private/Graph/MaterializeGraphValidator.cpp) -replace '#include "Graph/MaterializeGraphNode.h"', '#include "MaterializeGraphNode.h"' | Set-Content Source/Materialize/Private/Graph/MaterializeGraphValidator.cpp
```

## Summary

Total fixes needed: 6
- ✅ Shader includes (13 files)
- ✅ Module conflict (2 files deleted)
- ✅ Missing brace (1 file)
- ⏳ Missing include (1 file)
- ⏳ Wrong enum (1 file)
- ⏳ Wrong include path (1 file)

After these fixes, the plugin should compile successfully!
