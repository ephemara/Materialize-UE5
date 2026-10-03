#include "Graph/Nodes/MaterializeGraphNode_Math.h"
#include "Graph/MaterializeGraphSchema.h"
#include "KLayerEvaluator.h"
#include "Graph/MaterializeGraphRuntime.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "KSampleGraphNode_Math"

// =============================================================================
// BASE MATH NODE
// =============================================================================

FLinearColor UMaterializeGraphNode_Math::GetNodeTitleColor() const
{
	return FLinearColor(0.6f, 0.8f, 0.2f); // Yellow-green
}

// =============================================================================
// ADD NODE
// =============================================================================

void UMaterializeGraphNode_Add::AllocateDefaultPins()
{
	// Two input textures
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("A"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("B"));
	
	// Output
	CreatePin(EGPD_Output, UMaterializeGraphSchema::PC_Texture, TEXT("Result"));
}

FText UMaterializeGraphNode_Add::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AddTitle", "Add");
}

UTexture2D* UMaterializeGraphNode_Add::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Get input pins
	UEdGraphPin* PinA = FindPin(TEXT("A"), EGPD_Input);
	UEdGraphPin* PinB = FindPin(TEXT("B"), EGPD_Input);
	
	if (!PinA || !PinB)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Add Node: Missing input pins"));
		Context.Errors.Add(TEXT("Add node missing input pins"));
		return nullptr;
	}
	
	// Get input textures from connected nodes
	UTexture2D* TextureA = GetInputTexture(PinA, Context, Width, Height);
	UTexture2D* TextureB = GetInputTexture(PinB, Context, Width, Height);
	
	// If either input is null, return the other (or null)
	if (!TextureA)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Add Node: Texture A is null, returning texture B"));
		return TextureB;
	}
	
	if (!TextureB)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Add Node: Texture B is null, returning texture A"));
		return TextureA;
	}
	
	// Add the textures using KLayerEvaluator
	FString Error;
	UTexture2D* Result = UKLayerEvaluator::AddTextures(TextureA, TextureB, Error);
	
	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Add Node: Failed to add textures - %s"), *Error);
		Context.Errors.Add(FString::Printf(TEXT("Add node failed: %s"), *Error));
		return TextureA; // Fall back to first texture
	}
	
	return Result;
}

// =============================================================================
// MULTIPLY NODE
// =============================================================================

void UMaterializeGraphNode_Multiply::AllocateDefaultPins()
{
	// Two input textures
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("A"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("B"));
	
	// Output
	CreatePin(EGPD_Output, UMaterializeGraphSchema::PC_Texture, TEXT("Result"));
}

FText UMaterializeGraphNode_Multiply::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("MultiplyTitle", "Multiply");
}

UTexture2D* UMaterializeGraphNode_Multiply::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Get input pins
	UEdGraphPin* PinA = FindPin(TEXT("A"), EGPD_Input);
	UEdGraphPin* PinB = FindPin(TEXT("B"), EGPD_Input);
	
	if (!PinA || !PinB)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Multiply Node: Missing input pins"));
		Context.Errors.Add(TEXT("Multiply node missing input pins"));
		return nullptr;
	}
	
	// Get input textures from connected nodes
	UTexture2D* TextureA = GetInputTexture(PinA, Context, Width, Height);
	UTexture2D* TextureB = GetInputTexture(PinB, Context, Width, Height);
	
	// If either input is null, return the other (or null)
	if (!TextureA)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Multiply Node: Texture A is null, returning texture B"));
		return TextureB;
	}
	
	if (!TextureB)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Multiply Node: Texture B is null, returning texture A"));
		return TextureA;
	}
	
	// Multiply the textures using KLayerEvaluator
	FString Error;
	UTexture2D* Result = UKLayerEvaluator::MultiplyTextures(TextureA, TextureB, Error);
	
	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Multiply Node: Failed to multiply textures - %s"), *Error);
		Context.Errors.Add(FString::Printf(TEXT("Multiply node failed: %s"), *Error));
		return TextureA; // Fall back to first texture
	}
	
	return Result;
}

// =============================================================================
// LERP NODE
// =============================================================================

void UMaterializeGraphNode_Lerp::AllocateDefaultPins()
{
	// Two input textures
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("A"));
	CreatePin(EGPD_Input, UMaterializeGraphSchema::PC_Texture, TEXT("B"));
	
	// Output
	CreatePin(EGPD_Output, UMaterializeGraphSchema::PC_Texture, TEXT("Result"));
}

FText UMaterializeGraphNode_Lerp::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("LerpTitle", "Lerp");
}

UTexture2D* UMaterializeGraphNode_Lerp::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Get input pins
	UEdGraphPin* PinA = FindPin(TEXT("A"), EGPD_Input);
	UEdGraphPin* PinB = FindPin(TEXT("B"), EGPD_Input);
	
	if (!PinA || !PinB)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Lerp Node: Missing input pins"));
		Context.Errors.Add(TEXT("Lerp node missing input pins"));
		return nullptr;
	}
	
	// Get input textures from connected nodes
	UTexture2D* TextureA = GetInputTexture(PinA, Context, Width, Height);
	UTexture2D* TextureB = GetInputTexture(PinB, Context, Width, Height);
	
	// If either input is null, return the other (or null)
	if (!TextureA)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Lerp Node: Texture A is null, returning texture B"));
		return TextureB;
	}
	
	if (!TextureB)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Lerp Node: Texture B is null, returning texture A"));
		return TextureA;
	}
	
	// Lerp the textures using KLayerEvaluator
	FString Error;
	UTexture2D* Result = UKLayerEvaluator::LerpTextures(TextureA, TextureB, Alpha, Error);
	
	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Lerp Node: Failed to lerp textures - %s"), *Error);
		Context.Errors.Add(FString::Printf(TEXT("Lerp node failed: %s"), *Error));
		return TextureA; // Fall back to first texture
	}
	
	return Result;
}

#undef LOCTEXT_NAMESPACE
