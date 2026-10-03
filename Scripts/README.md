# Materialize: Complete Material Creation Suite

**GPU-Accelerated PBR Material Generation & Procedural Authoring for Unreal Engine 5**

Materialize combines two powerful workflows into one unified plugin:
1. **K-Sampler**: Photo-to-PBR extraction (like Substance Sampler)
2. **K-Designer**: Node-based procedural materials (like Substance Designer)

---

## 🏗️ Architecture Overview

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│                              K-SAMPLE PLUGIN                                      │
├──────────────────────────────────────────────────────────────────────────────────┤
│                                                                                   │
│  ┌─────────────────────────┐          ┌─────────────────────────┐                │
│  │    Sampler Workflow     │          │    Designer Workflow    │                │
│  │   (Photo → PBR Maps)    │          │  (Procedural Nodes)     │                │
│  ├─────────────────────────┤          ├─────────────────────────┤                │
│  │ SMaterializeEditor.cpp      │          │ KSampleGraphEditorToolkit│               │
│  │ (Main UI - 34KB)        │          │ (Graph UI - 9KB)        │                │
│  │                         │          │                         │                │
│  │ • Texture slot input    │          │ • Node-based canvas     │                │
│  │ • Parameter sliders     │          │ • Visual connections    │                │
│  │ • Material presets      │          │ • Live node previews    │                │
│  │ • Channel thumbnails    │          │ • 3D preview viewport   │                │
│  │ • 3D mesh preview       │          │ • Graph compilation     │                │
│  └───────────┬─────────────┘          └───────────┬─────────────┘                │
│              │                                    │                              │
│              ▼                                    ▼                              │
│  ┌───────────────────────────────────────────────────────────────────┐          │
│  │                    SHARED PROCESSING CORE                         │          │
│  ├───────────────────────────────────────────────────────────────────┤          │
│  │                                                                   │          │
│  │  ┌─────────────────────┐    ┌─────────────────────┐              │          │
│  │  │  KLayerEvaluator    │    │ KSampleComputeEngine│              │          │
│  │  │  (20KB)             │    │ (37KB)              │              │          │
│  │  │                     │    │                     │              │          │
│  │  │ • Stack evaluation  │    │ • RDG GPU pipeline  │              │          │
│  │  │ • Blend operations  │    │ • Poisson solver    │              │          │
│  │  │ • Filter/adjustment │    │ • Jacobi iteration  │              │          │
│  │  │ • Procedural noise  │    │ • Seamless tiling   │              │          │
│  │  └─────────────────────┘    └─────────────────────┘              │          │
│  │              │                        │                          │          │
│  │              ▼                        ▼                          │          │
│  │  ┌─────────────────────────────────────────────────────┐        │          │
│  │  │              Compute Shaders (.usf)                 │        │          │
│  │  │   PBRGenerator.usf • LayerBlend.usf • Noise.usf    │        │          │
│  │  └─────────────────────────────────────────────────────┘        │          │
│  └───────────────────────────────────────────────────────────────────┘          │
│                                                                                   │
│                                 OUTPUT                                            │
│  ┌───────────────────────────────────────────────────────────────────┐          │
│  │ FMaterializeResult / FKLayerEvalResult                                │          │
│  │ • BaseColor, Normal, Roughness, Metallic, Height, AO, Emissive   │          │
│  │ • ORM Packed texture                                              │          │
│  │ • Material Instance                                               │          │
│  └───────────────────────────────────────────────────────────────────┘          │
└──────────────────────────────────────────────────────────────────────────────────┘
```

---

## 📁 File Structure & Responsibilities

### Data Types (`Public/`)

| File | Purpose |
|------|---------|
| `KSampleTypes.h` | Core structs: `FMaterializeParams`, `FMaterializeResult`, `FMaterializePreset`, enums |
| `KLayerStack.h` | Layer system: `FKLayer`, `FKLayerStack`, blend modes, noise types, filters |

### Processing Engines (Shared Core)

| File | Purpose | Size |
|------|---------|------|
| `KSampleComputeEngine.cpp` | **GPU pipeline via RDG**. Gradient, Poisson, Jacobi passes. | 37KB |
| `KLayerEvaluator.cpp` | **Layer stack compositor**. Blend textures, procedural noise, filters. | 20KB |
| `KSampleEngine.cpp` | **Legacy CPU fallback**. Sobel, edge detection, manual loops. | 31KB |

### Sampler Workflow (Photo → PBR)

| File | Purpose | Size |
|------|---------|------|
| `SMaterializeEditor.cpp` | Main editor UI. Texture input, sliders, presets, viewport. | 34KB |
| `KSampleAssetActions.cpp` | Context menu: "Generate PBR Material" on texture right-click. | 8KB |
| `SMaterializeBatchWindow.cpp` | Bulk processing UI. Drag folders, async queue. | 20KB |
| `KSampleBatchProcessor.cpp` | Batch job execution. | 16KB |
| `KSamplePresets.cpp` | Material presets (Organic, Metal, Ground, etc). | 16KB |

### Designer Workflow (Procedural Nodes)

| File | Purpose |
|------|---------|
| `Graph/KSampleGraph.cpp` | UObject asset for the node graph |
| `Graph/KSampleGraphSchema.cpp` | Pin connections, node spawning rules |
| `Graph/KSampleGraphExecutor.cpp` | Traverse graph, dispatch GPU shaders per node |
| `Graph/KSampleGraphCompiler.cpp` | Validate and compile graph topology |
| `Graph/Nodes/KSampleGraphNode.cpp` | Base node class with preview texture |
| `Graph/Nodes/KSampleGraphNode_Noise.cpp` | Procedural noise generator node |
| `Graph/Nodes/KSampleGraphNode_Blend.cpp` | Blend two textures node |
| `Graph/Nodes/KSampleGraphNode_Filter.cpp` | Blur, sharpen, edge detect node |
| `Graph/Nodes/KSampleGraphNode_Math.cpp` | Add, subtract, multiply textures |
| `Graph/Nodes/KSampleGraphNode_Output.cpp` | PBR output node with bake function |
| `Editor/KSampleGraphEditorToolkit.cpp` | Graph editor window with tabs |
| `Editor/Widgets/SMaterializeGraphNode.cpp` | Visual node widget with live preview |
| `Editor/SMaterialize3DPreviewViewport.cpp` | 3D material preview in graph editor |
| `Editor/Preview/KSamplePreviewViewport.cpp` | Per-node texture preview renderer |

### Shaders

```
Shaders/
├── PBRGenerator.usf           (18KB) - Main PBR pipeline: gradient, height, normal passes
├── KSampleBlend.usf           (6KB)  - 20+ Photoshop blend modes
├── KSampleFilters.usf         (10KB) - Blur, sharpen, edge, emboss, warp
├── KSampleNoiseGenerator.usf  (7KB)  - Procedural noise dispatch
├── KSampleProceduralCommon.ush(9KB)  - Shared noise functions (Perlin, Voronoi, FBM)
├── SeamlessAndPacking.usf     (6KB)  - Tileable textures, ORM packing
│
└── KStudioCore/               (Shared with other K-Studio plugins)
    ├── LayerBlend.usf         (7KB)  - Core blend mode implementations
    ├── LayerFilter.usf        (7KB)  - Convolution kernels, blur, sharpen
    ├── LayerAdjustment.usf    (8KB)  - Levels, curves, HSV, color balance
    └── ProceduralNoise.usf    (12KB) - Perlin, Simplex, Voronoi, Cellular, FBM
