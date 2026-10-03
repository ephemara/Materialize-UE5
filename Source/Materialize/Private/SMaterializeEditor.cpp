#include "SMaterializeEditor.h"
#include "AssetViewerSettings.h"
#include "Brushes/SlateColorBrush.h"
#include "ContentBrowserModule.h"
#include "EditorStyleSet.h"
#include "Engine/EngineTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"
#include "Framework/Notifications/NotificationManager.h"
#include "EdGraphUtilities.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditor.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Graph/MaterializeGraphExecutor.h"
#include "Graph/MaterializeGraphSchema.h"
#include "HAL/PlatformTime.h"
#include "IContentBrowserSingleton.h"
#include "IDetailsView.h"
#include "KLayerEvaluator.h"
#include "UKLayerPropertyEditor.h"
#include "MaterializeComputeEngine.h"
#include "MaterializeEditorContext.h"
#include "MaterializeEditorViewportClient.h"
#include "MaterializeEngine.h"
#include "MaterializePresets.h"
#include "MaterializeToolModel.h"
#include "MaterializeMaterialLoader.h"
#include "MaterializeTransientGenerator.h"
#include "MaterializeValidation.h"
#include "MaterializePresetRegistry.h"
#include "MaterialDomain.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Editor/SMaterializeNodePalette.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

// Environment map paths
static const TCHAR *GEnvironmentMaps[] = {
    TEXT("/Materialize/StudioSmall.StudioSmall"),
    TEXT("/Engine/EngineSky/CloudySky/CloudySky.CloudySky"),
    TEXT("/Engine/MapTemplates/Sky/"
         "DaylightAmbientCubemap.DaylightAmbientCubemap"),
};

static const TCHAR *GEnvironmentNames[] = {
    TEXT("Studio"),
    TEXT("Cloudy Sky"),
    TEXT("Daylight"),
};

static const FString GDefaultEnvironmentName(TEXT("Daylight"));

void SMaterializeEditor::Construct(const FArguments &InArgs) {
  // 1. Initialize Data Model
  ToolModel = NewObject<UMaterializeToolModel>();
  ToolModel->AddToRoot(); // Prevent GC
  ToolModel->Params = FMaterializePresets::GetDefaultParams();
  ToolModel->OnModelChanged.AddSP(this, &SMaterializeEditor::OnModelChanged);
  FMaterializeEditorContext::OnTextureChanged.AddSP(
      this, &SMaterializeEditor::OnContextTextureChanged);

  // Initialize preset system
  InitializePresets();
  LoadPresetFromEditorPrefs();

  // 2. Initialize Viewport
  PreviewScene =
      MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
  PreviewScene->SetFloorVisibility(false);

  // Set default environment map (Epic/Daylight style)
  CurrentEnvironmentName = GDefaultEnvironmentName;
  SetEnvironment(CurrentEnvironmentName);

  // Create viewport widget (it will create and return the ViewportClient)
  ViewportWidget = SNew(SMaterializeEditorViewport, PreviewScene, ViewportClient);

  // Initialize status text widget
  StatusText = SNew(STextBlock)
    .Text(FText::FromString(TEXT("Ready")))
    .Font(FAppStyle::Get().GetFontStyle("NormalFont"))
    .ColorAndOpacity(FLinearColor::White)
    .ShadowOffset(FVector2D(1, 1));

  // Load default mesh
  if (!ToolModel->SourceStaticMesh) {
    ToolModel->SourceStaticMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  }
  UpdateMeshPreview();

  // 3. Initialize Details View (Parameters)
  FPropertyEditorModule &PropertyEditorModule =
      FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
  FDetailsViewArgs DetailsViewArgs;
  DetailsViewArgs.bUpdatesFromSelection = false;
  DetailsViewArgs.bLockable = false;
  DetailsViewArgs.bAllowSearch = true;
  DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
  DetailsViewArgs.bHideSelectionTip = true;
  DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
  DetailsView->SetObject(ToolModel);

  // 4. Initialize Layer Property Editor and Details View
  LayerPropertyEditor = NewObject<UKLayerPropertyEditor>();
  LayerPropertyEditor->AddToRoot();
  LayerPropertyEditor->OnLayerPropertyChanged.AddSP(this, &SMaterializeEditor::OnLayerStackChanged);

  FDetailsViewArgs LayerDetailsArgs;
  LayerDetailsArgs.bUpdatesFromSelection = false;
  LayerDetailsArgs.bLockable = false;
  LayerDetailsArgs.bAllowSearch = false;
  LayerDetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
  LayerDetailsArgs.bHideSelectionTip = true;
  LayerDetailsView = PropertyEditorModule.CreateDetailView(LayerDetailsArgs);
  LayerDetailsView->SetObject(nullptr); // Will be set when layer is selected

  // 4. Initialize Asset Picker
  FContentBrowserModule &ContentBrowserModule =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          TEXT("ContentBrowser"));
  FAssetPickerConfig AssetPickerConfig;
  AssetPickerConfig.Filter.ClassPaths.Add(
      UTexture2D::StaticClass()->GetClassPathName());
  AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
  AssetPickerConfig.bAddFilterUI = true;
  AssetPickerConfig.bShowPathInColumnView = true;
  AssetPickerConfig.bSortByPathInColumnView = true;
  AssetPickerConfig.bForceShowEngineContent = true;
  AssetPickerConfig.OnAssetSelected =
      FOnAssetSelected::CreateSP(this, &SMaterializeEditor::OnAssetSelected);
  AssetPickerConfig.bShowBottomToolbar = true;
  AssetPickerConfig.bAutohideSearchBar = false;

  TSharedRef<SWidget> AssetPicker =
      ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig);

  // 5. Build UI Layout
  ChildSlot
      [SNew(SVerticalBox)

       // --- Main Toolbar ---
       + SVerticalBox::Slot().AutoHeight().Padding(
             0)[SNew(SBorder)
                    .Padding(FMargin(8, 4))
                    .BorderImage(FAppStyle::Get().GetBrush(
                        "ToolPanel.GroupBorder"))[CreateToolbar()]]

       // --- Workflow Toolbar (Layer View / Graph View Toggle) ---
       + SVerticalBox::Slot().AutoHeight().Padding(
             0)[SNew(SBorder)
                    .Padding(FMargin(8, 2))
                    .BorderImage(FAppStyle::Get().GetBrush(
                        "ToolPanel.DarkGroupBorder"))[CreateWorkflowToolbar()]]

       // --- View Mode Toolbar ---
       + SVerticalBox::Slot().AutoHeight().Padding(
             0)[SNew(SBorder)
                    .Padding(FMargin(8, 2))
                    .BorderImage(FAppStyle::Get().GetBrush(
                        "ToolPanel.DarkGroupBorder"))[CreateViewModeToolbar()]]

       // --- Main Content Splitter ---
       +
       SVerticalBox::Slot().FillHeight(1.0f)
           [SNew(SSplitter)
                .Orientation(Orient_Horizontal)
                .Style(FAppStyle::Get(), "Splitter")
                .PhysicalSplitterHandleSize(5.0f)

            // Left: Workflow Content Switcher (Layer View / Graph View)
            + SSplitter::Slot().Value(0.75f)
                  [SAssignNew(WorkflowContentSwitcher, SWidgetSwitcher)
                   .WidgetIndex_Lambda([this]() { 
                     return CurrentWorkflowMode == EMaterializeWorkflowMode::LayerView ? 0 : 1; 
                   })
                   
                   // Slot 0: Layer View Content
                   + SWidgetSwitcher::Slot()[CreateLayerViewContent()]
                   
                   // Slot 1: Graph View Content
                   + SWidgetSwitcher::Slot()[CreateGraphViewContent()]]

            // Right: Tabbed Settings (Parameters / Layers)
            + SSplitter::Slot().Value(0.25f)[CreateRightPanel()]]];
}

SMaterializeEditor::~SMaterializeEditor() {
  if (ToolModel) {
    ToolModel->RemoveFromRoot();
  }
  if (LayerPropertyEditor) {
    LayerPropertyEditor->RemoveFromRoot();
  }
  if (CurrentGraph) {
    CurrentGraph->RemoveFromRoot();
  }
}

TSharedRef<SWidget> SMaterializeEditor::CreateToolbar() {
  return SNew(SHorizontalBox)
         // Preset Selector
         +
         SHorizontalBox::Slot()
             .AutoWidth()
             .VAlign(VAlign_Center)
             .Padding(4, 0)[CreatePresetSelector()]
         
         // Separator
         + SHorizontalBox::Slot().AutoWidth().Padding(
               8, 0)[SNew(SSpacer).Size(FVector2D(2, 20))]
         
         // Load Selected (Quick Action)
         +
         SHorizontalBox::Slot().AutoWidth().Padding(
             4, 0)[SNew(SButton)
                       .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                       .ToolTipText(FText::FromString(
                           "Use currently selected texture in Content Browser"))
                       .OnClicked_Lambda([this]() {
                         OnLoadSelectedTexture();
                         return FReply::Handled();
                       })
                       .Content()[SNew(SHorizontalBox) +
                                  SHorizontalBox::Slot().AutoWidth().VAlign(
                                      VAlign_Center)
                                      [SNew(SImage)
                                           .Image(FAppStyle::Get().GetBrush(
                                               "Icons.Search"))
                                           .ColorAndOpacity(
                                               FSlateColor::UseForeground())] +
                                  SHorizontalBox::Slot()
                                      .AutoWidth()
                                      .VAlign(VAlign_Center)
                                      .Padding(4, 0, 0,
                                               0)[SNew(STextBlock)
                                                      .Text(FText::FromString(
                                                          "Load Selected"))]]]

         + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]

         // Mesh Selector removed - now handled via Details Panel
         // SourceStaticMesh property

         // Separator
         + SHorizontalBox::Slot().AutoWidth().Padding(
               8, 0)[SNew(SSpacer).Size(FVector2D(2, 20))]

         // Environment Selector
         + SHorizontalBox::Slot()
               .AutoWidth()
               .VAlign(VAlign_Center)
               .Padding(4, 0)[CreateEnvironmentSelector()];
}

TSharedRef<SWidget> SMaterializeEditor::CreateWorkflowToolbar() {
  return SNew(SHorizontalBox) +
         SHorizontalBox::Slot()
             .AutoWidth()
             .VAlign(VAlign_Center)
             .Padding(0, 0, 8, 0)
             [SNew(STextBlock).Text(FText::FromString("Workflow:"))] +
         SHorizontalBox::Slot().AutoWidth()
             [SNew(SSegmentedControl<EMaterializeWorkflowMode>)
                  .Value(EMaterializeWorkflowMode::LayerView)
                  .OnValueChanged_Lambda(
                      [this](EMaterializeWorkflowMode Mode) { SetWorkflowMode(Mode); }) +
              SSegmentedControl<EMaterializeWorkflowMode>::Slot(
                  EMaterializeWorkflowMode::LayerView)
                  .Text(FText::FromString("Layer View (Sampler)")) +
              SSegmentedControl<EMaterializeWorkflowMode>::Slot(
                  EMaterializeWorkflowMode::GraphView)
                  .Text(FText::FromString("Graph View (Designer)"))];
}

