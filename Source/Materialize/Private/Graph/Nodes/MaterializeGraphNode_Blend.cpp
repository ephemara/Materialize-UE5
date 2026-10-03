#include "Graph/Nodes/MaterializeGraphNode_Blend.h"
#include "Graph/MaterializeGraphSchema.h"
#include "KLayerEvaluator.h"
#include "Graph/MaterializeGraphRuntime.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "KSampleGraphNode_Blend"

void UMaterializeGraphNode_Blend::AllocateDefaultPins()
{
	// Two input textures
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Base"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Blend"));
	
	// Output
	CreatePin(EGPD_Output, UMaterializeGraphSchema::PC_Texture, TEXT("Result"));
}

FText UMaterializeGraphNode_Blend::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Title", "Blend");
}

FLinearColor UMaterializeGraphNode_Blend::GetNodeTitleColor() const
{
	return FLinearColor(0.8f, 0.4f, 0.2f); // Orange
}

UTexture2D* UMaterializeGraphNode_Blend::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Get input pins
	UEdGraphPin* BasePin = FindPin(TEXT("Base"), EGPD_Input);
	UEdGraphPin* BlendPin = FindPin(TEXT("Blend"), EGPD_Input);
	
	if (!BasePin || !BlendPin)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Blend Node: Missing input pins"));
		Context.Errors.Add(TEXT("Blend node missing input pins"));
		return nullptr;
	}
	
	// Get input textures from connected nodes
	UTexture2D* BaseTexture = GetInputTexture(BasePin, Context, Width, Height);
	UTexture2D* BlendTexture = GetInputTexture(BlendPin, Context, Width, Height);
	
	// If base is null, return blend texture (or null)
	if (!BaseTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Blend Node: Base texture is null, returning blend texture"));
		return BlendTexture;
	}
	
	// If blend is null, return base texture
	if (!BlendTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Blend Node: Blend texture is null, returning base texture"));
		return BaseTexture;
	}
	
	// Blend the textures using KLayerEvaluator
	FString Error;
	UTexture2D* Result = UKLayerEvaluator::BlendTextures(
		BaseTexture,
		BlendTexture,
		BlendMode,
		Opacity,
		nullptr,  // No mask
		false,    // Don't invert mask
		Error
	);
	
	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Blend Node: Failed to blend textures - %s"), *Error);
		Context.Errors.Add(FString::Printf(TEXT("Blend node failed: %s"), *Error));
		return BaseTexture; // Fall back to base texture
	}
	
	return Result;
}

#undef LOCTEXT_NAMESPACE
