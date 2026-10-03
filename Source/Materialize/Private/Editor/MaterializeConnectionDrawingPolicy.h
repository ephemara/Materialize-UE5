// Copyright K-Studio. All Rights Reserved.
// Custom Connection Drawing Policy for Materialize Graph

#pragma once

#include "ConnectionDrawingPolicy.h"
#include "Containers/Map.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Math/Vector2D.h"

class FSlateRect;
class FSlateWindowElementList;
class UEdGraph;
class UEdGraphPin;
class UMaterializeGraph;
class UMaterializeGraphSchema;
class UMaterializeGraphNode_Knot;

/**
 * Custom connection drawing policy for Materialize graphs
 * Implements type-based wire colors, hover deemphasis, and custom tooltips
 */
class FMaterializeConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
protected:
	UMaterializeGraph* SampleGraph;
	const UMaterializeGraphSchema* SampleGraphSchema;

	// Track knot node wire directions
	TMap<UMaterializeGraphNode_Knot*, bool> KnotToReversedDirectionMap;

	/** Determine if knot tangent should be reversed based on connected nodes */
	bool ShouldChangeTangentForKnot(UMaterializeGraphNode_Knot* Knot);
	
	/** Get average position of connected pins for a knot in given direction */
	bool GetAverageConnectedPosition(UMaterializeGraphNode_Knot* Knot, EEdGraphPinDirection Direction, FVector2D& OutPos) const;
	
	/** Find the center point of a pin widget */
	bool FindPinCenter(UEdGraphPin* Pin, FVector2D& OutCenter) const;

public:
	FMaterializeConnectionDrawingPolicy(
		int32 InBackLayerID, 
		int32 InFrontLayerID, 
		float ZoomFactor, 
		const FSlateRect& InClippingRect, 
		FSlateWindowElementList& InDrawElements, 
		UEdGraph* InGraphObj);

	// FConnectionDrawingPolicy interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override;
	virtual TSharedPtr<IToolTip> GetConnectionToolTip(const SGraphPanel& GraphPanel, const FGraphSplineOverlapResult& OverlapData) const override;

private:
	/** Get formatted node/pin info for tooltip */
	FText GetNodePinInfo(const TSharedPtr<SGraphPin>& PinWidget) const;

	/** Wire color constants */
	static constexpr FLinearColor WireColorTexture = FLinearColor(0.9f, 0.5f, 0.1f, 1.0f);   // Orange
	static constexpr FLinearColor WireColorScalar = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);    // Gray
	static constexpr FLinearColor WireColorVector = FLinearColor(0.2f, 0.4f, 0.9f, 1.0f);    // Blue
	static constexpr FLinearColor WireColorMask = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);      // Light Gray
	static constexpr FLinearColor WireColorExec = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);      // White
	static constexpr FLinearColor WireColorInactive = FLinearColor(0.3f, 0.3f, 0.3f, 0.5f);  // Dim
};

/**
 * Factory to create Materialize connection drawing policy
 */
struct FMaterializePinConnectionFactory : public FGraphPanelPinConnectionFactory
{
public:
	virtual FConnectionDrawingPolicy* CreateConnectionPolicy(
		const UEdGraphSchema* Schema, 
		int32 InBackLayerID, 
		int32 InFrontLayerID, 
		float ZoomFactor, 
		const FSlateRect& InClippingRect, 
		FSlateWindowElementList& InDrawElements, 
		UEdGraph* InGraphObj) const override;
};
