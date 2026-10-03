#pragma once

#include "CoreMinimal.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "MaterializeGraphNode_ChannelOutput.generated.h"

/**
 * Output channel types for PBR material generation
 */
UENUM(BlueprintType)
enum class EMaterializeOutputChannel : uint8
{
	BaseColor UMETA(DisplayName = "Base Color"),
	Normal UMETA(DisplayName = "Normal"),
	Roughness UMETA(DisplayName = "Roughness"),
	Metallic UMETA(DisplayName = "Metallic"),
	AO UMETA(DisplayName = "Ambient Occlusion"),
	Height UMETA(DisplayName = "Height"),
	Emissive UMETA(DisplayName = "Emissive")
};

/**
 * Pin type for output channel inputs
 */
UENUM(BlueprintType)
enum class EMaterializeOutputPinType : uint8
{
	Color UMETA(DisplayName = "Color (RGBA)"),
	Scalar UMETA(DisplayName = "Scalar (Grayscale)"),
	Normal UMETA(DisplayName = "Normal (RGB)")
};

/**
 * Individual output node for a specific PBR channel
 * Multiple instances can exist in a graph, each configured for different channels
 * 
 * This is a terminal node that extracts textures to the final PBR output map
 * 
 * **Validates: Requirements 4.3**
 */
UCLASS()
class MATERIALIZE_API UMaterializeGraphNode_ChannelOutput : public UMaterializeGraphNode
{
	GENERATED_BODY()

public:
	UMaterializeGraphNode_ChannelOutput();

	/** Which PBR channel this output node represents */
	UPROPERTY(EditAnywhere, Category = "Output")
	EMaterializeOutputChannel OutputChannel = EMaterializeOutputChannel::BaseColor;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual bool CanDuplicateNode() const override { return false; } // Prevent duplicate outputs for same channel
	//~ End UEdGraphNode Interface

	//~ Begin UMaterializeGraphNode Interface
	virtual UTexture2D* Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height) override;
	//~ End UMaterializeGraphNode Interface

	/** Get the expected pin type for this output channel */
	EMaterializeOutputPinType GetExpectedPinType() const;

	/** Get the channel name as FName for use in output maps */
	FName GetChannelName() const;

	/** Get a user-friendly display name for the channel */
	FText GetChannelDisplayName() const;

private:
	/** Helper to determine if a pin type is compatible with this channel */
	bool IsPinTypeCompatible(const FEdGraphPinType& PinType) const;
};