TSharedRef<SWidget> SMaterializeEditor::CreateViewModeToolbar() {
  return SNew(SHorizontalBox) +
         SHorizontalBox::Slot()
             .AutoWidth()
             .VAlign(VAlign_Center)
             .Padding(0, 0, 8,
                      0)[SNew(STextBlock).Text(FText::FromString("View:"))] +
         SHorizontalBox::Slot().AutoWidth()
             [SNew(SSegmentedControl<EMaterializeViewMode>)
                  .Value(EMaterializeViewMode::Material)
                  .OnValueChanged_Lambda(
                      [this](EMaterializeViewMode Mode) { SetViewMode(Mode); }) +
              SSegmentedControl<EMaterializeViewMode>::Slot(
                  EMaterializeViewMode::Material)
                  .Text(FText::FromString("Material")) +
              SSegmentedControl<EMaterializeViewMode>::Slot(
                  EMaterializeViewMode::Normal)
                  .Text(FText::FromString("N")) +
              SSegmentedControl<EMaterializeViewMode>::Slot(
                  EMaterializeViewMode::Roughness)
                  .Text(FText::FromString("R")) +
              SSegmentedControl<EMaterializeViewMode>::Slot(
                  EMaterializeViewMode::Metallic)
                  .Text(FText::FromString("M")) +
              SSegmentedControl<EMaterializeViewMode>::Slot(EMaterializeViewMode::AO)
                  .Text(FText::FromString("AO")) +
              SSegmentedControl<EMaterializeViewMode>::Slot(
                  EMaterializeViewMode::Height)
                  .Text(FText::FromString("H")) +
              SSegmentedControl<EMaterializeViewMode>::Slot(
                  EMaterializeViewMode::BaseColor)
                  .Text(FText::FromString("Base"))];
}

TSharedRef<SWidget> SMaterializeEditor::CreateChannelStrip() {
  auto MakeEntry = [&](const FString &Label, EMaterializeViewMode Mode,
                       TSharedPtr<SImage> &ImageSlot) -> TSharedRef<SWidget> {
    return SNew(SButton)
        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
        .ToolTipText(
            FText::FromString(FString::Printf(TEXT("View %s channel"), *Label)))
        .OnClicked_Lambda([this, Mode]() {
          SetViewMode(Mode);
          return FReply::Handled();
        })[SNew(SVerticalBox) +
           SVerticalBox::Slot().AutoHeight().HAlign(
               HAlign_Center)[SNew(SBox).WidthOverride(64).HeightOverride(
               64)[SNew(SBorder)
                       .BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
                       .BorderBackgroundColor(
                           FLinearColor(0.1f, 0.1f, 0.1f, 1.0f))
                       .Padding(2)[SAssignNew(ImageSlot, SImage)
                                       .Image(FAppStyle::Get().GetBrush(
                                           "WhiteBrush"))]]] +
           SVerticalBox::Slot()
               .AutoHeight()
               .HAlign(HAlign_Center)
               .Padding(0, 4, 0, 0)[SNew(STextBlock)
                                        .Text(FText::FromString(Label))
                                        .Font(FAppStyle::Get().GetFontStyle(
                                            "SmallFont"))]];
  };

  return SNew(SHorizontalBox) +
         SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center)
             [SNew(SHorizontalBox) +
              SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[MakeEntry(
                  TEXT("Normal"), EMaterializeViewMode::Normal, NormalThumbnail)] +
              SHorizontalBox::Slot().AutoWidth().Padding(
                  4, 0)[MakeEntry(TEXT("Rough"), EMaterializeViewMode::Roughness,
                                  RoughnessThumbnail)] +
              SHorizontalBox::Slot().AutoWidth().Padding(
                  4, 0)[MakeEntry(TEXT("Metal"), EMaterializeViewMode::Metallic,
                                  MetallicThumbnail)] +
              SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[MakeEntry(
                  TEXT("AO"), EMaterializeViewMode::AO, AOThumbnail)] +
              SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[MakeEntry(
                  TEXT("Height"), EMaterializeViewMode::Height, HeightThumbnail)] +
              SHorizontalBox::Slot().AutoWidth().Padding(
                  4, 0)[MakeEntry(TEXT("Emissive"), EMaterializeViewMode::Emissive,
                                  EmissiveThumbnail)]];
}

TSharedRef<SWidget> SMaterializeEditor::CreateEnvironmentSelector() {
  return SNew(SHorizontalBox) +
         SHorizontalBox::Slot()
             .AutoWidth()
             .VAlign(VAlign_Center)
             .Padding(0, 0, 8,
                      0)[SNew(STextBlock).Text(FText::FromString("Env:"))] +
         SHorizontalBox::Slot().AutoWidth()
             [SNew(SSegmentedControl<FString>)
                 .Value_Lambda([this]() { return CurrentEnvironmentName; })
                 .OnValueChanged_Lambda([this](const FString &Value) {
                   SetEnvironment(Value);
                 }) +
              SSegmentedControl<FString>::Slot(FString("Studio"))
                  .Text(FText::FromString("Studio")) +
              SSegmentedControl<FString>::Slot(FString("Cloudy Sky"))
                  .Text(FText::FromString("Sky")) +
              SSegmentedControl<FString>::Slot(FString("Daylight"))
                  .Text(FText::FromString("Day"))];
}

void SMaterializeEditor::SetEnvironment(const FString &EnvName) {
  const TCHAR *EnvPath = nullptr;
  FString ResolvedName = GDefaultEnvironmentName;

  for (int32 i = 0; i < UE_ARRAY_COUNT(GEnvironmentNames); ++i) {
    if (EnvName == GEnvironmentNames[i]) {
      EnvPath = GEnvironmentMaps[i];
      ResolvedName = GEnvironmentNames[i];
      break;
    }
  }

  if (!EnvPath) {
    for (int32 i = 0; i < UE_ARRAY_COUNT(GEnvironmentNames); ++i) {
      if (ResolvedName == GEnvironmentNames[i]) {
        EnvPath = GEnvironmentMaps[i];
        break;
      }
    }

    if (!EnvPath) {
      EnvPath = GEnvironmentMaps[0];
      ResolvedName = GEnvironmentNames[0];
    }
  }

  CurrentEnvironmentName = ResolvedName;

  UTextureCube *EnvMap = LoadObject<UTextureCube>(nullptr, EnvPath);
  if (EnvMap && PreviewScene.IsValid()) {
    FPreviewSceneProfile Profile;
    Profile.EnvironmentCubeMap = EnvMap;
    Profile.bShowEnvironment = true;
    Profile.bShowFloor = false;
    Profile.EnvironmentIntensity = 1.0f;
    Profile.SkyLightIntensity = 1.0f;
    PreviewScene->UpdateScene(Profile);
  }
}

void SMaterializeEditor::SetViewMode(EMaterializeViewMode NewMode) {
  CurrentViewMode = NewMode;
  ApplyChannelMaterial(NewMode);
}

void SMaterializeEditor::ApplyChannelMaterial(EMaterializeViewMode Mode) {
  if (!CachedResult.IsValid())
    return;

  if (Mode == EMaterializeViewMode::Material) {
    SetPreviewMaterialFromResult(CachedResult);
    return;
  }

  UTexture2D *ChannelTex = nullptr;
  bool bIsNormal = false;

  switch (Mode) {
  case EMaterializeViewMode::BaseColor:
    ChannelTex = ToolModel->SourceTexture
                     ? ToolModel->SourceTexture.Get()
                     : FMaterializeEditorContext::GetCurrentTexture();
    break;
  case EMaterializeViewMode::Normal:
    ChannelTex = CachedResult.Normal;
    bIsNormal = true;
    break;
  case EMaterializeViewMode::Roughness:
    ChannelTex = CachedResult.Roughness;
    break;
  case EMaterializeViewMode::Metallic:
    ChannelTex = CachedResult.Metallic;
    break;
  case EMaterializeViewMode::AO:
    ChannelTex = CachedResult.AO;
    break;
  case EMaterializeViewMode::Height:
    ChannelTex = CachedResult.Height;
    break;
  case EMaterializeViewMode::Emissive:
    ChannelTex = CachedResult.Emissive;
    break;
  default:
    break;
  }

  if (ChannelTex) {
    UMaterialInstanceDynamic *ChannelMat =
        CreateChannelPreviewMaterial(ChannelTex, bIsNormal);
    if (ChannelMat && ViewportClient.IsValid()) {
      ViewportClient->SetPreviewMaterial(ChannelMat);
    }
  }
}

UMaterialInstanceDynamic *
SMaterializeEditor::CreateChannelPreviewMaterial(UTexture2D *ChannelTexture,
                                             bool bIsNormal) {
  if (!ChannelTexture)
    return nullptr;

  // Create a simple transient material for previewing
  FName MatName =
      MakeUniqueObjectName(GetTransientPackage(), UMaterial::StaticClass(),
                           TEXT("M_ChannelPreview"));
  UMaterial *PreviewMat =
      NewObject<UMaterial>(GetTransientPackage(), MatName, RF_Transient);

  // Create texture sampler parameter
  UMaterialExpressionTextureSampleParameter2D *Sampler =
      NewObject<UMaterialExpressionTextureSampleParameter2D>(PreviewMat);
  Sampler->ParameterName = TEXT("InputTexture");

  // Set sampler type and compatible default texture
  // Must use plugin's default textures that match the sampler type
  if (bIsNormal) {
    Sampler->SamplerType = SAMPLERTYPE_Normal;
    UTexture *DefaultNormal = LoadObject<UTexture>(
        nullptr,
        TEXT("/Materialize/Textures/Defaults/T_Default_Normal.T_Default_Normal"));
    if (DefaultNormal) {
      Sampler->Texture = DefaultNormal;
    }
  } else {
    Sampler->SamplerType = SAMPLERTYPE_Masks;
    UTexture *DefaultLinear = LoadObject<UTexture>(
        nullptr,
        TEXT("/Materialize/Textures/Defaults/T_Default_ORM.T_Default_ORM"));
    if (DefaultLinear) {
      Sampler->Texture = DefaultLinear;
    }
  }

  PreviewMat->GetExpressionCollection().AddExpression(Sampler);

  // Create connection
#if WITH_EDITOR
  PreviewMat->GetEditorOnlyData()->EmissiveColor.Expression = Sampler;
  PreviewMat->GetEditorOnlyData()->BaseColor.Expression = Sampler;
#endif

  PreviewMat->MaterialDomain = EMaterialDomain::MD_Surface;
  PreviewMat->BlendMode = BLEND_Opaque;
  PreviewMat->SetShadingModel(MSM_Unlit);

  // Compile the material
  PreviewMat->PostEditChange();

  UMaterialInstanceDynamic *MID =
      UMaterialInstanceDynamic::Create(PreviewMat, nullptr);
  MID->SetTextureParameterValue(TEXT("InputTexture"), ChannelTexture);

  return MID;
}

void SMaterializeEditor::OnModelChanged() {
  // Check if mesh changed
  if (ToolModel->SourceStaticMesh != LastPreviewMesh) {
    UpdateMeshPreview();
  }

  MaybeLiveUpdate();
}

void SMaterializeEditor::MaybeLiveUpdate() {
  // Cancel any pending debounce by marking a new request.
  // The active timer checks this flag and resets its countdown each call.
  bPreviewDebounceRequested = true;

  // Only register a new timer if one isn't already running.
  if (!PreviewDebounceHandle.IsValid()) {
    TWeakPtr<SMaterializeEditor> WeakSelf = SharedThis(this);
    TSharedRef<FActiveTimerHandle> Handle = RegisterActiveTimer(
        0.15f, // 0.15s debounce interval
        FWidgetActiveTimerDelegate::CreateLambda(
            [WeakSelf](double InCurrentTime, float InDeltaTime) -> EActiveTimerReturnType {
              if (TSharedPtr<SMaterializeEditor> Pinned = WeakSelf.Pin()) {
                if (Pinned->bPreviewDebounceRequested) {
                  // Another change came in — reset and wait another tick
                  Pinned->bPreviewDebounceRequested = false;
                  return EActiveTimerReturnType::Continue;
                }
                // No new changes — fire the preview and stop the timer
                Pinned->PreviewDebounceHandle.Reset();
                Pinned->OnGeneratePreview();
                return EActiveTimerReturnType::Stop;
              }
              return EActiveTimerReturnType::Stop;
            }));
    PreviewDebounceHandle = Handle;
  }
}

