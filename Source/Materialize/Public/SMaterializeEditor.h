#pragma once

#include "AdvancedPreviewScene.h"
#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "KLayerStack.h"
#include "MaterializeTypes.h"
#include "SEditorViewport.h"
#include "Widgets/SCompoundWidget.h"
#include "Graph/MaterializeGraphExecutor.h"

class FMaterializeEditorViewportClient;
class UMaterializeToolModel;
class IDetailsView;
class UKLayerEvaluator;
class UKLayerPropertyEditor;

/**
 * Viewport display mode for channel visualization
 */
UENUM()
enum class EMaterializeViewMode : uint8 {
  Material,  // Full PBR material
  BaseColor, // Source texture only
  Normal,    // Normal map
  Roughness, // Roughness channel
  Metallic,  // Metallic channel
  AO,        // Ambient Occlusion
  Height,    // Height/Displacement
  Emissive,  // Emissive channel
  SplitAB    // Side-by-side comparison
};

/**
 * Workflow mode for the editor (Layer-based vs Node-based)
 */
UENUM()
enum class EMaterializeWorkflowMode : uint8 {
  LayerView,  // Layer-based Sampler workflow
  GraphView   // Node-based Designer workflow
};

/**
 * Layer item for UI display (wraps FKLayer pointer for list view)
 */
struct FKLayerListItem {
  int32 Index;
  FKLayer *Layer;

  FKLayerListItem() : Index(INDEX_NONE), Layer(nullptr) {}
  FKLayerListItem(int32 InIndex, FKLayer *InLayer)
      : Index(InIndex), Layer(InLayer) {}
};

/**
 * Main Materialize Editor Widget
 */
class MATERIALIZE_API SMaterializeEditor : public SCompoundWidget, public FGCObject {
public:
  SLATE_BEGIN_ARGS(SMaterializeEditor) {}
  SLATE_END_ARGS()
  void Construct(const FArguments &InArgs);
  virtual ~SMaterializeEditor();

  // --- FGCObject Interface ---
  virtual void AddReferencedObjects(FReferenceCollector &Collector) override;
  virtual FString GetReferencerName() const override;

private:
  // UI Components
  TSharedPtr<class SMaterializeEditorViewport> ViewportWidget;
  TSharedPtr<FMaterializeEditorViewportClient> ViewportClient;
  TSharedPtr<FAdvancedPreviewScene> PreviewScene;
  TSharedPtr<IDetailsView> DetailsView;
  TSharedPtr<class STextBlock> StatusText;
  TSharedPtr<class STextBlock> OutputPathText;

  // Channel View Thumbnails
  TSharedPtr<class SImage> NormalThumbnail;
  TSharedPtr<class SImage> RoughnessThumbnail;
  TSharedPtr<class SImage> MetallicThumbnail;
  TSharedPtr<class SImage> AOThumbnail;
  TSharedPtr<class SImage> HeightThumbnail;
  TSharedPtr<class SImage> EmissiveThumbnail;

  // Thumbnail brushes
  TSharedPtr<struct FSlateBrush> NormalBrush;
  TSharedPtr<struct FSlateBrush> RoughnessBrush;
  TSharedPtr<struct FSlateBrush> MetallicBrush;
  TSharedPtr<struct FSlateBrush> AOBrush;
  TSharedPtr<struct FSlateBrush> HeightBrush;
  TSharedPtr<struct FSlateBrush> EmissiveBrush;

  // Data Model
  TObjectPtr<UMaterializeToolModel> ToolModel;
  TObjectPtr<UStaticMesh> LastPreviewMesh;

  // View State
  EMaterializeViewMode CurrentViewMode = EMaterializeViewMode::Material;
  EMaterializeWorkflowMode CurrentWorkflowMode = EMaterializeWorkflowMode::LayerView;
  FString CurrentEnvironmentName;
  double LastPreviewTime = 0.0;
  TArray<TSharedPtr<FMaterializePreset>> PresetOptions;
  
  // Preset System
  TSharedPtr<SComboBox<TSharedPtr<FMaterializeMasterPreset>>> PresetSelector;
  TArray<TSharedPtr<FMaterializeMasterPreset>> AvailablePresets;
  TSharedPtr<FMaterializeMasterPreset> CurrentPreset;

  // Cached result for channel switching
  FMaterializeResult CachedResult;

  // Debounced live preview — fires 0.15s after last param change
  TWeakPtr<FActiveTimerHandle> PreviewDebounceHandle;
  bool bPreviewDebounceRequested = false;

  // --- Tab System ---
  TSharedPtr<class SWidgetSwitcher> RightPanelSwitcher;
  int32 CurrentTabIndex = 0; // 0 = Parameters, 1 = Layers

  // --- Workflow View Switcher ---
  TSharedPtr<class SWidgetSwitcher> WorkflowContentSwitcher;

