#include "Graph/Nodes/MaterializeGraphNode_ChannelOutput.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Graph/MaterializeGraphRuntime.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "KSampleGraphNode_ChannelOutput"

UMaterializeGraphNode_ChannelOutput::UMaterializeGraphNode_ChannelOutput()
{
	// Output nodes don't show previews since they're terminal nodes
	bEnablePreview = false;
}

void UMaterializeGraphNode_ChannelOutput::AllocateDefaultPins()
{
	// Create a single typed input pin based on the output channel.
	// MUST use the schema's PC_* constants so CanCreateConnection validation passes.
	EMaterializeOutputPinType PinType = GetExpectedPinType();

	FName PinCategory;
	switch (PinType)
	{
		case EMaterializeOutputPinType::Color:
			PinCategory = UMaterializeGraphSchema::PC_Texture;
			break;
		case EMaterializeOutputPinType::Scalar:
			PinCategory = UMaterializeGraphSchema::PC_Scalar;
			break;
		case EMaterializeOutputPinType::Normal:
			PinCategory = UMaterializeGraphSchema::PC_Normal;
			break;
		default:
			PinCategory = UMaterializeGraphSchema::PC_Texture;
			break;
	}

	CreatePin(EGPD_Input, PinCategory, TEXT("Input"));
}

FText UMaterializeGraphNode_ChannelOutput::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::Format(
		LOCTEXT("TitleFormat", "Output: {0}"),
		GetChannelDisplayName()
	);
}

FLinearColor UMaterializeGraphNode_ChannelOutput::GetNodeTitleColor() const
{
	// Use different colors for different channel types
	switch (OutputChannel)
	{
		case EMaterializeOutputChannel::BaseColor:
			return FLinearColor(0.8f, 0.4f, 0.2f); // Orange
		case EMaterializeOutputChannel::Normal:
			return FLinearColor(0.4f, 0.4f, 0.8f); // Blue
		case EMaterializeOutputChannel::Roughness:
			return FLinearColor(0.6f, 0.6f, 0.6f); // Gray
		case EMaterializeOutputChannel::Metallic:
			return FLinearColor(0.7f, 0.7f, 0.8f); // Silver
		case EMaterializeOutputChannel::AO:
			return FLinearColor(0.3f, 0.3f, 0.3f); // Dark gray
		case EMaterializeOutputChannel::Height:
			return FLinearColor(0.5f, 0.3f, 0.1f); // Brown
		case EMaterializeOutputChannel::Emissive:
			return FLinearColor(1.0f, 0.9f, 0.2f); // Yellow
		default:
			return FLinearColor(1.0f, 0.3f, 0.3f); // Red (fallback)
	}
}

UTexture2D* UMaterializeGraphNode_ChannelOutput::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Output nodes extract textures from their input and store them in the context
	// They don't produce their own output texture
	
	// Find the input pin
	UEdGraphPin* InputPin = nullptr;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			InputPin = Pin;
			break;
		}
	}
	
	if (!InputPin || InputPin->LinkedTo.Num() == 0)
	{
		// No input connected - this is valid, just means this channel won't be generated
		UE_LOG(LogTemp, Warning, TEXT("Output node for channel '%s' has no input connected"), 
			*GetChannelName().ToString());
		return nullptr;
	}
	
	// Get the input texture from the connected node
	UTexture2D* InputTexture = GetInputTexture(InputPin, Context, Width, Height);
	
	if (!InputTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("Output node for channel '%s' failed to get input texture"), 
			*GetChannelName().ToString());
		return nullptr;
	}
	
	// Store the texture in the context's intermediate textures map using the channel name as key
	// The graph executor will extract these later
	FName ChannelName = GetChannelName();
	Context.IntermediateTextures.Add(NodeGuid, InputTexture);
	
	UE_LOG(LogTemp, Log, TEXT("Output node extracted texture for channel '%s': %dx%d"), 
		*ChannelName.ToString(), InputTexture->GetSizeX(), InputTexture->GetSizeY());
	
	// Return the input texture for preview purposes
	return InputTexture;
}

EMaterializeOutputPinType UMaterializeGraphNode_ChannelOutput::GetExpectedPinType() const
{
	switch (OutputChannel)
	{
		case EMaterializeOutputChannel::BaseColor:
			return EMaterializeOutputPinType::Color;
		case EMaterializeOutputChannel::Normal:
			return EMaterializeOutputPinType::Normal;
		case EMaterializeOutputChannel::Roughness:
		case EMaterializeOutputChannel::Metallic:
		case EMaterializeOutputChannel::AO:
		case EMaterializeOutputChannel::Height:
			return EMaterializeOutputPinType::Scalar;
		case EMaterializeOutputChannel::Emissive:
			return EMaterializeOutputPinType::Color;
		default:
			return EMaterializeOutputPinType::Color;
	}
}

FName UMaterializeGraphNode_ChannelOutput::GetChannelName() const
{
	switch (OutputChannel)
	{
		case EMaterializeOutputChannel::BaseColor:
			return FName(TEXT("BaseColor"));
		case EMaterializeOutputChannel::Normal:
			return FName(TEXT("Normal"));
		case EMaterializeOutputChannel::Roughness:
			return FName(TEXT("Roughness"));
		case EMaterializeOutputChannel::Metallic:
			return FName(TEXT("Metallic"));
		case EMaterializeOutputChannel::AO:
			return FName(TEXT("AO"));
		case EMaterializeOutputChannel::Height:
			return FName(TEXT("Height"));
		case EMaterializeOutputChannel::Emissive:
			return FName(TEXT("Emissive"));
		default:
			return FName(TEXT("Unknown"));
	}
}

FText UMaterializeGraphNode_ChannelOutput::GetChannelDisplayName() const
{
	switch (OutputChannel)
	{
		case EMaterializeOutputChannel::BaseColor:
			return LOCTEXT("BaseColor", "Base Color");
		case EMaterializeOutputChannel::Normal:
			return LOCTEXT("Normal", "Normal");
		case EMaterializeOutputChannel::Roughness:
			return LOCTEXT("Roughness", "Roughness");
		case EMaterializeOutputChannel::Metallic:
			return LOCTEXT("Metallic", "Metallic");
		case EMaterializeOutputChannel::AO:
			return LOCTEXT("AO", "Ambient Occlusion");
		case EMaterializeOutputChannel::Height:
			return LOCTEXT("Height", "Height");
		case EMaterializeOutputChannel::Emissive:
			return LOCTEXT("Emissive", "Emissive");
		default:
			return LOCTEXT("Unknown", "Unknown");
	}
}

bool UMaterializeGraphNode_ChannelOutput::IsPinTypeCompatible(const FEdGraphPinType& PinType) const
{
	EMaterializeOutputPinType ExpectedType = GetExpectedPinType();
	
	// Get the pin type category name
	FName PinCategory = PinType.PinCategory;
	
	switch (ExpectedType)
	{
		case EMaterializeOutputPinType::Color:
			// Color pins accept "color" or "texture" types
			return PinCategory == TEXT("color") || PinCategory == TEXT("texture");
			
		case EMaterializeOutputPinType::Scalar:
			// Scalar pins accept "scalar" or "float" types
			return PinCategory == TEXT("scalar") || PinCategory == TEXT("float");
			
		case EMaterializeOutputPinType::Normal:
			// Normal pins only accept "normal" types
			return PinCategory == TEXT("normal");
			
		default:
			return false;
	}
}

#undef LOCTEXT_NAMESPACE
