#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "MaterializeGraphSchema.generated.h"

class UMaterializeGraphNode;

/** Action to add a node to the graph */
USTRUCT()
struct MATERIALIZE_API FMaterializeGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	// Template node to spawn
	UPROPERTY()
	TObjectPtr<class UMaterializeGraphNode> NodeTemplate;

	FMaterializeGraphSchemaAction_NewNode()
		: FEdGraphSchemaAction()
		, NodeTemplate(nullptr)
	{}

	FMaterializeGraphSchemaAction_NewNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
		, NodeTemplate(nullptr)
	{}

	//~ Begin FEdGraphSchemaAction Interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	//~ End FEdGraphSchemaAction Interface
};

UCLASS()
class MATERIALIZE_API UMaterializeGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	/** Pin Category Constants for wire coloring */
	static const FName PC_Texture;   // Texture2D outputs - Orange wire (Color/RGBA)
	static const FName PC_Scalar;    // Float/grayscale - Gray wire
	static const FName PC_Vector;    // Float3/Float4 color - Blue wire
	static const FName PC_Mask;      // Mask/alpha channel - Light gray wire
	static const FName PC_Normal;    // Normal map data - Cyan wire

	//~ Begin UEdGraphSchema Interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
	virtual int32 GetNodeSelectionCount(const UEdGraph* Graph) const override;
	virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const override;
	virtual class FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const override;
	//~ End UEdGraphSchema Interface
};

