// Copyright K-Studio. All Rights Reserved.
// 3D Preview Viewport for Materialize Graph - Shows material applied to mesh

#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "SEditorViewport.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "AdvancedPreviewScene.h"

class UMaterializeGraph;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * Viewport client for Materialize 3D preview
 */
class FMaterialize3DPreviewViewportClient : public FEditorViewportClient
{
public:
	FMaterialize3DPreviewViewportClient(FAdvancedPreviewScene* InPreviewScene);
	virtual ~FMaterialize3DPreviewViewportClient();

	// FEditorViewportClient interface
	virtual void Tick(float DeltaSeconds) override;

	/** Update the preview material with new textures */
	void UpdateMaterial(UTexture2D* BaseColor, UTexture2D* Normal, UTexture2D* Roughness, 
		UTexture2D* Metallic, UTexture2D* Height, UTexture2D* AO, UTexture2D* Emissive);

	/** Set the preview mesh */
	void SetPreviewMesh(UStaticMesh* InMesh);

private:
	FAdvancedPreviewScene* PreviewScene;
	UMaterialInstanceDynamic* PreviewMaterial;
	UStaticMeshComponent* PreviewMeshComponent;
};

/**
 * Slate viewport widget for Materialize 3D preview
 */
class SMaterialize3DPreviewViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SMaterialize3DPreviewViewport) {}
		SLATE_ARGUMENT(UMaterializeGraph*, Graph)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMaterialize3DPreviewViewport();

	/** Update the preview with new textures */
	void UpdatePreview(UTexture2D* BaseColor, UTexture2D* Normal, UTexture2D* Roughness,
		UTexture2D* Metallic, UTexture2D* Height, UTexture2D* AO, UTexture2D* Emissive);

	// SEditorViewport interface
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

	// ICommonEditorViewportToolbarInfoProvider interface
	virtual TSharedRef<class SEditorViewport> GetViewportWidget() override { return SharedThis(this); }
	virtual TSharedPtr<FExtender> GetExtenders() const override { return nullptr; }
	virtual void OnFloatingButtonClicked() override {}

private:
	TWeakObjectPtr<UMaterializeGraph> Graph;
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FMaterialize3DPreviewViewportClient> ViewportClient;
};
