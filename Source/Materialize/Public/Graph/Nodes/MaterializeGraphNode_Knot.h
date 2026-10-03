// Copyright K-Studio. All Rights Reserved.
// Reroute/Knot Node for Materialize Graph

#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "MaterializeGraphNode_Knot.generated.h"

/**
 * Reroute node for clean cable management in Materialize graphs
 * Acts as a passthrough - doesn't generate any shader code
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Knot : public UMaterializeGraphNode
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode_Knot();

	// Pass-through execution: returns the connected input texture
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override;
	virtual bool CanDuplicateNode() const override { return true; }
	virtual bool CanUserDeleteNode() const override { return true; }
	//~ End UEdGraphNode Interface

	/** Knot nodes don't show preview - they're just reroutes */
	virtual bool ShouldShowPreview() const override { return false; }

	/** Get the input pin */
	UEdGraphPin* GetInputPin() const;

	/** Get the output pin */
	UEdGraphPin* GetOutputPin() const;

	/** Get the pass-through pin type from connected pins */
	FEdGraphPinType GetPassThroughPinType() const;

private:
	/** Update pin types based on connections */
	void UpdatePinTypes();
};
