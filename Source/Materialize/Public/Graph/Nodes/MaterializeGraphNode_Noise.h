#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "KLayerStack.h" // For FKProceduralParams
#include "MaterializeGraphNode_Noise.generated.h"

UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Noise : public UMaterializeGraphNode
{
	GENERATED_BODY()

public:
	// Noise configuration
	UPROPERTY(EditAnywhere, Category = "Noise", meta = (ShowOnlyInnerProperties))
	FKProceduralParams Params;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;
	//~ End UMaterializeGraphNode Interface
};
