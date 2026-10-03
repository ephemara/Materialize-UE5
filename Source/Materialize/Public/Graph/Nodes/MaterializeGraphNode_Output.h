#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "MaterializeGraphNode_Output.generated.h"

class UTexture2D;

/**
 * Final output node - the endpoint of the graph
 * Collects PBR channels and provides bake functionality
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_Output : public UMaterializeGraphNode
{
	GENERATED_BODY()

public:
	/** Output texture resolution width */
	UPROPERTY(EditAnywhere, Category = "Output", meta = (ClampMin = "64", ClampMax = "8192"))
	int32 Width = 1024;

	/** Output texture resolution height */
	UPROPERTY(EditAnywhere, Category = "Output", meta = (ClampMin = "64", ClampMax = "8192"))
	int32 Height = 1024;

	/** Base path for saving baked textures */
	UPROPERTY(EditAnywhere, Category = "Output")
	FString OutputPath = TEXT("/Game/Textures/Materialize");

	/** Base name for baked textures */
	UPROPERTY(EditAnywhere, Category = "Output")  
	FString OutputName = TEXT("Generated");

	/** Cached baked textures */
	UPROPERTY(Transient)
	UTexture2D* BakedBaseColor = nullptr;

	UPROPERTY(Transient)
	UTexture2D* BakedNormal = nullptr;

	UPROPERTY(Transient)
	UTexture2D* BakedRoughness = nullptr;

	UPROPERTY(Transient)
	UTexture2D* BakedMetallic = nullptr;

	UPROPERTY(Transient)
	UTexture2D* BakedHeight = nullptr;

	UPROPERTY(Transient)
	UTexture2D* BakedAO = nullptr;

	UPROPERTY(Transient)
	UTexture2D* BakedEmissive = nullptr;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 InWidth, int32 InHeight) override;
	//~ End UMaterializeGraphNode Interface

	/** Bake all connected channels to texture assets */
	UFUNCTION(CallInEditor, Category = "Output")
	void BakeToTextures();

	/** Get the input pin for a specific channel */
	UEdGraphPin* GetChannelPin(FName ChannelName) const;
};