void SMaterializeEditor::OnAssetSelected(const FAssetData &AssetData) {
  if (UTexture2D *Tex = Cast<UTexture2D>(AssetData.GetAsset())) {
    ToolModel->SourceTexture = Tex;
    FMaterializeEditorContext::SetCurrentTexture(Tex);
    MaybeLiveUpdate();
  }
}

void SMaterializeEditor::OnPickTexture() {
  FContentBrowserModule &CBModule =
      FModuleManager::LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser");
  FOpenAssetDialogConfig Config;
  Config.DialogTitleOverride = FText::FromString("Choose Texture2D");
  Config.bAllowMultipleSelection = false;
  Config.DefaultPath = TEXT("/Game");
  Config.AssetClassNames.Add(UTexture2D::StaticClass()->GetClassPathName());

  TArray<FAssetData> Assets = CBModule.Get().CreateModalOpenAssetDialog(Config);
  if (Assets.Num() > 0) {
    ToolModel->SourceTexture = Cast<UTexture2D>(Assets[0].GetAsset());
    FMaterializeEditorContext::SetCurrentTexture(ToolModel->SourceTexture.Get());
    MaybeLiveUpdate();
  }
}

void SMaterializeEditor::OnLoadSelectedTexture() {
  TArray<FAssetData> Selected;
  FContentBrowserModule &CBModule =
      FModuleManager::LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser");
  CBModule.Get().GetSelectedAssets(Selected);
  for (const FAssetData &AD : Selected) {
    if (UTexture2D *Tex = Cast<UTexture2D>(AD.GetAsset())) {
      ToolModel->SourceTexture = Tex;
      FMaterializeEditorContext::SetCurrentTexture(ToolModel->SourceTexture.Get());
      MaybeLiveUpdate();
      break;
    }
  }
}

void SMaterializeEditor::OnContextTextureChanged(UTexture2D *NewTexture) {
  if (NewTexture) {
    ToolModel->SourceTexture = NewTexture;
    // Do NOT call SetCurrentTexture here to avoid recursion
    MaybeLiveUpdate();
  }
}

void SMaterializeEditor::UpdateMeshPreview() {
  if (!ViewportClient)
    return;

  UStaticMesh *Mesh = ToolModel->SourceStaticMesh;
  // Fallback if null
  if (!Mesh) {
    Mesh = LoadObject<UStaticMesh>(nullptr,
                                   TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  }

  LastPreviewMesh = Mesh;
  ViewportClient->SetPreviewMesh(Mesh, true);

  // Re-apply current material to the new mesh
  ApplyChannelMaterial(CurrentViewMode);
}

void SMaterializeEditor::OnGeneratePreview() {
  UTexture2D *Tex = ToolModel->SourceTexture
                        ? ToolModel->SourceTexture.Get()
                        : FMaterializeEditorContext::GetCurrentTexture();
  if (!Tex) {
    if (StatusText.IsValid())
      StatusText->SetText(FText::FromString("Select a Texture2D first"));
    return;
  }

  // Safety check for partially loaded assets
  if (Tex->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad)) {
    if (StatusText.IsValid())
      StatusText->SetText(
          FText::FromString("Asset not fully loaded - Aborting"));
    UE_LOG(LogTemp, Warning, TEXT("Materialize: Texture %s is not fully loaded"),
           *Tex->GetName());
    return;
  }

  // CRITICAL: ensure texture is fully GPU-resident before compute dispatch
  // without this, streaming textures can trigger CurrentGPUVirtualAddress != 0
  // assertion
  if (Tex->GetResource()) {
    Tex->WaitForStreaming();
    FlushRenderingCommands();

    FRHITexture* TestRHI = Tex->GetResource()->GetTexture2DRHI();
    if (!TestRHI || !TestRHI->GetNativeResource()) {
      if (StatusText.IsValid())
        StatusText->SetText(
            FText::FromString("Texture GPU resource not ready"));
      return;
    }
  } else {
    if (StatusText.IsValid())
      StatusText->SetText(FText::FromString("Texture resource not available"));
    return;
  }

  if (UMaterializeComputeEngine::GeneratePBRMapsGPU(Tex, ToolModel->Params,
                                                CachedResult)) {
    // Apply based on current view mode
    ApplyChannelMaterial(CurrentViewMode);
    UpdateChannelThumbnails();
    if (StatusText.IsValid())
      StatusText->SetText(FText::Format(
          FText::FromString("GPU Preview: {0} ms"),
          FText::AsNumber(FMath::RoundToInt(CachedResult.GenerationTimeMs))));
  } else {
    if (StatusText.IsValid())
      StatusText->SetText(FText::FromString("GPU Generation failed"));
  }
}

void SMaterializeEditor::OnGenerateAndSave() {
  UTexture2D *Tex = ToolModel->SourceTexture
                        ? ToolModel->SourceTexture.Get()
                        : FMaterializeEditorContext::GetCurrentTexture();
  if (!Tex) {
    if (StatusText.IsValid())
      StatusText->SetText(FText::FromString("Select a Texture2D first"));
    return;
  }

  // Safety check for partially loaded assets
  if (Tex->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad)) {
    if (StatusText.IsValid())
      StatusText->SetText(
          FText::FromString("Asset not fully loaded - Aborting"));
    UE_LOG(LogTemp, Warning, TEXT("Materialize: Texture %s is not fully loaded"),
           *Tex->GetName());
    return;
  }

  // CRITICAL: ensure texture is fully GPU-resident before compute dispatch
  if (Tex->GetResource()) {
    Tex->WaitForStreaming();
    FlushRenderingCommands();

    FRHITexture* TestRHI = Tex->GetResource()->GetTexture2DRHI();
    if (!TestRHI || !TestRHI->GetNativeResource()) {
      if (StatusText.IsValid())
        StatusText->SetText(
            FText::FromString("Texture GPU resource not ready"));
      return;
    }
  } else {
    if (StatusText.IsValid())
      StatusText->SetText(FText::FromString("Texture resource not available"));
    return;
  }

  FMaterializeResult Result;
  if (UMaterializeComputeEngine::GeneratePBRMapsGPU(Tex, ToolModel->Params,
                                                Result)) {
    // Get computed output path and base name from tool model
    FString OutputPath = ToolModel->GetComputedOutputPath();
    FString BaseName = ToolModel->GetComputedBaseName();

    if (UMaterializeEngine::GenerateAndSavePBRMaps(Tex, ToolModel->Params,
                                               OutputPath, BaseName, Result)) {
      CachedResult = Result;
      SetPreviewMaterialFromResult(Result);

      // Show Toast with output path info
      FNotificationInfo Info(FText::FromString("Materialize Assets Generated"));
      Info.ExpireDuration = 4.0f;
      Info.SubText = FText::FromString(FString::Printf(
          TEXT("Saved to: %s\nMaterial: MI_%s"), *OutputPath, *BaseName));
      Info.Image = FAppStyle::Get().GetBrush("LevelEditor.Recompile");
      FSlateNotificationManager::Get().AddNotification(Info);

      if (StatusText.IsValid())
        StatusText->SetText(FText::Format(FText::FromString("Saved to {0}"),
                                          FText::FromString(OutputPath)));
    }
  } else {
    if (StatusText.IsValid())
      StatusText->SetText(FText::FromString("Generation failed"));
  }
}

void SMaterializeEditor::UpdateChannelThumbnails() {
  const FVector2D ThumbSize(64.0f, 64.0f);

  // Helper to update thumbnail with an optional grayscale mode
  // For single-channel R32F textures, we use a material-based approach
  // to replicate R to RGB for proper grayscale display
  auto UpdateThumb = [&](UTexture2D *Tex, TSharedPtr<FSlateBrush> &Brush,
                         TSharedPtr<SImage> &Image, bool bIsGrayscale = false) {
    if (!Image.IsValid())
      return;

    if (!Tex) {
      Image->SetImage(FAppStyle::Get().GetBrush("WhiteBrush"));
      return;
    }

    if (!Brush.IsValid()) {
      Brush = MakeShareable(new FSlateBrush());
    }

    if (bIsGrayscale) {
      // For single-channel textures (R32F), we need a material to properly
      // display as grayscale. Create a simple unlit material that replicates R.
      UMaterialInstanceDynamic *GrayscaleMID = nullptr;

      // Try to get or create a grayscale preview material
      // NOTE: Must use IsValid() instead of bare nullptr check — a static pointer
      // can dangle after GC collects the UObject (pointer is non-null but dead).
      static UMaterial *GrayscaleMaterial = nullptr;
      if (!IsValid(GrayscaleMaterial)) {
        GrayscaleMaterial = nullptr; // Reset dangling pointer if GC'd

        // Create a transient material that converts R to RGB
        GrayscaleMaterial = NewObject<UMaterial>(
            GetTransientPackage(),
            MakeUniqueObjectName(GetTransientPackage(),
                                 UMaterial::StaticClass(),
                                 TEXT("M_GrayscalePreview")),
            RF_Transient);

        // Prevent GC from collecting this — we hold a raw static pointer
        GrayscaleMaterial->AddToRoot();

        // Create texture sampler
        UMaterialExpressionTextureSampleParameter2D *Sampler =
            NewObject<UMaterialExpressionTextureSampleParameter2D>(
                GrayscaleMaterial);
        Sampler->ParameterName = TEXT("InputTexture");
        // Use Masks sampler type to match our plugin's default ORM texture
        Sampler->SamplerType = SAMPLERTYPE_Masks;
        // Set a compatible default texture to avoid engine default texture
        // conflicts
        UTexture *DefaultTex = LoadObject<UTexture>(
            nullptr,
            TEXT("/Materialize/Textures/Defaults/T_Default_ORM.T_Default_ORM"));
        if (DefaultTex) {
          Sampler->Texture = DefaultTex;
        }
        GrayscaleMaterial->GetExpressionCollection().AddExpression(Sampler);

        // Create component mask to get R channel
        UMaterialExpressionComponentMask *RMask =
            NewObject<UMaterialExpressionComponentMask>(GrayscaleMaterial);
        RMask->R = true;
        RMask->G = false;
        RMask->B = false;
        RMask->A = false;
        RMask->Input.Connect(0, Sampler);
        GrayscaleMaterial->GetExpressionCollection().AddExpression(RMask);

        // Create append to replicate R to RGB
        UMaterialExpressionAppendVector *Append1 =
            NewObject<UMaterialExpressionAppendVector>(GrayscaleMaterial);
        Append1->A.Connect(0, RMask);
        Append1->B.Connect(0, RMask);
        GrayscaleMaterial->GetExpressionCollection().AddExpression(Append1);

        UMaterialExpressionAppendVector *Append2 =
            NewObject<UMaterialExpressionAppendVector>(GrayscaleMaterial);
        Append2->A.Connect(0, Append1);
        Append2->B.Connect(0, RMask);
        GrayscaleMaterial->GetExpressionCollection().AddExpression(Append2);

#if WITH_EDITOR
        GrayscaleMaterial->GetEditorOnlyData()->EmissiveColor.Expression =
            Append2;
#endif

        GrayscaleMaterial->MaterialDomain = EMaterialDomain::MD_UI;
        GrayscaleMaterial->BlendMode = BLEND_Opaque;
        GrayscaleMaterial->SetShadingModel(MSM_Unlit);
        GrayscaleMaterial->PostEditChange();
      }

      GrayscaleMID =
          UMaterialInstanceDynamic::Create(GrayscaleMaterial, nullptr);
      GrayscaleMID->SetTextureParameterValue(TEXT("InputTexture"), Tex);

      Brush->SetResourceObject(GrayscaleMID);
    } else {
      // For RGB textures, display directly
      Brush->SetResourceObject(Tex);
    }

    Brush->ImageSize = ThumbSize;
    Brush->DrawAs = ESlateBrushDrawType::Image;
    Image->SetImage(Brush.Get());
  };

  // Normal is RGB so it displays correctly
  UpdateThumb(CachedResult.Normal, NormalBrush, NormalThumbnail, false);

  // Single-channel textures need grayscale conversion
  UpdateThumb(CachedResult.Roughness, RoughnessBrush, RoughnessThumbnail, true);
  UpdateThumb(CachedResult.Metallic, MetallicBrush, MetallicThumbnail, true);
  UpdateThumb(CachedResult.AO, AOBrush, AOThumbnail, true);
  UpdateThumb(CachedResult.Height, HeightBrush, HeightThumbnail, true);

  // Emissive is RGB
  UpdateThumb(CachedResult.Emissive, EmissiveBrush, EmissiveThumbnail, false);
}