```

| Shader | Purpose |
|--------|---------|
| `PBRGenerator.usf` | Multi-pass GPU pipeline: Sobel gradients → Jacobi height solver → Poisson normal reconstruction → channel outputs |
| `KSampleBlend.usf` | Blend modes: Normal, Multiply, Screen, Overlay, Soft Light, Hard Light, Color Dodge, Color Burn, Difference, Exclusion, Linear Light, Pin Light, Vivid Light, Hard Mix |
| `KSampleFilters.usf` | Image filters: Gaussian blur (separable), sharpen, high pass, edge detect, emboss, directional blur, warp/distort |
| `KSampleNoiseGenerator.usf` | Compute shader entry points for procedural texture generation |
| `KSampleProceduralCommon.ush` | Include file with noise primitives: `PerlinNoise()`, `VoronoiNoise()`, `FBM()`, `CellularNoise()`, `SimplexNoise()` |
| `SeamlessAndPacking.usf` | Cross-blend and mirror-blend tiling algorithms, ORM channel packing (R=AO, G=Roughness, B=Metallic) |
| `KStudioCore/LayerBlend.usf` | Shared blend mode functions used by multiple K-Studio plugins |
| `KStudioCore/LayerFilter.usf` | Shared filter kernels: box blur, gaussian, laplacian, sobel |
| `KStudioCore/LayerAdjustment.usf` | Color adjustments: levels (black/white/gamma), HSV shift, brightness/contrast, posterize, gradient map |
| `KStudioCore/ProceduralNoise.usf` | Comprehensive noise library with tiling support and anti-aliasing |

---

## 🔄 Data Flow

### Sampler Workflow
```
User drops texture → SMaterializeEditor
                         │
                         ▼
              FMaterializeParams (sliders)
                         │
                         ▼
         UMaterializeComputeEngine::GeneratePBRMapsGPU()
                         │
                         ├── Gradient Pass (Sobel)
                         ├── Height Pass (Jacobi iteration)
                         ├── Normal Pass (Poisson reconstruction)
                         ├── Roughness/Metallic/AO Pass
                         └── ORM Packing
                         │
                         ▼
               FMaterializeResult (transient textures)
                         │
                         ▼
              SMaterializeEditor preview viewport
                         │
                         ▼ (on Save)
              Persistent UTexture2D assets + Material Instance
