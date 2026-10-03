#include "MaterializeEditorViewportClient.h"
#include "Components/StaticMeshComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UnrealEdGlobals.h"

FMaterializeEditorViewportClient::FMaterializeEditorViewportClient(
    const TSharedRef<SEditorViewport> &InViewportWidget,
    TSharedPtr<FAdvancedPreviewScene> InPreviewScene)
    : FEditorViewportClient(nullptr, InPreviewScene.Get(), InViewportWidget),
      PreviewScene(InPreviewScene) {
  SetViewLocation(FVector(0, -250, 0));
  SetViewRotation(FRotator(0, 0, 0));
  SetViewModes(VMI_Lit, VMI_Lit);
  EngineShowFlags.SetSelectionOutline(false);

  UStaticMesh *Mesh = LoadObject<UStaticMesh>(
      nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  if (Mesh) {
    PreviewMeshComponent = NewObject<UStaticMeshComponent>();
    PreviewMeshComponent->SetStaticMesh(Mesh);
    PreviewMeshComponent->SetMobility(EComponentMobility::Movable);
    PreviewMeshComponent->SetWorldScale3D(FVector(1.0f));
    PreviewScene->AddComponent(PreviewMeshComponent, FTransform::Identity);
  }
}

void FMaterializeEditorViewportClient::SetPreviewMaterial(
    UMaterialInterface *InMaterial) {
  if (PreviewMeshComponent && InMaterial) {
    PreviewMeshComponent->SetMaterial(0, InMaterial);

    // Force the mesh component to update its render state for immediate refresh
    PreviewMeshComponent->MarkRenderStateDirty();

    // Invalidate the viewport to trigger immediate redraw
    Invalidate();
  }
}

void FMaterializeEditorViewportClient::SetPreviewMesh(UStaticMesh *InMesh, bool bAutoFocus) {
  if (PreviewMeshComponent && InMesh) {
    PreviewMeshComponent->SetStaticMesh(InMesh);
    PreviewMeshComponent->MarkRenderStateDirty();

    if (bAutoFocus) {
      FocusOnPreviewMesh(true);
    }

    Invalidate();
  }
}

void FMaterializeEditorViewportClient::FocusOnPreviewMesh(bool bInstant) {
  if (!PreviewMeshComponent || !PreviewMeshComponent->GetStaticMesh()) {
    return;
  }

  if (Viewport == nullptr) {
    return;
  }

  PreviewMeshComponent->UpdateBounds();
  const FBox FocusBounds =
      PreviewMeshComponent->CalcBounds(PreviewMeshComponent->GetComponentTransform()).GetBox();
  if (!FocusBounds.IsValid) {
    return;
  }

  FocusViewportOnBox(FocusBounds, bInstant);
  Invalidate();
}