void SMaterializeEditor::SetPreviewMaterialFromResult(FMaterializeResult &Result) {
  // Use the new material loading system with fallback chain
  FMaterializeMaterialLoadResult LoadResult = FMaterializeMaterialLoader::LoadMasterMaterial(TEXT("Standard"));
  
  UMaterial *Parent = nullptr;
  
  if (LoadResult.IsValid()) {
    Parent = LoadResult.Material;
    UE_LOG(LogMaterialize, Log, TEXT("Materialize: Loaded master material successfully"));
  } else {
    // Fallback to generating transient material if master doesn't exist
    UE_LOG(LogMaterialize, Warning,
           TEXT("Materialize: Master material not found, generating transient material"));
    Parent = FMaterializeTransientGenerator::Generate(TEXT("Standard"));
  }

  // Verify parent material is valid
  MATERIALIZE_CHECK_PTR_VOID(Parent);

  // Create Material Instance Dynamic with null check
  UMaterialInstanceDynamic *MID = UMaterialInstanceDynamic::Create(Parent, nullptr);
  
  if (!MID) {
    UE_LOG(LogMaterialize, Error, TEXT("Materialize: Failed to create MID - parent material may be invalid"));
    return;
  }

  // Set BaseColor: prefer layer-evaluated result, fall back to source texture
  if (Result.LayerBaseColor) {
    MID->SetTextureParameterValue(TEXT("BaseColor"), Result.LayerBaseColor);
  } else if (ToolModel->SourceTexture) {
    MATERIALIZE_CHECK_PTR_VOID(ToolModel->SourceTexture);
    MID->SetTextureParameterValue(TEXT("BaseColor"), ToolModel->SourceTexture);
  }
  
  if (Result.Normal) {
    MATERIALIZE_CHECK_PTR_VOID(Result.Normal);
    MID->SetTextureParameterValue(TEXT("Normal"), Result.Normal);
  }

  // Use ORM packed texture if available, otherwise fall back to individual textures
  if (Result.ORM) {
    MATERIALIZE_CHECK_PTR_VOID(Result.ORM);
    MID->SetTextureParameterValue(TEXT("ORM"), Result.ORM);
  } else {
    // Fallback for older generation code that doesn't produce ORM
    if (Result.Roughness) {
      MATERIALIZE_CHECK_PTR_VOID(Result.Roughness);
      MID->SetTextureParameterValue(TEXT("Roughness"), Result.Roughness);
    }
    if (Result.Metallic) {
      MATERIALIZE_CHECK_PTR_VOID(Result.Metallic);
      MID->SetTextureParameterValue(TEXT("Metallic"), Result.Metallic);
    }
    if (Result.AO) {
      MATERIALIZE_CHECK_PTR_VOID(Result.AO);
      MID->SetTextureParameterValue(TEXT("AO"), Result.AO);
    }
    if (Result.Height) {
      MATERIALIZE_CHECK_PTR_VOID(Result.Height);
      MID->SetTextureParameterValue(TEXT("Height"), Result.Height);
    }
  }

  // Set emissive if available
  if (Result.Emissive) {
    MATERIALIZE_CHECK_PTR_VOID(Result.Emissive);
    MID->SetTextureParameterValue(TEXT("Emissive"), Result.Emissive);
  }

  // Set scalar parameters from ToolModel params
  // These match the parameter names in M_Materialize_Master
  MID->SetScalarParameterValue(TEXT("Tiling"), 1.0f); // Default tiling
  MID->SetScalarParameterValue(TEXT("AO_Power"), ToolModel->Params.AOIntensity);
  MID->SetScalarParameterValue(TEXT("Roughness_Mult"),
                               ToolModel->Params.RoughnessContrast);
  MID->SetScalarParameterValue(TEXT("Roughness_Offset"),
                               ToolModel->Params.RoughnessBrightness / 255.0f);
  MID->SetScalarParameterValue(TEXT("Metallic_Mult"),
                               ToolModel->Params.MetallicContrast);
  MID->SetScalarParameterValue(TEXT("Emissive_Power"),
                               ToolModel->Params.EmissiveThreshold);

  // Set tint color
  MID->SetVectorParameterValue(TEXT("Tint"), FLinearColor::White);

  if (ViewportClient.IsValid()) {
    UE_LOG(LogMaterialize, Log, TEXT("Materialize: Setting preview material on viewport"));
    ViewportClient->SetPreviewMaterial(MID);

    // Force immediate render update
    FlushRenderingCommands();
  } else {
    UE_LOG(LogMaterialize, Warning, TEXT("Materialize: ViewportClient is not valid!"));
  }
}

void SMaterializeEditor::SetPreviewMaterialFromGraphResult(const FMaterializeGraphExecutionResult &GraphResult) {
  // Load master material (same chain as the layer/compute path)
  FMaterializeMaterialLoadResult LoadResult = FMaterializeMaterialLoader::LoadMasterMaterial(TEXT("Standard"));

  UMaterial *Parent = nullptr;
  if (LoadResult.IsValid()) {
    Parent = LoadResult.Material;
  } else {
    Parent = FMaterializeTransientGenerator::Generate(TEXT("Standard"));
  }

  if (!Parent) {
    UE_LOG(LogMaterialize, Error, TEXT("Materialize: Cannot create preview material - no parent material available"));
    return;
  }

  UMaterialInstanceDynamic *MID = UMaterialInstanceDynamic::Create(Parent, nullptr);
  if (!MID) {
    UE_LOG(LogMaterialize, Error, TEXT("Materialize: Failed to create MID from graph result"));
    return;
  }

  // Graph produces its own BaseColor — use it instead of ToolModel->SourceTexture
  if (GraphResult.BaseColor) {
    MID->SetTextureParameterValue(TEXT("BaseColor"), GraphResult.BaseColor);
  } else if (ToolModel && ToolModel->SourceTexture) {
    // Fallback to the user's source texture if graph didn't produce BaseColor
    MID->SetTextureParameterValue(TEXT("BaseColor"), ToolModel->SourceTexture);
  }

  if (GraphResult.Normal) {
    MID->SetTextureParameterValue(TEXT("Normal"), GraphResult.Normal);
  }
  if (GraphResult.Roughness) {
    MID->SetTextureParameterValue(TEXT("Roughness"), GraphResult.Roughness);
  }
  if (GraphResult.Metallic) {
    MID->SetTextureParameterValue(TEXT("Metallic"), GraphResult.Metallic);
  }
  if (GraphResult.AO) {
    MID->SetTextureParameterValue(TEXT("AO"), GraphResult.AO);
  }
  if (GraphResult.Height) {
    MID->SetTextureParameterValue(TEXT("Height"), GraphResult.Height);
  }
  if (GraphResult.Emissive) {
    MID->SetTextureParameterValue(TEXT("Emissive"), GraphResult.Emissive);
  }

  // Sensible defaults for scalar params
  MID->SetScalarParameterValue(TEXT("Tiling"), 1.0f);
  MID->SetScalarParameterValue(TEXT("AO_Power"), 1.0f);
  MID->SetScalarParameterValue(TEXT("Roughness_Mult"), 1.0f);
  MID->SetScalarParameterValue(TEXT("Roughness_Offset"), 0.0f);
  MID->SetScalarParameterValue(TEXT("Metallic_Mult"), 1.0f);
  MID->SetScalarParameterValue(TEXT("Emissive_Power"), 1.0f);
  MID->SetVectorParameterValue(TEXT("Tint"), FLinearColor::White);

  if (ViewportClient.IsValid()) {
    ViewportClient->SetPreviewMaterial(MID);
    FlushRenderingCommands();
  }
}

// =============================================================================
// RIGHT PANEL - Tabbed Interface (Parameters / Layers)
// =============================================================================

TSharedRef<SWidget> SMaterializeEditor::CreateRightPanel() {
  return SNew(SBorder)
      .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
      .Padding(
          0)[SNew(SVerticalBox)

             // Tab Headers
             + SVerticalBox::Slot().AutoHeight()
                   [SNew(SHorizontalBox)

                    // Parameters Tab Button
                    + SHorizontalBox::Slot().FillWidth(
                          1.0f)[SNew(SButton)
                                    .HAlign(HAlign_Center)
                                    .ButtonStyle(FAppStyle::Get(), "FlatButton")
                                    .ContentPadding(FMargin(12, 6))
                                    .Text(FText::FromString("Parameters"))
                                    .OnClicked_Lambda([this]() {
                                      CurrentTabIndex = 0;
                                      if (RightPanelSwitcher.IsValid()) {
                                        RightPanelSwitcher
                                            ->SetActiveWidgetIndex(0);
                                      }
                                      return FReply::Handled();
                                    })]

                    // Layers Tab Button
                    + SHorizontalBox::Slot().FillWidth(
                          1.0f)[SNew(SButton)
                                    .HAlign(HAlign_Center)
                                    .ButtonStyle(FAppStyle::Get(), "FlatButton")
                                    .ContentPadding(FMargin(12, 6))
                                    .Text(FText::FromString("Layers"))
                                    .OnClicked_Lambda([this]() {
                                      CurrentTabIndex = 1;
                                      if (RightPanelSwitcher.IsValid()) {
                                        RightPanelSwitcher
                                            ->SetActiveWidgetIndex(1);
                                      }
                                      return FReply::Handled();
                                    })]]

             // Separator line
             +
             SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(
                 2)[SNew(SImage).Image(FAppStyle::Get().GetBrush("Separator"))]]

             // Tab Content Area
             + SVerticalBox::Slot().FillHeight(
                   1.0f)[SAssignNew(RightPanelSwitcher, SWidgetSwitcher)
                             .WidgetIndex(0)

                         // Tab 0: Parameters
                         + SWidgetSwitcher::Slot()[CreateParametersTab()]

                         // Tab 1: Layers
                         + SWidgetSwitcher::Slot()[CreateLayersTab()]]];
}

