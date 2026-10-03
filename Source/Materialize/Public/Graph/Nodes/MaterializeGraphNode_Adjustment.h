#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "KLayerStack.h" // For EKAdjustmentType and FKAdjustmentParams
#include "MaterializeGraphNode_Adjustment.generated.h"

/**
 * Base class for adjustment nodes that apply color and tone adjustments to textures
 */
UCLASS(Abstract)
class MATERIALIZE_API UMaterializeGraphNode_Adjustment : public UMaterializeGraphNode
{
	GENERATED_BODY()

public:
	/** Adjustment parameters */
	UPROPERTY(EditAnywhere, Category = "Adjustment")
	FKAdjustmentParams AdjustmentParams;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;
	//~ End UMaterializeGraphNode Interface
};

/**
 * Applies levels adjustment to input texture
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Levels : public UMaterializeGraphNode_Adjustment
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode_Levels();

	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface
};

/**
 * Applies curves adjustment to input texture
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Curves : public UMaterializeGraphNode_Adjustment
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode_Curves();

	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface
};

/**
 * Applies HSV adjustment to input texture
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_HSV : public UMaterializeGraphNode_Adjustment
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode_HSV();

	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface
};
