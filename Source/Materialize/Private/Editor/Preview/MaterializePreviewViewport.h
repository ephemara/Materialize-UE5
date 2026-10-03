// Copyright K-Studio. All Rights Reserved.
// Live GPU Preview System - Based on Epic's Material Editor patterns

#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"
#include "CanvasTypes.h"
#include "Rendering/RenderingCommon.h"

class UMaterializeGraphNode;
class UTexture;
class UTexture2D;
class FSlateShaderResource;

/**
 * Simple render target for the node preview canvas
 * Thread-safe access from render thread only
 */
class FMaterializePreviewRenderTarget : public FRenderTarget
{
public:
	virtual FIntPoint GetSizeXY() const override
	{
		return ClippingRect.Size();
	}

	void SetViewRect(const FIntRect& InViewRect)
	{
		ViewRect = InViewRect;
	}

	const FIntRect& GetViewRect() const
	{
		return ViewRect;
	}

	void SetClippingRect(const FIntRect& InClippingRect)
	{
		ClippingRect = InClippingRect;
	}

	const FIntRect& GetClippingRect() const
	{
		return ClippingRect;
	}

private:
	FIntRect ViewRect;
	FIntRect ClippingRect;
};

/**
 * Custom Slate element for rendering node previews on the render thread
 * UE 5.4 compatible version
 */
class FMaterializePreviewElement : public ICustomSlateElement
{
public:
	FMaterializePreviewElement();
	virtual ~FMaterializePreviewElement();

	/**
	 * Prepares the canvas for rendering
	 */
	bool BeginRenderingCanvas(
		const FIntRect& InCanvasRect, 
		const FIntRect& InClippingRect,
		UTexture* InPreviewTexture,
		bool bInIsRealtime);

	/**
	 * Updates the preview texture (thread-safe)
	 */
	void UpdatePreviewTexture(UTexture* InTexture);

	// ICustomSlateElement interface - UE 5.4 signature
	virtual void DrawRenderThread(FRHICommandListImmediate& RHICmdList, const void* RenderTarget) override;

private:
	FMaterializePreviewRenderTarget* RenderTarget;
	UTexture* PreviewTexture;
	bool bIsRealtime;
	FCriticalSection TextureLock;
};

/**
 * Viewport interface for Materialize node preview
 * Provides the bridge between Slate viewport widget and our custom rendering
 */
class FMaterializePreviewViewport : public ISlateViewport
{
public:
	FMaterializePreviewViewport(UMaterializeGraphNode* InNode);
	virtual ~FMaterializePreviewViewport();

	// ISlateViewport interface
	virtual FIntPoint GetSize() const override;
	virtual FSlateShaderResource* GetViewportRenderTargetTexture() const override;
	virtual bool RequiresVsync() const override { return false; }

	/**
	 * Updates the preview with new texture data
	 */
	void UpdatePreview(UTexture* InTexture);

	/**
	 * Sets realtime mode (animates time-based effects)
	 */
	void SetRealtime(bool bInRealtime) { bIsRealtime = bInRealtime; }
	bool IsRealtime() const { return bIsRealtime; }

private:
	TWeakObjectPtr<UMaterializeGraphNode> GraphNode;
	TSharedPtr<FMaterializePreviewElement> PreviewElement;
	bool bIsRealtime;
};