TSharedRef<SWidget> SMaterializeEditor::CreateParametersTab() {
  return SNew(SVerticalBox)

         // Details View
         + SVerticalBox::Slot().FillHeight(1.0f).Padding(
               5)[DetailsView.ToSharedRef()]

         // Output Path Preview
         +
         SVerticalBox::Slot().AutoHeight().Padding(10, 5)
             [SNew(SVerticalBox) +
              SVerticalBox::Slot().AutoHeight()
                  [SNew(STextBlock)
                       .Text(FText::FromString("Output Path:"))
                       .Font(FAppStyle::Get().GetFontStyle("SmallFont"))
                       .ColorAndOpacity(FSlateColor::UseSubduedForeground())] +
              SVerticalBox::Slot().AutoHeight().Padding(
                  0,
                  2)[SNew(SBorder)
                         .BorderImage(FAppStyle::Get().GetBrush(
                             "ToolPanel.DarkGroupBorder"))
                         .Padding(FMargin(
                             6, 4))[SAssignNew(OutputPathText, STextBlock)
                                        .Text_Lambda([this]() {
                                          if (ToolModel) {
                                            return FText::FromString(
                                                ToolModel
                                                    ->GetComputedOutputPath());
                                          }
                                          return FText::FromString("/Game");
                                        })
                                        .Font(FAppStyle::Get().GetFontStyle(
                                            "SmallFont"))
                                        .ColorAndOpacity(FLinearColor(
                                            0.6f, 0.8f, 1.0f, 1.0f))]]]

         // Generate Button
         + SVerticalBox::Slot().AutoHeight().Padding(
               10)[SNew(SButton)
                       .HAlign(HAlign_Center)
                       .VAlign(VAlign_Center)
                       .ContentPadding(FMargin(10, 8))
                       .ButtonStyle(FAppStyle::Get(), "PrimaryButton")
                       .Text(FText::FromString("Generate & Save Assets"))
                       .OnClicked_Lambda([this]() {
                         OnGenerateAndSave();
                         return FReply::Handled();
                       })];
}

TSharedRef<SWidget> SMaterializeEditor::CreateLayersTab() {
  return SNew(SVerticalBox)

      // Add Layer Toolbar - Row 1
      + SVerticalBox::Slot().AutoHeight().Padding(4)
            [SNew(SHorizontalBox)

             + SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)
                   [SNew(SButton)
                        .HAlign(HAlign_Center)
                        .ContentPadding(FMargin(4, 2))
                        .Text(FText::FromString("+ Image"))
                        .ToolTipText(FText::FromString("Add texture layer"))
                        .OnClicked_Lambda([this]() {
                          AddLayer(EKLayerType::Image);
                          return FReply::Handled();
                        })]

             + SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)
                   [SNew(SButton)
                        .HAlign(HAlign_Center)
                        .ContentPadding(FMargin(4, 2))
                        .Text(FText::FromString("+ Fill"))
                        .ToolTipText(FText::FromString("Add solid color layer"))
                        .OnClicked_Lambda([this]() {
                          AddLayer(EKLayerType::Fill);
                          return FReply::Handled();
                        })]

             + SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)
                   [SNew(SButton)
                        .HAlign(HAlign_Center)
                        .ContentPadding(FMargin(4, 2))
                        .Text(FText::FromString("+ Proc"))
                        .ToolTipText(FText::FromString("Add procedural noise layer"))
                        .OnClicked_Lambda([this]() {
                          AddLayer(EKLayerType::Procedural);
                          return FReply::Handled();
                        })]]

      // Add Layer Toolbar - Row 2
      + SVerticalBox::Slot().AutoHeight().Padding(4, 0)
            [SNew(SHorizontalBox)

             + SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)
                   [SNew(SButton)
                        .HAlign(HAlign_Center)
                        .ContentPadding(FMargin(4, 2))
                        .Text(FText::FromString("+ Filter"))
                        .ToolTipText(FText::FromString("Add filter layer"))
                        .OnClicked_Lambda([this]() {
                          AddLayer(EKLayerType::Filter);
                          return FReply::Handled();
                        })]

             + SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)
                   [SNew(SButton)
                        .HAlign(HAlign_Center)
                        .ContentPadding(FMargin(4, 2))
                        .Text(FText::FromString("+ Adjust"))
                        .ToolTipText(FText::FromString("Add adjustment layer"))
                        .OnClicked_Lambda([this]() {
                          AddLayer(EKLayerType::Adjustment);
                          return FReply::Handled();
                        })]

             + SHorizontalBox::Slot().FillWidth(1.0f).Padding(1)
                   [SNew(SButton)
                        .HAlign(HAlign_Center)
                        .ContentPadding(FMargin(4, 2))
                        .Text(FText::FromString("+ Gen"))
                        .ToolTipText(FText::FromString("Add generator layer"))
                        .OnClicked_Lambda([this]() {
                          AddLayer(EKLayerType::Generator);
                          return FReply::Handled();
                        })]]

      // Main Content Area - Splitter with List on top, Details on bottom
      + SVerticalBox::Slot().FillHeight(1.0f).Padding(4)
            [SNew(SSplitter)
                 .Orientation(Orient_Vertical)
                 .Style(FAppStyle::Get(), "Splitter")
                 .PhysicalSplitterHandleSize(4.0f)

             // Top: Layer List (~35%)
             + SSplitter::Slot().Value(0.35f)
                   [SNew(SVerticalBox)

                    // Layer List
                    + SVerticalBox::Slot().FillHeight(1.0f)
                          [SNew(SBorder)
                               .BorderImage(FAppStyle::Get().GetBrush(
                                   "ToolPanel.DarkGroupBorder"))
                               .Padding(2)
                                   [SNew(SScrollBox)
                                    + SScrollBox::Slot()
                                          [SAssignNew(LayerListView,
                                                      SListView<TSharedPtr<FKLayerListItem>>)
                                               .ListItemsSource(&LayerListItems)
                                               .OnGenerateRow(this, &SMaterializeEditor::OnGenerateLayerRow)
                                               .OnSelectionChanged(this, &SMaterializeEditor::OnLayerSelectionChanged)
                                               .SelectionMode(ESelectionMode::Single)]]]

                    // Layer Actions Toolbar
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
                          [SNew(SHorizontalBox)

                           + SHorizontalBox::Slot().AutoWidth().Padding(1)
                                 [SNew(SButton)
                                      .ContentPadding(FMargin(6, 2))
                                      .Text(FText::FromString("Up"))
                                      .OnClicked_Lambda([this]() {
                                        MoveLayerUp();
                                        return FReply::Handled();
                                      })]

                           + SHorizontalBox::Slot().AutoWidth().Padding(1)
                                 [SNew(SButton)
                                      .ContentPadding(FMargin(6, 2))
                                      .Text(FText::FromString("Down"))
                                      .OnClicked_Lambda([this]() {
                                        MoveLayerDown();
                                        return FReply::Handled();
                                      })]

                           + SHorizontalBox::Slot().FillWidth(1.0f)

                           + SHorizontalBox::Slot().AutoWidth().Padding(1)
                                 [SNew(SButton)
                                      .ContentPadding(FMargin(6, 2))
                                      .Text(FText::FromString("Dup"))
                                      .OnClicked_Lambda([this]() {
                                        DuplicateSelectedLayer();
                                        return FReply::Handled();
                                      })]

                           + SHorizontalBox::Slot().AutoWidth().Padding(1)
                                 [SNew(SButton)
                                      .ContentPadding(FMargin(6, 2))
                                      .Text(FText::FromString("X"))
                                      .OnClicked_Lambda([this]() {
                                        RemoveSelectedLayer();
                                        return FReply::Handled();
                                      })]]]

             // Bottom: Layer Details View (~65%)
             + SSplitter::Slot().Value(0.65f)
                   [SNew(SBorder)
                        .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                        .Padding(4)
                            [SNew(SVerticalBox)

                             // Header
                             + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
                                   [SNew(STextBlock)
                                        .Text(FText::FromString("Layer Properties"))
                                        .Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))]

                             // Details View
                             + SVerticalBox::Slot().FillHeight(1.0f)
                                   [LayerDetailsView.ToSharedRef()]]]];
}

// =============================================================================
// LAYER LIST MANAGEMENT
// =============================================================================

TSharedRef<ITableRow> SMaterializeEditor::OnGenerateLayerRow(
    TSharedPtr<FKLayerListItem> Item,
    const TSharedRef<STableViewBase> &OwnerTable) {
  if (!Item.IsValid() || !Item->Layer) {
    return SNew(STableRow<TSharedPtr<FKLayerListItem>>, OwnerTable);
  }

  FKLayer &Layer = *Item->Layer;

  // Get layer type icon/label
  FString TypeLabel;
  FLinearColor TypeColor = FLinearColor(0.5f, 0.7f, 1.0f); // Default blue
  bool bIsBasePa = (Layer.LayerType == EKLayerType::Base);

  switch (Layer.LayerType) {
  case EKLayerType::Base:
    TypeLabel = TEXT("[BASE]");
    TypeColor = FLinearColor(1.0f, 0.85f, 0.3f); // Gold for foundation layer
    break;
  case EKLayerType::Image:
    TypeLabel = TEXT("[IMG]");
    break;
  case EKLayerType::Fill:
    TypeLabel = TEXT("[FIL]");
    break;
  case EKLayerType::Procedural:
    TypeLabel = TEXT("[PRO]");
    break;
  case EKLayerType::Filter:
    TypeLabel = TEXT("[FLT]");
    break;
  case EKLayerType::Adjustment:
    TypeLabel = TEXT("[ADJ]");
    break;
  case EKLayerType::Generator:
    TypeLabel = TEXT("[GEN]");
    break;
  case EKLayerType::Folder:
    TypeLabel = TEXT("[FLD]");
    break;
  }

  return SNew(STableRow<TSharedPtr<FKLayerListItem>>, OwnerTable)
      .Padding(FMargin(2))
          [SNew(SHorizontalBox)

           // Visibility Toggle
           +
           SHorizontalBox::Slot()
               .AutoWidth()
               .VAlign(VAlign_Center)
               .Padding(
                   2)[SNew(SCheckBox)
                          .IsChecked(Layer.bEnabled ? ECheckBoxState::Checked
                                                    : ECheckBoxState::Unchecked)
                          .OnCheckStateChanged_Lambda(
                              [this,
                               Index = Item->Index](ECheckBoxState NewState) {
                                if (ToolModel &&
                                    ToolModel->LayerStack.Layers.IsValidIndex(
                                        Index)) {
                                  ToolModel->LayerStack.Layers[Index].bEnabled =
                                      (NewState == ECheckBoxState::Checked);
                                  OnLayerStackChanged();
                                }
                              })]

           // Layer Type
           +
           SHorizontalBox::Slot()
               .AutoWidth()
               .VAlign(VAlign_Center)
               .Padding(2)[SNew(STextBlock)
                               .Text(FText::FromString(TypeLabel))
                               .Font(FAppStyle::Get().GetFontStyle("SmallFont"))
                               .ColorAndOpacity(TypeColor)]

           // Layer Name
           + SHorizontalBox::Slot()
                 .FillWidth(1.0f)
                 .VAlign(VAlign_Center)
                 .Padding(4, 0)[SNew(STextBlock)
                                    .Text(FText::FromName(Layer.Name))
                                    .Font(FAppStyle::Get().GetFontStyle(
                                        "NormalFont"))]

           // Opacity
           + SHorizontalBox::Slot()
                 .AutoWidth()
                 .VAlign(VAlign_Center)
                 .Padding(2)[SNew(SBox).WidthOverride(
                     40)[SNew(STextBlock)
                             .Text(FText::FromString(FString::Printf(
                                 TEXT("%.0f%%"), Layer.Opacity * 100.f)))
                             .Font(FAppStyle::Get().GetFontStyle("SmallFont"))
                             .Justification(ETextJustify::Right)]]];
}

