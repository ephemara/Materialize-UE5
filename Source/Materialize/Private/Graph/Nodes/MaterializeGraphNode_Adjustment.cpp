#include "Graph/Nodes/MaterializeGraphNode_Adjustment.h"
#include "Graph/MaterializeGraphSchema.h"
#include "KLayerEvaluator.h"
#include "Graph/MaterializeGraphRuntime.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "KSampleGraphNode_Adjustment"

// =============================================================================
// BASE ADJUSTMENT NODE
// =============================================================================

void UMaterializeGraphNode_Adjustment::AllocateDefaultPins()
{
	// Single input texture
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Input"));
	
	// Output
	CreatePin(EGPD_Output, UMaterializeGraphSchema::PC_Texture, TEXT("Result"));
}

FLinearColor UMaterializeGraphNode_Adjustment::GetNodeTitleColor() const
{
	return FLinearColor(0.8f, 0.4f, 0.2f); // Orange
}

UTexture2D* UMaterializeGraphNode_Adjustment::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Get input pin
	UEdGraphPin* InputPin = FindPin(TEXT("Input"), EGPD_Input);
	
	if (!InputPin)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Adjustment Node: Missing input pin"));
		Context.Errors.Add(TEXT("Adjustment node missing input pin"));
		return nullptr;
	}
	
	// Get input texture from connected node
	UTexture2D* InputTexture = GetInputTexture(InputPin, Context, Width, Height);
	
	// If input is null, return null
	if (!InputTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Adjustment Node: Input texture is null"));
		return nullptr;
	}
	
	// Apply adjustment using KLayerEvaluator
	FString Error;
	UTexture2D* Result = UKLayerEvaluator::ApplyAdjustment(InputTexture, AdjustmentParams, Error);
	
	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Adjustment Node: Failed to apply adjustment - %s"), *Error);
		Context.Errors.Add(FString::Printf(TEXT("Adjustment node failed: %s"), *Error));
		return InputTexture; // Fall back to input texture
	}
	
	return Result;
}

// =============================================================================
// LEVELS NODE
// =============================================================================

UMaterializeGraphNode_Levels::UMaterializeGraphNode_Levels()
{
	// Set default adjustment type to Levels
	AdjustmentParams.AdjustmentType = EKAdjustmentType::Levels;
	AdjustmentParams.InputBlack = 0.0f;
	AdjustmentParams.InputWhite = 1.0f;
	AdjustmentParams.Gamma = 1.0f;
	AdjustmentParams.OutputBlack = 0.0f;
	AdjustmentParams.OutputWhite = 1.0f;
}

FText UMaterializeGraphNode_Levels::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("LevelsTitle", "Levels");
}

// =============================================================================
// CURVES NODE
// =============================================================================

UMaterializeGraphNode_Curves::UMaterializeGraphNode_Curves()
{
	// Set default adjustment type to Curves
	AdjustmentParams.AdjustmentType = EKAdjustmentType::Curves;
	AdjustmentParams.Brightness = 0.0f;
	AdjustmentParams.Contrast = 0.0f;
}

FText UMaterializeGraphNode_Curves::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("CurvesTitle", "Curves");
}

// =============================================================================
// HSV NODE
// =============================================================================

UMaterializeGraphNode_HSV::UMaterializeGraphNode_HSV()
{
	// Set default adjustment type to HSV
	AdjustmentParams.AdjustmentType = EKAdjustmentType::HSV;
	AdjustmentParams.HueShift = 0.0f;
	AdjustmentParams.SaturationAdjust = 0.0f;
	AdjustmentParams.ValueAdjust = 0.0f;
}

FText UMaterializeGraphNode_HSV::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("HSVTitle", "HSV");
}

#undef LOCTEXT_NAMESPACE