  // --- Layer System UI ---
  TSharedPtr<SListView<TSharedPtr<FKLayerListItem>>> LayerListView;
  TArray<TSharedPtr<FKLayerListItem>> LayerListItems;
  int32 SelectedLayerIndex = INDEX_NONE;
  TSharedPtr<IDetailsView> LayerDetailsView;
  TObjectPtr<UKLayerPropertyEditor> LayerPropertyEditor;
  TSharedPtr<SVerticalBox> LayerPropertiesBox;

  /** Graph editor widget instance */
  TSharedPtr<class SGraphEditor> GraphEditorWidget;

  /** Command list used for the graph editor (Delete/Copy/Paste/Duplicate) */
  TSharedPtr<class FUICommandList> GraphEditorCommands;
  TSharedPtr<class SMaterializeNodePalette> NodePalette;
  TObjectPtr<class UMaterializeGraph> CurrentGraph;
  TUniquePtr<class FMaterializeGraphExecutor> GraphExecutor;

  // Internal Methods
  TSharedRef<SWidget> CreateToolbar();
  TSharedRef<SWidget> CreateWorkflowToolbar();
  TSharedRef<SWidget> CreateViewModeToolbar();
  TSharedRef<SWidget> CreateChannelStrip();
  TSharedRef<SWidget> CreateEnvironmentSelector();
  TSharedRef<SWidget> CreatePresetSelector();
  TSharedRef<SWidget> CreateParametersTab();
  TSharedRef<SWidget> CreateLayersTab();
  TSharedRef<SWidget> CreateRightPanel();
  TSharedRef<SWidget> CreateLayerViewContent();
  TSharedRef<SWidget> CreateGraphViewContent();

  void OnModelChanged();
  void MaybeLiveUpdate();
  void OnGeneratePreview();
  void OnGenerateAndSave();
  void SetPreviewMaterialFromResult(struct FMaterializeResult &Result);
  void SetPreviewMaterialFromGraphResult(const struct FMaterializeGraphExecutionResult &GraphResult);
  void SetViewMode(EMaterializeViewMode NewMode);
  void SetWorkflowMode(EMaterializeWorkflowMode NewMode);
  void UpdateChannelThumbnails();
  void ApplyChannelMaterial(EMaterializeViewMode Mode);
  void OnPickTexture();
  void OnLoadSelectedTexture();
  void OnContextTextureChanged(UTexture2D *NewTexture);
  void OnAssetSelected(const FAssetData &AssetData);
  void UpdateMeshPreview();
  void SetEnvironment(const FString &EnvName);
  
  // Preset system helpers
  void InitializePresets();
  void LoadPresetFromEditorPrefs();
  void SavePresetToEditorPrefs();
  void OnPresetSelected(TSharedPtr<FMaterializeMasterPreset> NewPreset, ESelectInfo::Type SelectInfo);
  FText GetCurrentPresetText() const;
  TSharedRef<SWidget> GeneratePresetWidget(TSharedPtr<FMaterializeMasterPreset> Preset);

  // Channel material helpers
  UMaterialInstanceDynamic *
  CreateChannelPreviewMaterial(UTexture2D *ChannelTexture,
                               bool bIsNormal = false);

  // Layer UI helpers
  TSharedRef<ITableRow>
  OnGenerateLayerRow(TSharedPtr<FKLayerListItem> Item,
                     const TSharedRef<STableViewBase> &OwnerTable);
  void OnLayerSelectionChanged(TSharedPtr<FKLayerListItem> Item,
                               ESelectInfo::Type SelectInfo);
  void RefreshLayerList();
  void AddLayer(EKLayerType Type);
  void RemoveSelectedLayer();
  void DuplicateSelectedLayer();
  void MoveLayerUp();
  void MoveLayerDown();
  void OnLayerStackChanged();
  void EvaluateLayerStack();
  void UpdateSelectedLayerProperties();

  // Graph editor helpers
  void InitializeGraphEditor();
  void OnGraphNodeSelectionChanged(const TSet<UObject*>& SelectedNodes);
  void OnGraphNodePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent);
  void ExecuteGraph();
  void InvalidateGraphPreview();
  void BindGraphCommands();
  void DeleteSelectedGraphNodes();
  void CopySelectedGraphNodes();
  void PasteGraphNodes();
  void DuplicateGraphNodes();
};

/**
 * Viewport widget for the Materialize Editor
 */
class MATERIALIZE_API SMaterializeEditorViewport : public SEditorViewport {
public:
  SLATE_BEGIN_ARGS(SMaterializeEditorViewport) {}
  SLATE_END_ARGS()
  void Construct(const FArguments &InArgs,
                 TSharedPtr<FAdvancedPreviewScene> InPreviewScene,
                 TSharedPtr<FMaterializeEditorViewportClient> &OutClient);

protected:
  virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
  TSharedPtr<FMaterializeEditorViewportClient> Client;
  TSharedPtr<FAdvancedPreviewScene> PreviewScene;
};