void SMaterializeEditor::OnLayerSelectionChanged(TSharedPtr<FKLayerListItem> Item,
                                             ESelectInfo::Type SelectInfo) {
  if (Item.IsValid()) {
    SelectedLayerIndex = Item->Index;
  } else {
    SelectedLayerIndex = INDEX_NONE;
  }

  // Bind the selected layer to the property editor
  if (LayerPropertyEditor && ToolModel) {
    if (SelectedLayerIndex != INDEX_NONE) {
      LayerPropertyEditor->SetLayer(&ToolModel->LayerStack, SelectedLayerIndex);
      LayerDetailsView->SetObject(LayerPropertyEditor);
    } else {
      LayerPropertyEditor->Clear();
      LayerDetailsView->SetObject(nullptr);
    }
  }
}

void SMaterializeEditor::RefreshLayerList() {
  LayerListItems.Empty();

  if (ToolModel) {
    // Ensure Base layer exists when layers mode is enabled
    if (ToolModel->bUseLayers) {
      bool bHasBaseLayer = false;
      for (const FKLayer& Layer : ToolModel->LayerStack.Layers) {
        if (Layer.LayerType == EKLayerType::Base) {
          bHasBaseLayer = true;
          break;
        }
      }

      // Create the foundation Base layer if it doesn't exist
      if (!bHasBaseLayer) {
        FKLayer BaseLayer;
        BaseLayer.LayerType = EKLayerType::Base;
        BaseLayer.Name = FName(TEXT("Base Pass"));
        BaseLayer.Opacity = 1.0f;
        BaseLayer.bEnabled = true;
        BaseLayer.BlendMode = EKLayerBlendMode::Normal;
        // Insert at index 0 so it's always at the bottom of the stack
        ToolModel->LayerStack.Layers.Insert(BaseLayer, 0);
      }
    }

    // Build list from bottom to top (reverse order for display)
    for (int32 i = ToolModel->LayerStack.Layers.Num() - 1; i >= 0; --i) {
      LayerListItems.Add(
          MakeShared<FKLayerListItem>(i, &ToolModel->LayerStack.Layers[i]));
    }
  }

  if (LayerListView.IsValid()) {
    LayerListView->RequestListRefresh();
  }
}

void SMaterializeEditor::AddLayer(EKLayerType Type) {
  if (!ToolModel)
    return;

  // Enable layers mode
  ToolModel->bUseLayers = true;

  // Create new layer with default name
  FKLayer NewLayer;
  NewLayer.Id = FGuid::NewGuid();
  NewLayer.LayerType = Type;

  switch (Type) {
  case EKLayerType::Image:
    NewLayer.Name = FName(TEXT("Image Layer"));
    break;
  case EKLayerType::Fill:
    NewLayer.Name = FName(TEXT("Fill Layer"));
    NewLayer.FillColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
    break;
  case EKLayerType::Procedural:
    NewLayer.Name = FName(TEXT("Procedural Layer"));
    break;
  case EKLayerType::Filter:
    NewLayer.Name = FName(TEXT("Filter Layer"));
    break;
  case EKLayerType::Adjustment:
    NewLayer.Name = FName(TEXT("Adjustment Layer"));
    break;
  case EKLayerType::Generator:
    NewLayer.Name = FName(TEXT("Generator Layer"));
    break;
  default:
    NewLayer.Name = FName(TEXT("New Layer"));
    break;
  }

  ToolModel->LayerStack.AddLayer(NewLayer);
  RefreshLayerList();
  OnLayerStackChanged();
}

void SMaterializeEditor::RemoveSelectedLayer() {
  if (!ToolModel || SelectedLayerIndex == INDEX_NONE)
    return;

  // Prevent deletion of Base layer - it's the foundation
  if (ToolModel->LayerStack.Layers.IsValidIndex(SelectedLayerIndex) &&
      ToolModel->LayerStack.Layers[SelectedLayerIndex].LayerType == EKLayerType::Base) {
    return; // Cannot delete the base layer
  }

  if (ToolModel->LayerStack.RemoveLayer(SelectedLayerIndex)) {
    SelectedLayerIndex = INDEX_NONE;
    RefreshLayerList();
    OnLayerStackChanged();
  }
}

void SMaterializeEditor::DuplicateSelectedLayer() {
  if (!ToolModel || SelectedLayerIndex == INDEX_NONE)
    return;

  int32 NewIndex = ToolModel->LayerStack.DuplicateLayer(SelectedLayerIndex);
  if (NewIndex != INDEX_NONE) {
    RefreshLayerList();
    OnLayerStackChanged();
  }
}

void SMaterializeEditor::MoveLayerUp() {
  if (!ToolModel || SelectedLayerIndex == INDEX_NONE)
    return;

  int32 NewIndex = SelectedLayerIndex + 1;
  if (ToolModel->LayerStack.MoveLayer(SelectedLayerIndex, NewIndex)) {
    SelectedLayerIndex = NewIndex;
    RefreshLayerList();
    OnLayerStackChanged();
  }
}

void SMaterializeEditor::MoveLayerDown() {
  if (!ToolModel || SelectedLayerIndex == INDEX_NONE || SelectedLayerIndex == 0)
    return;

  int32 NewIndex = SelectedLayerIndex - 1;
  if (ToolModel->LayerStack.MoveLayer(SelectedLayerIndex, NewIndex)) {
    SelectedLayerIndex = NewIndex;
    RefreshLayerList();
    OnLayerStackChanged();
  }
}

void SMaterializeEditor::OnLayerStackChanged() {
  if (!ToolModel)
    return;

  // If we have layers, evaluate the stack
  if (ToolModel->bUseLayers && ToolModel->LayerStack.Layers.Num() > 0) {
    EvaluateLayerStack();
  }

  // Notify model changed for property updates
  ToolModel->OnModelChanged.Broadcast();
}

void SMaterializeEditor::EvaluateLayerStack() {
  if (!ToolModel)
    return;

  // Update status
  if (StatusText.IsValid()) {
    StatusText->SetText(FText::FromString(TEXT("Evaluating layers...")));
  }

  // Use the layer evaluator to process the stack
  FKLayerEvalResult EvalResult;
  FString EvalError;
  if (UKLayerEvaluator::EvaluateStack(ToolModel->LayerStack, EvalResult, EvalError)) {
    // Copy all channel results — BaseColor from layers takes priority over SourceTexture
    CachedResult.Normal    = EvalResult.Normal;
    CachedResult.Roughness = EvalResult.Roughness;
    CachedResult.Metallic  = EvalResult.Metallic;
    CachedResult.Height    = EvalResult.Height;
    CachedResult.AO        = EvalResult.AO;
    CachedResult.Emissive  = EvalResult.Emissive;
    // Use layer-evaluated BaseColor if produced, otherwise SourceTexture fallback
    // is handled inside SetPreviewMaterialFromResult
    CachedResult.LayerBaseColor = EvalResult.BaseColor;

    // Update preview
    SetPreviewMaterialFromResult(CachedResult);
    UpdateChannelThumbnails();

    if (StatusText.IsValid()) {
      StatusText->SetText(FText::FromString(FString::Printf(
          TEXT("Layers evaluated: %.1fms"), EvalResult.EvaluationTimeMs)));
    }
  } else {
    if (StatusText.IsValid()) {
      FString ErrorMsg = EvalError.IsEmpty() ? TEXT("Layer evaluation failed") : FString::Printf(TEXT("Layer evaluation failed: %s"), *EvalError);
      StatusText->SetText(FText::FromString(ErrorMsg));
    }
  }
}

void SMaterializeEditor::UpdateSelectedLayerProperties() {
  if (!LayerPropertiesBox.IsValid())
    return;

  LayerPropertiesBox->ClearChildren();

  if (!ToolModel || SelectedLayerIndex == INDEX_NONE ||
      !ToolModel->LayerStack.Layers.IsValidIndex(SelectedLayerIndex)) {
    // No layer selected, show placeholder
    LayerPropertiesBox->AddSlot().Padding(
        4)[SNew(STextBlock)
               .Text(FText::FromString("Select a layer to edit"))
               .Font(FAppStyle::Get().GetFontStyle("SmallFont"))
               .ColorAndOpacity(FSlateColor::UseSubduedForeground())];
    return;
  }

  FKLayer &Layer = ToolModel->LayerStack.Layers[SelectedLayerIndex];

  // Layer properties header
  LayerPropertiesBox->AddSlot().AutoHeight().Padding(
      4, 2)[SNew(STextBlock)
                .Text(FText::FromName(Layer.Name))
                .Font(FAppStyle::Get().GetFontStyle("HeadingExtraSmall"))];

  // Opacity slider
  LayerPropertiesBox->AddSlot().AutoHeight().Padding(
      4, 2)[SNew(SHorizontalBox)

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                  [SNew(STextBlock)
                       .Text(FText::FromString("Opacity:"))
                       .Font(FAppStyle::Get().GetFontStyle("SmallFont"))]

            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(
                  4, 0)[SNew(SSlider)
                            .Value(Layer.Opacity)
                            .OnValueChanged_Lambda([this](float NewValue) {
                              if (ToolModel &&
                                  ToolModel->LayerStack.Layers.IsValidIndex(
                                      SelectedLayerIndex)) {
                                ToolModel->LayerStack.Layers[SelectedLayerIndex]
                                    .Opacity = NewValue;
                                RefreshLayerList();
                                OnLayerStackChanged();
                              }
                            })]

            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                  [SNew(STextBlock)
                       .Text(FText::FromString(FString::Printf(
                           TEXT("%.0f%%"), Layer.Opacity * 100.f)))
                       .Font(FAppStyle::Get().GetFontStyle("SmallFont"))]];
}

// --- Workflow View Content Creation ---

TSharedRef<SWidget> SMaterializeEditor::CreateLayerViewContent() {
  // This is the existing Layer View content (Viewport + Channel Strip + Asset Picker)
  FContentBrowserModule &ContentBrowserModule =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          TEXT("ContentBrowser"));
  FAssetPickerConfig AssetPickerConfig;
  AssetPickerConfig.Filter.ClassPaths.Add(
      UTexture2D::StaticClass()->GetClassPathName());
  AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
  AssetPickerConfig.bAddFilterUI = true;
  AssetPickerConfig.bShowPathInColumnView = true;
  AssetPickerConfig.bSortByPathInColumnView = true;
  AssetPickerConfig.bForceShowEngineContent = true;
  AssetPickerConfig.OnAssetSelected =
      FOnAssetSelected::CreateSP(this, &SMaterializeEditor::OnAssetSelected);
  AssetPickerConfig.bShowBottomToolbar = true;
  AssetPickerConfig.bAutohideSearchBar = false;

  TSharedRef<SWidget> AssetPicker =
      ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig);

  return SNew(SSplitter)
      .Orientation(Orient_Vertical)
      .Style(FAppStyle::Get(), "Splitter")

      // Top: Viewport with Overlay
      + SSplitter::Slot().Value(0.65f)
            [SNew(SOverlay) +
             SOverlay::Slot()[ViewportWidget.ToSharedRef()] +
             SOverlay::Slot()
                 .VAlign(VAlign_Top)
                 .HAlign(HAlign_Center)
                 .Padding(10)[StatusText.ToSharedRef()] +
             SOverlay::Slot()
                 .VAlign(VAlign_Top)
                 .HAlign(HAlign_Left)
                 .Padding(10)
                     [SNew(STextBlock)
                          .Text_Lambda([this]() {
                            FString ModeStr;
                            switch (CurrentViewMode) {
                            case EMaterializeViewMode::Material:
                              ModeStr = TEXT("Material");
                              break;
                            case EMaterializeViewMode::BaseColor:
                              ModeStr = TEXT("Base Color");
                              break;
                            case EMaterializeViewMode::Normal:
                              ModeStr = TEXT("Normal");
                              break;
                            case EMaterializeViewMode::Roughness:
                              ModeStr = TEXT("Roughness");
                              break;
                            case EMaterializeViewMode::Metallic:
                              ModeStr = TEXT("Metallic");
                              break;
                            case EMaterializeViewMode::AO:
                              ModeStr = TEXT("AO");
                              break;
                            case EMaterializeViewMode::Height:
                              ModeStr = TEXT("Height");
                              break;
                            case EMaterializeViewMode::Emissive:
                              ModeStr = TEXT("Emissive");
                              break;
                            case EMaterializeViewMode::SplitAB:
                              ModeStr = TEXT("A/B Compare");
                              break;
                            }
                            return FText::FromString(ModeStr);
                          })
                          .Font(FAppStyle::Get().GetFontStyle("NormalFont"))
                          .ColorAndOpacity(FLinearColor(0.5f, 0.8f, 1.0f, 0.9f))]]

      // Middle: Channel Strip
      + SSplitter::Slot().Value(0.12f)
            [SNew(SBorder)
                 .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
                 .Padding(4)[CreateChannelStrip()]]

      // Bottom: Asset Picker
      + SSplitter::Slot().Value(0.23f)
            [SNew(SBorder)
                 .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                 .Padding(0)[AssetPicker]];
}

