#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "Graph/MaterializeGraph.h"
#include "MaterializeGraphNode.generated.h"

class UTexture2D;
class FMaterializeGraphContext;

/**
 * Base parameters structure for graph nodes
 * Derived node types can extend this with specific parameters
 */
USTRUCT(BlueprintType)
struct MATERIALIZE_API FMaterializeGraphNodeParams
{
	GENERATED_BODY()

	/** Generic intensity/strength parameter used by many node types */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Intensity = 1.0f;

	/** Generic scale parameter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (ClampMin = "0.01", ClampMax = "100.0"))
	float Scale = 1.0f;

	FMaterializeGraphNodeParams() = default;
};

/**
 * Abstract base class for all Materialize graph nodes
 * Provides common functionality for node execution, UI display, and preview management
 * 
 * Derived classes must implement:
 * - Execute(): Core node logic that processes inputs and produces output
 * - AllocateDefaultPins(): Create input/output pins for the node
 * - GetNodeTitle(): Return the display name for the node
 * - GetNodeTitleColor(): Return the color for the node header
 */
UCLASS(Abstract)
class MATERIALIZE_API UMaterializeGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode();

	/** Base parameters - derived classes can add their own parameter structures */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	FMaterializeGraphNodeParams Parameters;

	/** Enable automated preview for this node */
	UPROPERTY(EditAnywhere, Category = "Preview")
	bool bEnablePreview = true;

	/** Whether the preview is currently collapsed */
	UPROPERTY()
	bool bPreviewCollapsed = false;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanDuplicateNode() const override { return true; }
	virtual bool CanUserDeleteNode() const override { return true; }
	//~ End UEdGraphNode Interface

	/**
	 * Execute this node's logic
	 * Pure virtual - must be implemented by derived classes
	 * 
	 * @param Context - Execution context containing intermediate textures and state
	 * @param Width - Output texture width
	 * @param Height - Output texture height
	 * @return The output texture produced by this node, or nullptr on failure
	 */
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) PURE_VIRTUAL(UMaterializeGraphNode::Execute, return nullptr;);

	/** Get the Materialize Graph that owns us */
	UMaterializeGraph* GetKSampleGraph() const;

	/** Get the cached preview texture for this node's output */
	UFUNCTION()
	UTexture* GetPreviewTexture() const { return PreviewTexture; }

	/** Set the preview texture (called after node execution) */
	void SetPreviewTexture(UTexture* InTexture) { PreviewTexture = InTexture; }

	/** Returns true if this node generates visible output that can be previewed */
	virtual bool ShouldShowPreview() const { return bEnablePreview && !bPreviewCollapsed; }

	/** Toggle the collapsed state of the preview */
	void TogglePreviewCollapsed() { bPreviewCollapsed = !bPreviewCollapsed; }

	/** Delegate called when preview needs to be invalidated */
	DECLARE_DELEGATE(FOnPreviewInvalidated);
	FOnPreviewInvalidated OnPreviewInvalidated;

	/** Invalidate the preview and request re-execution */
	void InvalidatePreview();

	/** Get the unique node GUID, generating one if needed */
	FGuid GetNodeGuid() const;

protected:
	/** Cached preview texture - transient, not saved */
	UPROPERTY(Transient)
	TObjectPtr<UTexture> PreviewTexture;

	/** Helper to get input texture from a connected pin */
	UTexture2D* GetInputTexture(UEdGraphPin* Pin, FMaterializeGraphContext& Context, int32 Width, int32 Height);
};

