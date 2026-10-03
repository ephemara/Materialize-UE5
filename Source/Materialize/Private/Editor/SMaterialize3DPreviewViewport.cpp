// Copyright K-Studio. All Rights Reserved.

#include "Editor/SMaterialize3DPreviewViewport.h"
#include "Graph/MaterializeGraph.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "EditorViewportCommands.h"

// ============================================================================
// FMaterialize3DPreviewViewportClient
// ============================================================================

FMaterialize3DPreviewViewportClient::FMaterialize3DPreviewViewportClient(FAdvancedPreviewScene* InPreviewScene)
	: FEditorViewportClient(nullptr, InPreviewScene)
	, PreviewScene(InPreviewScene)
	, PreviewMaterial(nullptr)
	, PreviewMeshComponent(nullptr)
{
	// Set up the viewport
	SetRealtime(true);
	
	// Camera setup
	SetViewLocation(FVector(300.0f, 300.0f, 200.0f));
	SetViewRotation(FRotator(-20.0f, -135.0f, 0.0f));

	// Create a simple preview material
	UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (BaseMaterial)
	{
		PreviewMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
	}

	// Add a preview sphere
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh && PreviewScene)
	{
		PreviewMeshComponent = NewObject<UStaticMeshComponent>();
		PreviewMeshComponent->SetStaticMesh(SphereMesh);
		PreviewMeshComponent->SetRelativeScale3D(FVector(2.0f));
		if (PreviewMaterial)
		{
			PreviewMeshComponent->SetMaterial(0, PreviewMaterial);
		}
		PreviewScene->AddComponent(PreviewMeshComponent, FTransform::Identity);
	}
}

FMaterialize3DPreviewViewportClient::~FMaterialize3DPreviewViewportClient()
{
	if (PreviewMeshComponent && PreviewScene)
	{
		PreviewScene->RemoveComponent(PreviewMeshComponent);
	}
}

void FMaterialize3DPreviewViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);
	
	if (PreviewScene)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

void FMaterialize3DPreviewViewportClient::UpdateMaterial(
	UTexture2D* BaseColor, 
	UTexture2D* Normal, 
	UTexture2D* Roughness, 
	UTexture2D* Metallic, 
	UTexture2D* Height, 
	UTexture2D* AO, 
	UTexture2D* Emissive)
{
	if (!PreviewMaterial)
	{
		return;
	}

	// Update material parameters if they exist on this material
	// Note: DefaultMaterial might not have all these parameters, but user can swap to a proper PBR material
	if (BaseColor)
	{
		PreviewMaterial->SetTextureParameterValue(FName("BaseColor"), BaseColor);
		PreviewMaterial->SetTextureParameterValue(FName("Diffuse"), BaseColor);
	}
	if (Normal)
	{
		PreviewMaterial->SetTextureParameterValue(FName("Normal"), Normal);
		PreviewMaterial->SetTextureParameterValue(FName("NormalMap"), Normal);
	}
	if (Roughness)
	{
		PreviewMaterial->SetTextureParameterValue(FName("Roughness"), Roughness);
	}
	if (Metallic)
	{
		PreviewMaterial->SetTextureParameterValue(FName("Metallic"), Metallic);
	}
	if (Height)
	{
		PreviewMaterial->SetTextureParameterValue(FName("Height"), Height);
		PreviewMaterial->SetTextureParameterValue(FName("Displacement"), Height);
	}
	if (AO)
	{
		PreviewMaterial->SetTextureParameterValue(FName("AO"), AO);
		PreviewMaterial->SetTextureParameterValue(FName("AmbientOcclusion"), AO);
	}
	if (Emissive)
	{
		PreviewMaterial->SetTextureParameterValue(FName("Emissive"), Emissive);
	}
}

void FMaterialize3DPreviewViewportClient::SetPreviewMesh(UStaticMesh* InMesh)
{
	if (PreviewMeshComponent && InMesh)
	{
		PreviewMeshComponent->SetStaticMesh(InMesh);
	}
}

// ============================================================================
// SMaterialize3DPreviewViewport
// ============================================================================

void SMaterialize3DPreviewViewport::Construct(const FArguments& InArgs)
{
	Graph = InArgs._Graph;

	// Create preview scene
	PreviewScene = MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues()));
	PreviewScene->SetFloorVisibility(true);

	// Parent construct
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SMaterialize3DPreviewViewport::~SMaterialize3DPreviewViewport()
{
	if (ViewportClient.IsValid())
	{
		ViewportClient->Viewport = nullptr;
	}
}

void SMaterialize3DPreviewViewport::UpdatePreview(
	UTexture2D* BaseColor, 
	UTexture2D* Normal, 
	UTexture2D* Roughness, 
	UTexture2D* Metallic, 
	UTexture2D* Height, 
	UTexture2D* AO, 
	UTexture2D* Emissive)
{
	if (ViewportClient.IsValid())
	{
		ViewportClient->UpdateMaterial(BaseColor, Normal, Roughness, Metallic, Height, AO, Emissive);
	}
}

TSharedRef<FEditorViewportClient> SMaterialize3DPreviewViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShareable(new FMaterialize3DPreviewViewportClient(PreviewScene.Get()));
	return ViewportClient.ToSharedRef();
}

