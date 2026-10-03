#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "AdvancedPreviewScene.h"

class MATERIALIZE_API FMaterializeEditorViewportClient : public FEditorViewportClient
{
public:
	FMaterializeEditorViewportClient(const TSharedRef<SEditorViewport>& InViewportWidget, TSharedPtr<FAdvancedPreviewScene> InPreviewScene);
	void SetPreviewMaterial(UMaterialInterface* InMaterial);
	void SetPreviewMesh(UStaticMesh* InMesh, bool bAutoFocus = true);
	void FocusOnPreviewMesh(bool bInstant = true);
	TSharedPtr<FAdvancedPreviewScene> GetPreviewScene() const { return PreviewScene; }
private:
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	UStaticMeshComponent* PreviewMeshComponent = nullptr;
};

