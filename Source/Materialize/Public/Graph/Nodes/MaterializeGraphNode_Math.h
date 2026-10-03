#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "MaterializeGraphNode_Math.generated.h"

/**
 * Base class for math operation nodes that perform per-pixel mathematical operations on textures
 */
UCLASS(Abstract)
class MATERIALIZE_API UMaterializeGraphNode_Math : public UMaterializeGraphNode
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode Interface
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End UEdGraphNode Interface
};

/**
 * Adds two textures together (A + B)
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Add : public UMaterializeGraphNode_Math
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;
	//~ End UMaterializeGraphNode Interface
};

/**
 * Multiplies two textures together (A * B)
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Multiply : public UMaterializeGraphNode_Math
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;
	//~ End UMaterializeGraphNode Interface
};

/**
 * Linear interpolation between two textures (Lerp(A, B, Alpha))
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Lerp : public UMaterializeGraphNode_Math
{
	GENERATED_BODY()

public:
	/** Alpha value for interpolation (0 = A, 1 = B) */
	UPROPERTY(EditAnywhere, Category = "Math", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Alpha = 0.5f;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;
	//~ End UMaterializeGraphNode Interface
};
