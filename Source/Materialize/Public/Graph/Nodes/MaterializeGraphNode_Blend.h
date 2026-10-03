#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "KLayerStack.h" // For EKLayerBlendMode
#include "MaterializeGraphNode_Blend.generated.h"

/**
 * Blends two input textures using a specified blend mode
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Blend : public UMaterializeGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Blend")
	EKLayerBlendMode BlendMode = EKLayerBlendMode::Normal;

	UPROPERTY(EditAnywhere, Category = "Blend", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 1.0f;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;
	//~ End UMaterializeGraphNode Interface
};
