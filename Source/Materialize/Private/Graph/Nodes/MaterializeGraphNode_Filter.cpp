#include "Graph/Nodes/MaterializeGraphNode_Filter.h"
#include "Graph/MaterializeGraphSchema.h"
#include "KLayerEvaluator.h"
#include "Graph/MaterializeGraphRuntime.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "KSampleGraphNode_Filter"

// =============================================================================
// BASE FILTER NODE
// =============================================================================

void UMaterializeGraphNode_Filter::AllocateDefaultPins()
{
	// Single input texture
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("Input"));
	
	// Output
	CreatePin(EGPD_Output, UMaterializeGraphSchema::PC_Texture, TEXT("Result"));
}

FLinearColor UMaterializeGraphNode_Filter::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.6f, 0.8f); // Blue
}

UTexture2D* UMaterializeGraphNode_Filter::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Get input pin
	UEdGraphPin* InputPin = FindPin(TEXT("Input"), EGPD_Input);
	
	if (!InputPin)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Filter Node: Missing input pin"));
		Context.Errors.Add(TEXT("Filter node missing input pin"));
		return nullptr;
	}
	
	// Get input texture from connected node
	UTexture2D* InputTexture = GetInputTexture(InputPin, Context, Width, Height);
	
	// If input is null, return null
	if (!InputTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Filter Node: Input texture is null"));
		return nullptr;
	}
	
	// Apply filter using KLayerEvaluator
	FString Error;
	UTexture2D* Result = UKLayerEvaluator::ApplyFilter(InputTexture, FilterParams, Error);
	
	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Filter Node: Failed to apply filter - %s"), *Error);
		Context.Errors.Add(FString::Printf(TEXT("Filter node failed: %s"), *Error));
		return InputTexture; // Fall back to input texture
	}
	
	return Result;
}

// =============================================================================
// BLUR NODE
// =============================================================================

UMaterializeGraphNode_Blur::UMaterializeGraphNode_Blur()
{
	// Set default filter type to Gaussian Blur
	FilterParams.FilterType = EKFilterType::GaussianBlur;
	FilterParams.Intensity = 1.0f;
	FilterParams.KernelSize = 5;
}

FText UMaterializeGraphNode_Blur::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("BlurTitle", "Blur");
}

// =============================================================================
// SHARPEN NODE
// =============================================================================

UMaterializeGraphNode_Sharpen::UMaterializeGraphNode_Sharpen()
{
	// Set default filter type to Sharpen
	FilterParams.FilterType = EKFilterType::Sharpen;
	FilterParams.Intensity = 1.0f;
	FilterParams.KernelSize = 3;
}

FText UMaterializeGraphNode_Sharpen::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("SharpenTitle", "Sharpen");
}

// =============================================================================
// EDGE DETECT NODE
// =============================================================================

UMaterializeGraphNode_EdgeDetect::UMaterializeGraphNode_EdgeDetect()
{
	// Set default filter type to Edge Detect
	FilterParams.FilterType = EKFilterType::EdgeDetect;
	FilterParams.Intensity = 1.0f;
	FilterParams.KernelSize = 3;
}

FText UMaterializeGraphNode_EdgeDetect::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("EdgeDetectTitle", "Edge Detect");
}

#undef LOCTEXT_NAMESPACE