```

### Designer Workflow
```
User creates UMaterializeGraph asset
                    │
                    ▼
         FMaterializeGraphEditorToolkit opens
                    │
                    ▼
    User adds nodes (Noise, Blend, Filter, Math, Output)
                    │
                    ▼
         User clicks "Compile"
                    │
                    ▼
    FMaterializeGraphExecutor::ExecuteWithPreviews()
                    │
                    ├── For each node (topological order):
                    │      └── UKLayerEvaluator::GenerateProceduralTexture()
                    │      └── UKLayerEvaluator::BlendTextures()
                    │      └── UKLayerEvaluator::ApplyFilter()
                    │
                    ▼
    FMaterializeGraphExecutionResult (per-channel textures)
                    │
                    ├── Node previews updated
                    └── 3D viewport material updated
                    │
                    ▼ (on Bake)
    UMaterializeGraphNode_Output::BakeToTextures()
                    │
                    ▼
    Persistent UTexture2D assets
```

---

## 🧩 Key Classes

### `FMaterializeParams`
Generation parameters. Maps to compute shader uniforms.
- Normal strength, roughness base/range, metallic bias
- AO intensity, height contrast
- Weathering: edge wear, grunge, bio/cyber detail
- Output: resolution, ORM packing, seamless mode

### `FKLayerStack`
Ordered list of `FKLayer` objects. Evaluated bottom-to-top.
- Layer types: Image, Procedural, Filter, Adjustment, Generator
- Blend modes: Normal, Multiply, Screen, Overlay, Soft Light...
- Per-layer opacity, mask, output channel flags

### `UKLayerEvaluator`
Static utility for GPU texture operations.
- `EvaluateStack()` - Process full layer stack
- `BlendTextures()` - Two-texture blend with mode
- `GenerateProceduralTexture()` - Noise generation
- `ApplyFilter()` - Blur, sharpen, etc.
- `ApplyAdjustment()` - Levels, curves, HSV

### `UMaterializeComputeEngine`
RDG-based PBR generation.
- `GeneratePBRMapsGPU()` - Full pipeline
- `MakeSeamless()` - Tileable textures
- `PackORM()` - Channel packing

### `FMaterializeGraphExecutor`
Node graph runner.
- `Execute()` - Run graph, return channel textures
- `ExecuteWithPreviews()` - Run graph, update node thumbnails
- Recursive traversal from Output node backward

---

## ⚡ GPU Pipeline (RDG)

The compute engine uses Unreal's Render Dependency Graph:

1. **Source Upload** - Upload source texture to GPU
2. **Grayscale** - Convert to luminance for processing
3. **Gradient** - Sobel derivatives (dx, dy)
4. **Height** - Multi-pass Jacobi iteration (24 passes default)
5. **Normal** - Reconstruct from height gradients
6. **Roughness** - Edge-based estimation
7. **Metallic** - Specular analysis
8. **AO** - Height-based occlusion
9. **Emissive** - Bright area extraction
10. **ORM Pack** - Channel packing (R=AO, G=Rough, B=Metal)

All passes run on GPU compute shaders in a single RDG submission.

---

## 📦 Asset Types

| Asset | Class | Editor |
|-------|-------|--------|
| Materialize Preset | `UMaterializeAsset` | `SMaterializeEditor` (Tools menu) |
| Materialize Graph | `UMaterializeGraph` | `FMaterializeGraphEditorToolkit` |

Both create in Content Browser via right-click → K-Studio.

---

## 🎯 Entry Points

### For Users
1. **Right-click texture** → "Generate PBR Material" (instant)
2. **Tools → K-Studio → Materialize Editor** (manual control)
3. **Tools → K-Studio → Batch Processor** (bulk)
4. **Content Browser → Create Materialize Graph** (procedural)

### For Developers
```cpp
// Quick generation
FMaterializeResult Result;
UMaterializeComputeEngine::GeneratePBRMapsGPU(SourceTexture, Params, Result);

// Layer stack
FKLayerStack Stack;
Stack.Layers.Add(MakeNoiseLayer());
Stack.Layers.Add(MakeBlendLayer());
FKLayerEvalResult EvalResult;
UKLayerEvaluator::EvaluateStack(Stack, EvalResult);

// Graph execution
FMaterializeGraphExecutor Executor;
FMaterializeGraphExecutionResult GraphResult = Executor.Execute(Graph, 1024, 1024);
```

---

## 🔌 Dependencies

- **KStudioCore** - Shared utilities (optional, can run standalone)
- **No external tools** - No Python, Substance, or third-party SDKs

---

## 📊 Module Statistics

| Component | Files | Lines | Size |
|-----------|-------|-------|------|
| Core Types | 2 | ~800 | 27KB |
| Engines | 3 | ~2800 | 89KB |
| Sampler Editor | 5 | ~2500 | 78KB |
| Graph System | 14 | ~1800 | 50KB |
| Shaders | 4 | ~600 | 15KB |
| **Total** | **~28** | **~8500** | **259KB** |

---

**Built by K-Studio**
*Targeting UE 5.4+*
