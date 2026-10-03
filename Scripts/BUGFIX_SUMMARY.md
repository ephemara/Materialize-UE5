# Materialize Plugin - Bug Fixes Summary

## Issues Fixed

### 1. Shader Compilation Error
**Problem**: Shader file include path was not updated after rename
- `MaterializeNoiseGenerator.usf` was including `/Plugin/Materialize/KSampleProceduralCommon.ush`
- File was renamed to `MaterializeProceduralCommon.ush`

**Fix**: Updated include path in `Shaders/MaterializeNoiseGenerator.usf`

### 2. Editor Crash on Startup (Assertion Failed)
**Problem**: `ViewportWidget` and `StatusText` were not initialized before use
- `SMaterializeEditor::CreateLayerViewContent()` called `.ToSharedRef()` on uninitialized shared pointers
- Caused assertion: `IsValid() [File:SharedPointer.h] [Line: 1037]`

**Fixes**:
- Changed `ViewportWidget` initialization from `SAssignNew` to `SNew` to properly handle the `OutClient` parameter pattern
- Added `StatusText` initialization in `SMaterializeEditor::Construct()` before UI layout

### 3. Material Preview Not Working
**Problem**: Master material path was not updated after rename
- Code was looking for `/Materialize/Materials/M_KSample_Master.M_KSample_Master`
- Material was renamed to `M_Materialize_Master.uasset`

**Fixes**:
- Updated material path in `SMaterializeEditor::SetPreviewMaterialFromResult()`
- Updated material paths in `MaterializeEngine.cpp` (3 locations)
- Updated comments referencing the old material name

### 4. Parameters Disappearing When Switching Tabs
**Problem**: DetailsView not restored when switching from Graph View back to Layer View
- When switching to Graph View, DetailsView shows selected graph nodes
- When switching back to Layer View, DetailsView was not restored to show ToolModel

**Fix**: Added DetailsView restoration in `SMaterializeEditor::SetWorkflowMode()`
```cpp
// Restore DetailsView to show ToolModel when switching back to Layer View
if (NewMode == EMaterializeWorkflowMode::LayerView) {
  if (DetailsView.IsValid() && ToolModel) {
    DetailsView->SetObject(ToolModel);
  }
}
```

## Files Modified

1. `Shaders/MaterializeNoiseGenerator.usf` - Fixed shader include path
2. `Source/Materialize/Private/SMaterializeEditor.cpp` - Fixed initialization and tab switching
3. `Source/Materialize/Private/MaterializeEngine.cpp` - Fixed master material paths

## Testing Recommendations

1. **Shader Compilation**: Launch Unreal Editor and verify no shader errors
2. **Editor Stability**: Open Materialize editor from toolbar - should not crash
3. **Material Preview**: 
   - Load a texture
   - Click "Generate Preview"
   - Switch between view modes (Material, BaseColor, Normal, etc.)
   - Verify "Material" mode shows the full PBR material
4. **Tab Switching**:
   - Open Materialize editor
   - Verify parameters are visible in Parameters tab
   - Switch to Graph View
   - Switch back to Layer View
   - Verify parameters are still visible
5. **Graph Editor**: Test node creation and connections in Graph View

## Known Issues

- Graph editor may still have some glitches (mentioned by user as "relatively glitchy but working")
- Layer system may need additional stabilization

## Next Steps

1. Close Unreal Editor
2. Rebuild the plugin
3. Test all workflows
4. Address any remaining graph editor glitches
5. Stabilize layer compositor system
