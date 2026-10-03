#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "KLayerStack.h" // For EKFilterType and FKFilterParams
#include "MaterializeGraphNode_Filter.generated.h"

/**
 * Base class for filter nodes that apply image processing filters to textures
 */
UCLASS(Abstract)
class MATERIALIZE_API UMaterializeGraphNode_Filter : public UMaterializeGraphNode
{
	GENERATED_BODY()

public:
	/** Filter parameters */
	UPROPERTY(EditAnywhere, Category = "Filter")
	FKFilterParams FilterParams;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;
	//~ End UMaterializeGraphNode Interface
};

/**
 * Applies blur filter to input texture
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Blur : public UMaterializeGraphNode_Filter
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode_Blur();

	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface
};

/**
 * Applies sharpen filter to input texture
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Sharpen : public UMaterializeGraphNode_Filter
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode_Sharpen();

	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface
};

/**
 * Applies edge detection filter to input texture
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_EdgeDetect : public UMaterializeGraphNode_Filter
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode_EdgeDetect();

	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface
};
