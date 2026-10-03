// Copyright K-Studio. All Rights Reserved.

#include "Editor/Preview/MaterializePreviewViewport.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "Misc/App.h"

// ============================================================================
// FMaterializePreviewElement
// ============================================================================

FMaterializePreviewElement::FMaterializePreviewElement()
	: RenderTarget(new FMaterializePreviewRenderTarget())
	, PreviewTexture(nullptr)
	, bIsRealtime(false)
{
}

FMaterializePreviewElement::~FMaterializePreviewElement()
{
	delete RenderTarget;
}

bool FMaterializePreviewElement::BeginRenderingCanvas(
	const FIntRect& InCanvasRect,
	const FIntRect& InClippingRect,
	UTexture* InPreviewTexture,
	bool bInIsRealtime)
{
	if (InCanvasRect.Size().X > 0 && InCanvasRect.Size().Y > 0 && 
		InClippingRect.Size().X > 0 && InClippingRect.Size().Y > 0)
	{
		struct FPreviewRenderInfo
		{
			FIntRect CanvasRect;
			FIntRect ClippingRect;
			UTexture* Texture;
			bool bIsRealtime;
		};

		FPreviewRenderInfo RenderInfo;
		RenderInfo.CanvasRect = InCanvasRect;
		RenderInfo.ClippingRect = InClippingRect;
		RenderInfo.Texture = InPreviewTexture;
		RenderInfo.bIsRealtime = bInIsRealtime;

		FMaterializePreviewElement* PreviewElement = this;
		ENQUEUE_RENDER_COMMAND(BeginRenderingKSamplePreview)(
			[PreviewElement, RenderInfo](FRHICommandListImmediate& RHICmdList)
			{
				PreviewElement->RenderTarget->SetViewRect(RenderInfo.CanvasRect);
				PreviewElement->RenderTarget->SetClippingRect(RenderInfo.ClippingRect);
				PreviewElement->PreviewTexture = RenderInfo.Texture;
				PreviewElement->bIsRealtime = RenderInfo.bIsRealtime;
			}
		);
		return true;
	}
	return false;
}

void FMaterializePreviewElement::UpdatePreviewTexture(UTexture* InTexture)
{
	FMaterializePreviewElement* PreviewElement = this;
	ENQUEUE_RENDER_COMMAND(UpdateKSamplePreviewTexture)(
		[PreviewElement, InTexture](FRHICommandListImmediate& RHICmdList)
		{
			PreviewElement->PreviewTexture = InTexture;
		}
	);
}

void FMaterializePreviewElement::DrawRenderThread(FRHICommandListImmediate& RHICmdList, const void* InRenderTarget)
{
	if (PreviewTexture && PreviewTexture->GetResource())
	{
		// Pass current time for animated effects
		double CurrentTime = bIsRealtime ? (FApp::GetCurrentTime() - GStartTime) : 0.0;
		float DeltaTime = bIsRealtime ? static_cast<float>(FApp::GetDeltaTime()) : 0.0f;

		FCanvas Canvas(
			RenderTarget, 
			nullptr, 
			FGameTime::CreateUndilated(CurrentTime, DeltaTime), 
			GMaxRHIFeatureLevel);
		{
			Canvas.SetAllowedModes(0);
			Canvas.SetRenderTargetRect(RenderTarget->GetViewRect());
			Canvas.SetRenderTargetScissorRect(RenderTarget->GetClippingRect());

			// Draw the preview texture as a tile
			FCanvasTileItem TileItem(
				FVector2D::ZeroVector, 
				PreviewTexture->GetResource(), 
				FVector2D(RenderTarget->GetSizeXY()),
				FLinearColor::White);
			TileItem.BlendMode = SE_BLEND_Opaque;
			Canvas.DrawItem(TileItem);
			
			Canvas.Flush_RenderThread(RHICmdList, true);
		}
	}
}

// ============================================================================
// FMaterializePreviewViewport
// ============================================================================

FMaterializePreviewViewport::FMaterializePreviewViewport(UMaterializeGraphNode* InNode)
	: GraphNode(InNode)
	, PreviewElement(MakeShareable(new FMaterializePreviewElement()))
	, bIsRealtime(false)
{
}

FMaterializePreviewViewport::~FMaterializePreviewViewport()
{
	// Pass the preview element to render thread for safe deletion
	ENQUEUE_RENDER_COMMAND(SafeDeleteKSamplePreviewElement)(
		[PreviewElement = PreviewElement](FRHICommandListImmediate& RHICmdList) mutable
		{
			PreviewElement.Reset();
		}
	);
}

FIntPoint FMaterializePreviewViewport::GetSize() const
{
	// Standard preview size (matches Material Editor)
	return FIntPoint(96, 96);
}

FSlateShaderResource* FMaterializePreviewViewport::GetViewportRenderTargetTexture() const
{
	// We don't use a persistent render target texture - we draw directly
	return nullptr;
}

void FMaterializePreviewViewport::UpdatePreview(UTexture* InTexture)
{
	if (PreviewElement.IsValid())
	{
		PreviewElement->UpdatePreviewTexture(InTexture);
	}
}