TSharedRef<SWidget> SMaterializeEditor::CreateGraphViewContent() {
  // Initialize graph editor if not already done
  if (!GraphEditorWidget.IsValid()) {
    InitializeGraphEditor();
  }

  // Layout mirrors UE5 Material Editor:
  //  [Palette 200px] | [Graph — fills remaining space] | [Viewport+Details 280px]
  return SNew(SSplitter)
      .Orientation(Orient_Horizontal)
      .Style(FAppStyle::Get(), "Splitter")
      .PhysicalSplitterHandleSize(5.0f)

      // Far Left: Node Palette
      + SSplitter::Slot().Value(0.15f)
            [SNew(SBorder)
                 .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                 .Padding(4)
                 [NodePalette.IsValid() ? NodePalette.ToSharedRef() : SNullWidget::NullWidget]]

      // Center: Graph Editor (main canvas)
      + SSplitter::Slot().Value(0.65f)
            [SNew(SBorder)
                 .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                 .Padding(0)
                 [GraphEditorWidget.IsValid() ? GraphEditorWidget.ToSharedRef() : SNullWidget::NullWidget]]

      // Right: Execute button + status (viewport lives in the outer right panel)
      + SSplitter::Slot().Value(0.20f)
            [SNew(SBorder)
                 .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                 .Padding(8)
                 [SNew(SVerticalBox)
                  + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                        [SNew(STextBlock)
                             .Text(FText::FromString("Graph Controls"))
                             .Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
                             .ColorAndOpacity(FLinearColor::White)]
                  + SVerticalBox::Slot().AutoHeight()
                        [SNew(SButton)
                             .HAlign(HAlign_Center)
                             .ButtonColorAndOpacity(FLinearColor(0.1f, 0.6f, 0.2f))
                             .ContentPadding(FMargin(12, 6))
                             .Text(FText::FromString("Execute Graph"))
                             .ToolTipText(FText::FromString("Run the node graph and update the 3D preview"))
                             .OnClicked_Lambda([this]() {
                               ExecuteGraph();
                               return FReply::Handled();
                             })]
                  + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
                        [SNew(STextBlock)
                             .Text_Lambda([this]() {
                               return StatusText.IsValid() ? StatusText->GetText() : FText::GetEmpty();
                             })
                             .Font(FAppStyle::Get().GetFontStyle("NormalFont"))
                             .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
                             .AutoWrapText(true)]]];
}

// --- Workflow Mode Switching ---

void SMaterializeEditor::SetWorkflowMode(EMaterializeWorkflowMode NewMode) {
  if (CurrentWorkflowMode == NewMode) {
    return;
  }

  // Store previous mode for potential rollback
  EMaterializeWorkflowMode PreviousMode = CurrentWorkflowMode;
  CurrentWorkflowMode = NewMode;

  // Trigger UI update (the widget switcher will automatically update based on the lambda)
  if (WorkflowContentSwitcher.IsValid()) {
    // Force widget switcher to update by setting active index directly
    int32 NewIndex = (NewMode == EMaterializeWorkflowMode::LayerView) ? 0 : 1;
    WorkflowContentSwitcher->SetActiveWidgetIndex(NewIndex);
  }

  // Update status text
  if (StatusText.IsValid()) {
    FString StatusStr = (NewMode == EMaterializeWorkflowMode::LayerView) 
        ? TEXT("Layer View - Photo to PBR") 
        : TEXT("Graph View - Procedural Designer");
    StatusText->SetText(FText::FromString(StatusStr));
  }

  // Restore DetailsView to show ToolModel when switching back to Layer View
  if (NewMode == EMaterializeWorkflowMode::LayerView) {
    if (DetailsView.IsValid() && ToolModel) {
      DetailsView->SetObject(ToolModel);
    }
  }

  // When switching to Graph View, ensure graph editor is initialized
  if (NewMode == EMaterializeWorkflowMode::GraphView) {
    if (!GraphEditorWidget.IsValid()) {
      InitializeGraphEditor();
    }
  }

  // Preserve viewport state - the viewport is shared between both views
  // so camera position, mesh, and material are automatically preserved

  // Log the switch for debugging
  UE_LOG(LogTemp, Log, TEXT("Switched workflow mode from %s to %s"),
         PreviousMode == EMaterializeWorkflowMode::LayerView ? TEXT("Layer View") : TEXT("Graph View"),
         NewMode == EMaterializeWorkflowMode::LayerView ? TEXT("Layer View") : TEXT("Graph View"));
}

// --- Graph Editor Initialization ---

void SMaterializeEditor::InitializeGraphEditor() {
  // Create a new graph if we don't have one
  if (!CurrentGraph) {
    CurrentGraph = NewObject<UMaterializeGraph>(GetTransientPackage(), NAME_None, RF_Transient);
    CurrentGraph->AddToRoot(); // Prevent GC

    // Only re-execute when wires are connected/disconnected, NOT on node placement.
    // Node placement fires NotifyGraphChanged before pins exist, causing pure-virtual crashes.
    // Execution is triggered explicitly via double-click or the Execute button.
  }

  // Create graph executor
  if (!GraphExecutor) {
    GraphExecutor = MakeUnique<FMaterializeGraphExecutor>();
  }

  // Create graph editor widget FIRST (before palette)
  if (!GraphEditorWidget.IsValid()) {
    SGraphEditor::FGraphEditorEvents GraphEvents;
    GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(
        this, &SMaterializeEditor::OnGraphNodeSelectionChanged);
    
    // Trigger preview update when nodes are moved or connections change
    GraphEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateLambda([this](UEdGraphNode* Node) {
      // Execute graph and update preview when node is double-clicked
      ExecuteGraph();
    });

    GraphEditorCommands = MakeShareable(new FUICommandList());

    GraphEditorWidget = SNew(SGraphEditor)
        .AdditionalCommands(GraphEditorCommands)
        .IsEditable(true)
        .GraphToEdit(CurrentGraph)  // UMaterializeGraph inherits from UEdGraph
        .GraphEvents(GraphEvents)
        .ShowGraphStateOverlay(false);

    BindGraphCommands();
  }

  // Create node palette AFTER graph editor widget exists
  if (!NodePalette.IsValid()) {
    SAssignNew(NodePalette, SMaterializeNodePalette)
        .GraphSchema(Cast<UMaterializeGraphSchema>(CurrentGraph->GetSchema()));
  }
}

void SMaterializeEditor::OnGraphNodeSelectionChanged(const TSet<UObject*>& SelectedNodes) {
  // Update details panel when nodes are selected
  if (DetailsView.IsValid() && SelectedNodes.Num() > 0) {
    TArray<UObject*> SelectedArray = SelectedNodes.Array();
    DetailsView->SetObjects(SelectedArray);
    
    // Set up property change callback for the selected nodes
    DetailsView->OnFinishedChangingProperties().Clear();
    DetailsView->OnFinishedChangingProperties().AddSP(
        this, &SMaterializeEditor::OnGraphNodePropertyChanged);
  } else if (DetailsView.IsValid()) {
    // Clear details view if no nodes selected
    DetailsView->SetObject(nullptr);
  }

  // Trigger preview update when selection changes
  // This ensures the preview reflects the current graph state
  ExecuteGraph();
}

void SMaterializeEditor::OnGraphNodePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent) {
  // When a node property changes, trigger graph execution and preview update
  UE_LOG(LogTemp, Log, TEXT("Graph node property changed: %s"), 
         PropertyChangedEvent.Property ? *PropertyChangedEvent.Property->GetName() : TEXT("Unknown"));
  
  // Invalidate and update preview
  InvalidateGraphPreview();
  ExecuteGraph();
}

void SMaterializeEditor::InvalidateGraphPreview() {
  // Request viewport redraw by calling Invalidate without parameters
  if (ViewportWidget.IsValid()) {
    ViewportWidget->Invalidate();
  }
  
  // Update status to show that preview is being updated
  if (StatusText.IsValid()) {
    StatusText->SetText(FText::FromString("Updating preview..."));
  }
}

void SMaterializeEditor::ExecuteGraph() {
  if (!CurrentGraph || !GraphExecutor) {
    return;
  }

  // Only execute if we're in Graph View mode
  if (CurrentWorkflowMode != EMaterializeWorkflowMode::GraphView) {
    return;
  }

  if (StatusText.IsValid()) {
    StatusText->SetText(FText::FromString("Executing graph..."));
  }

  // Determine output resolution from ToolModel or fall back to defaults
  const int32 Width  = (ToolModel && ToolModel->OutputResolution > 0) ? ToolModel->OutputResolution : 1024;
  const int32 Height = Width;

  // Execute the full node graph via the compiled executor
  FMaterializeGraphExecutionResult GraphResult = GraphExecutor->Execute(CurrentGraph, Width, Height);

  if (!GraphResult.IsValid()) {
    // Combine all errors into a single status string
    FString CombinedErrors;
    for (const FString& Err : GraphResult.Errors) {
      if (!CombinedErrors.IsEmpty()) CombinedErrors += TEXT("; ");
      CombinedErrors += Err;
    }
    UE_LOG(LogTemp, Error, TEXT("Graph execution failed: %s"), *CombinedErrors);
    if (StatusText.IsValid()) {
      StatusText->SetText(FText::FromString(FString::Printf(TEXT("Graph Error: %s"), *CombinedErrors)));
    }
    return;
  }

  // Convert FMaterializeGraphExecutionResult → FMaterializeResult so the
  // existing preview pipeline (SetPreviewMaterialFromResult / UpdateChannelThumbnails) works.
  CachedResult = FMaterializeResult();
  CachedResult.Normal    = GraphResult.Normal;
  CachedResult.Roughness = GraphResult.Roughness;
  CachedResult.Metallic  = GraphResult.Metallic;
  CachedResult.AO        = GraphResult.AO;
  CachedResult.Height    = GraphResult.Height;
  CachedResult.Emissive  = GraphResult.Emissive;
  CachedResult.GenerationTimeMs = GraphResult.ExecutionTimeMs;

  // Feed graph BaseColor as the source texture for the preview material if present.
  // The layer/compute path uses ToolModel->SourceTexture as BaseColor, but
  // the graph can produce its own BaseColor channel.
  SetPreviewMaterialFromGraphResult(GraphResult);
  UpdateChannelThumbnails();

  if (StatusText.IsValid()) {
    StatusText->SetText(FText::Format(
        FText::FromString("Graph: {0} nodes in {1} ms"),
        FText::AsNumber(GraphResult.NodesExecuted),
        FText::AsNumber(FMath::RoundToInt(GraphResult.ExecutionTimeMs))));
  }

  // Update node preview thumbnails so the graph editor shows intermediate results
  GraphExecutor->ExecuteWithPreviews(CurrentGraph, 256, 256);

  if (ViewportWidget.IsValid()) {
    ViewportWidget->Invalidate();
  }
}

void SMaterializeEditor::BindGraphCommands() {
  if (!GraphEditorWidget.IsValid()) {
    return;
  }

  // Bind standard editing actions onto the shared command list that was given
  // to the graph editor on creation.
  if (!GraphEditorCommands.IsValid()) {
    return;
  }

  GraphEditorCommands->MapAction(
      FGenericCommands::Get().Delete,
      FExecuteAction::CreateSP(this, &SMaterializeEditor::DeleteSelectedGraphNodes),
      FCanExecuteAction::CreateLambda([this]() {
        return GraphEditorWidget.IsValid() && GraphEditorWidget->GetSelectedNodes().Num() > 0;
      }));

  GraphEditorCommands->MapAction(
      FGenericCommands::Get().Copy,
      FExecuteAction::CreateSP(this, &SMaterializeEditor::CopySelectedGraphNodes),
      FCanExecuteAction::CreateLambda([this]() {
        return GraphEditorWidget.IsValid() && GraphEditorWidget->GetSelectedNodes().Num() > 0;
      }));

  GraphEditorCommands->MapAction(
      FGenericCommands::Get().Paste,
      FExecuteAction::CreateSP(this, &SMaterializeEditor::PasteGraphNodes),
      FCanExecuteAction::CreateLambda([this]() {
        return CurrentGraph != nullptr;
      }));

  GraphEditorCommands->MapAction(
      FGenericCommands::Get().Duplicate,
      FExecuteAction::CreateSP(this, &SMaterializeEditor::DuplicateGraphNodes),
      FCanExecuteAction::CreateLambda([this]() {
        return GraphEditorWidget.IsValid() && GraphEditorWidget->GetSelectedNodes().Num() > 0;
      }));
}

void SMaterializeEditor::DeleteSelectedGraphNodes() {
  if (!GraphEditorWidget.IsValid() || !CurrentGraph) {
    return;
  }

  const TSet<UObject*>& SelectedNodes = GraphEditorWidget->GetSelectedNodes();
  if (SelectedNodes.Num() == 0) {
    return;
  }

  const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Graph Nodes")));
  CurrentGraph->Modify();

  // Collect deletable nodes first (some nodes may not allow deletion)
  TArray<UEdGraphNode*> NodesToDelete;
  for (UObject* Obj : SelectedNodes) {
    UEdGraphNode* Node = Cast<UEdGraphNode>(Obj);
    if (Node && Node->CanUserDeleteNode()) {
      NodesToDelete.Add(Node);
    }
  }

  for (UEdGraphNode* Node : NodesToDelete) {
    // Break all pin connections
    Node->BreakAllNodeLinks();
    // Remove from graph
    if (UMaterializeGraphNode* MatNode = Cast<UMaterializeGraphNode>(Node)) {
      CurrentGraph->RemoveNode(MatNode);
    }
  }

  // Clear selection and re-execute
  GraphEditorWidget->ClearSelectionSet();
  ExecuteGraph();
}

void SMaterializeEditor::CopySelectedGraphNodes() {
  if (!GraphEditorWidget.IsValid()) {
    return;
  }

  const TSet<UObject*>& SelectedNodes = GraphEditorWidget->GetSelectedNodes();

  // Filter to only copyable nodes
  TSet<UObject*> CopyableNodes;
  for (UObject* Obj : SelectedNodes) {
    UEdGraphNode* Node = Cast<UEdGraphNode>(Obj);
    if (Node && Node->CanDuplicateNode()) {
      Node->PrepareForCopying();
      CopyableNodes.Add(Node);
    }
  }

  if (CopyableNodes.Num() > 0) {
    FString ExportedText;
    FEdGraphUtilities::ExportNodesToText(CopyableNodes, ExportedText);
    FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
  }
}

void SMaterializeEditor::PasteGraphNodes() {
  if (!CurrentGraph || !GraphEditorWidget.IsValid()) {
    return;
  }

  FString ClipboardText;
  FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

  if (ClipboardText.IsEmpty()) {
    return;
  }

  const FScopedTransaction Transaction(FText::FromString(TEXT("Paste Graph Nodes")));
  CurrentGraph->Modify();

  // Determine paste location (center of current view)
  FVector2D PasteLocation = GraphEditorWidget->GetPasteLocation();

  TSet<UEdGraphNode*> PastedNodes;
  FEdGraphUtilities::ImportNodesFromText(CurrentGraph, ClipboardText, PastedNodes);

  // Offset pasted nodes and select them
  GraphEditorWidget->ClearSelectionSet();
  for (UEdGraphNode* Node : PastedNodes) {
    Node->NodePosX += PasteLocation.X;
    Node->NodePosY += PasteLocation.Y;
    Node->CreateNewGuid();
    GraphEditorWidget->SetNodeSelection(Node, true);
  }

  GraphEditorWidget->NotifyGraphChanged();
  ExecuteGraph();
}

void SMaterializeEditor::DuplicateGraphNodes() {
  CopySelectedGraphNodes();
  PasteGraphNodes();
}

// --- FGCObject Interface ---

void SMaterializeEditor::AddReferencedObjects(FReferenceCollector &Collector) {
  Collector.AddReferencedObject(CachedResult.Normal);
  Collector.AddReferencedObject(CachedResult.Roughness);
  Collector.AddReferencedObject(CachedResult.Metallic);
  Collector.AddReferencedObject(CachedResult.AO);
  Collector.AddReferencedObject(CachedResult.Height);
  Collector.AddReferencedObject(CachedResult.Emissive);
  Collector.AddReferencedObject(CachedResult.ORM);

  Collector.AddReferencedObject(ToolModel);
  Collector.AddReferencedObject(LastPreviewMesh);
  Collector.AddReferencedObject(LayerPropertyEditor);
  Collector.AddReferencedObject(CurrentGraph);
}

FString SMaterializeEditor::GetReferencerName() const {
  return TEXT("SMaterializeEditor");
}

void SMaterializeEditorViewport::Construct(
    const FArguments &InArgs, TSharedPtr<FAdvancedPreviewScene> InPreviewScene,
    TSharedPtr<FMaterializeEditorViewportClient> &OutClient) {
  PreviewScene = InPreviewScene;
  this->SEditorViewport::Construct(SEditorViewport::FArguments());
  OutClient = Client;
}

TSharedRef<FEditorViewportClient>
SMaterializeEditorViewport::MakeEditorViewportClient() {
  Client =
      MakeShared<FMaterializeEditorViewportClient>(SharedThis(this), PreviewScene);
  return Client.ToSharedRef();
}

// ============================================================================
// PRESET SYSTEM IMPLEMENTATION
// ============================================================================

void SMaterializeEditor::InitializePresets() {
	// Initialize preset registry if not already done
	FMaterializePresetRegistry::Initialize();
	
	// Get all available presets
	TArray<FMaterializeMasterPreset> Presets = FMaterializePresetRegistry::GetAllPresets();
	
	// Convert to shared pointers for UI
	AvailablePresets.Empty();
	for (const FMaterializeMasterPreset& Preset : Presets) {
		AvailablePresets.Add(MakeShared<FMaterializeMasterPreset>(Preset));
	}
	
	// Set default preset if none selected
	if (!CurrentPreset.IsValid() && AvailablePresets.Num() > 0) {
		CurrentPreset = AvailablePresets[0]; // Default to first preset (Standard)
	}
}

void SMaterializeEditor::LoadPresetFromEditorPrefs() {
	// Load last selected preset from editor preferences
	const FString PresetKey = TEXT("Materialize.LastSelectedPreset");
	FString LastPresetId;
	
	if (GConfig->GetString(TEXT("MaterializeEditor"), *PresetKey, LastPresetId, GEditorPerProjectIni)) {
		// Find preset by ID
		for (const TSharedPtr<FMaterializeMasterPreset>& Preset : AvailablePresets) {
			if (Preset.IsValid() && Preset->PresetId.ToString() == LastPresetId) {
				CurrentPreset = Preset;
				break;
			}
		}
	}
	
	// Fallback to default if not found
	if (!CurrentPreset.IsValid() && AvailablePresets.Num() > 0) {
		CurrentPreset = AvailablePresets[0];
	}
}

void SMaterializeEditor::SavePresetToEditorPrefs() {
	if (!CurrentPreset.IsValid()) {
		return;
	}
	
	// Save current preset to editor preferences
	const FString PresetKey = TEXT("Materialize.LastSelectedPreset");
	GConfig->SetString(TEXT("MaterializeEditor"), *PresetKey, *CurrentPreset->PresetId.ToString(), GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

void SMaterializeEditor::OnPresetSelected(TSharedPtr<FMaterializeMasterPreset> NewPreset, ESelectInfo::Type SelectInfo) {
	if (!NewPreset.IsValid() || NewPreset == CurrentPreset) {
		return;
	}
	
	CurrentPreset = NewPreset;
	
	// Save selection to preferences
	SavePresetToEditorPrefs();
	
	// Update preview with new preset
	if (CachedResult.IsValid()) {
		SetPreviewMaterialFromResult(CachedResult);
	}
	
	// Log preset change
	UE_LOG(LogTemp, Log, TEXT("Materialize: Preset changed to %s"), *CurrentPreset->DisplayName.ToString());
}

FText SMaterializeEditor::GetCurrentPresetText() const {
	if (CurrentPreset.IsValid()) {
		return CurrentPreset->DisplayName;
	}
	return FText::FromString(TEXT("Standard PBR"));
}

TSharedRef<SWidget> SMaterializeEditor::GeneratePresetWidget(TSharedPtr<FMaterializeMasterPreset> Preset) {
	if (!Preset.IsValid()) {
		return SNullWidget::NullWidget;
	}
	
	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SVerticalBox)
			
			// Preset name
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Preset->DisplayName)
				.Font(FAppStyle::Get().GetFontStyle("NormalFont"))
			]
			
			// Preset description (if available)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Preset->Description)
				.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Visibility(Preset->Description.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
			]
		];
}

TSharedRef<SWidget> SMaterializeEditor::CreatePresetSelector() {
	return SNew(SHorizontalBox)
		
		// Label
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Preset:")))
			.Font(FAppStyle::Get().GetFontStyle("NormalFont"))
		]
		
		// Combo box
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(PresetSelector, SComboBox<TSharedPtr<FMaterializeMasterPreset>>)
			.OptionsSource(&AvailablePresets)
			.OnGenerateWidget(this, &SMaterializeEditor::GeneratePresetWidget)
			.OnSelectionChanged(this, &SMaterializeEditor::OnPresetSelected)
			.InitiallySelectedItem(CurrentPreset)
			[
				SNew(STextBlock)
				.Text(this, &SMaterializeEditor::GetCurrentPresetText)
				.Font(FAppStyle::Get().GetFontStyle("NormalFont"))
			]
		];
}
