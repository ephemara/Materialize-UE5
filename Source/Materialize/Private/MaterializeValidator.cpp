#include "MaterializeValidator.h"
#include "MaterializeErrorHandler.h"
#include "KLayerStack.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"

bool FMaterializeValidator::ValidateTexture(UTexture2D* Texture, FString& OutError)
{
	// Delegate to error handler for texture validation
	return FMaterializeErrorHandler::ValidateTexture(Texture, OutError);
}

bool FMaterializeValidator::ValidateRenderTarget(UTextureRenderTarget2D* RenderTarget, FString& OutError)
{
	// Delegate to error handler for render target validation
	return FMaterializeErrorHandler::ValidateRenderTarget(RenderTarget, OutError);
}

bool FMaterializeValidator::ValidateLayerStack(const FKLayerStack& LayerStack, FString& OutError)
{
	// Check for empty stack
	if (LayerStack.Layers.Num() == 0)
	{
		OutError = TEXT("Layer stack is empty");
		return false;
	}

	// TODO: Validate resolution - FKLayerStack structure needs to be checked
	// if (!ValidateDimensions(LayerStack.Resolution.X, LayerStack.Resolution.Y, OutError))
	// {
	// 	OutError = FString::Printf(TEXT("Layer stack has invalid resolution: %s"), *OutError);
	// 	return false;
	// }

	// Validate each layer
	for (int32 i = 0; i < LayerStack.Layers.Num(); ++i)
	{
		if (!ValidateLayer(LayerStack.Layers[i], i, OutError))
		{
			return false;
		}
	}

	return true;
}

bool FMaterializeValidator::ValidateLayer(const FKLayer& Layer, int32 LayerIndex, FString& OutError)
{
	// Validate opacity
	if (!ValidateOpacity(Layer.Opacity, OutError))
	{
		OutError = FString::Printf(TEXT("Layer %d has invalid opacity: %s"), LayerIndex, *OutError);
		return false;
	}

	// Validate blend mode
	if (!ValidateBlendMode(Layer.BlendMode, OutError))
	{
		OutError = FString::Printf(TEXT("Layer %d has invalid blend mode: %s"), LayerIndex, *OutError);
		return false;
	}

	// If it's an image layer, validate the source texture
	if (Layer.LayerType == EKLayerType::Image)
	{
		if (!Layer.ImageTexture)
		{
			OutError = FString::Printf(TEXT("Layer %d is an image layer but has no image texture"), LayerIndex);
			return false;
		}

		FString TextureError;
		if (!ValidateTexture(Layer.ImageTexture, TextureError))
		{
			OutError = FString::Printf(TEXT("Layer %d has invalid image texture: %s"), LayerIndex, *TextureError);
			return false;
		}
	}

	// Validate filter type if it's a filter layer
	if (Layer.LayerType == EKLayerType::Filter)
	{
		if (!ValidateFilterType(Layer.FilterParams.FilterType, OutError))
		{
			OutError = FString::Printf(TEXT("Layer %d has invalid filter type: %s"), LayerIndex, *OutError);
			return false;
		}
	}

	return true;
}

bool FMaterializeValidator::ValidateGraph(UMaterializeGraph* Graph, FString& OutError)
{
	if (!Graph)
	{
		OutError = TEXT("Graph is null");
		return false;
	}

	if (!Graph->IsValidLowLevel())
	{
		OutError = TEXT("Graph is not valid");
		return false;
	}

	// Check for nodes (use base class Nodes array)
	if (Graph->Nodes.Num() == 0)
	{
		OutError = TEXT("Graph has no nodes");
		return false;
	}

	// Check for required output nodes
	TSet<FName> RequiredOutputs = {TEXT("BaseColor"), TEXT("Normal"), TEXT("Roughness")};
	TSet<FName> FoundOutputs;

	for (UEdGraphNode* EdNode : Graph->Nodes)
	{
		UMaterializeGraphNode* Node = Cast<UMaterializeGraphNode>(EdNode);
		if (!Node)
		{
			continue;
		}

		// Check if it's an output node
		if (Node->GetClass()->GetName().Contains(TEXT("Output")))
		{
			// Try to get the output channel name from the node
			// This is a simplified check - actual implementation may vary
			FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
			if (NodeTitle.Contains(TEXT("BaseColor")))
			{
				FoundOutputs.Add(TEXT("BaseColor"));
			}
			else if (NodeTitle.Contains(TEXT("Normal")))
			{
				FoundOutputs.Add(TEXT("Normal"));
			}
			else if (NodeTitle.Contains(TEXT("Roughness")))
			{
				FoundOutputs.Add(TEXT("Roughness"));
			}
		}
	}

	// Check if all required outputs are present
	for (const FName& Required : RequiredOutputs)
	{
		if (!FoundOutputs.Contains(Required))
		{
			OutError = FString::Printf(TEXT("Graph is missing required output: %s"), *Required.ToString());
			return false;
		}
	}

	return true;
}

bool FMaterializeValidator::ValidateBlendMode(EKLayerBlendMode BlendMode, FString& OutError)
{
	// Check if blend mode is within valid range
	// EKLayerBlendMode is an enum, so we check against the enum values
	int32 BlendModeValue = static_cast<int32>(BlendMode);
	
	// Valid blend modes range from 0 to the last defined blend mode
	// This assumes the enum is properly defined with sequential values
	// Note: We can't use EKLayerBlendMode::MAX directly, so we use a large number
	if (BlendModeValue < 0 || BlendModeValue >= 100)
	{
		OutError = FString::Printf(TEXT("Blend mode value %d is out of valid range"), BlendModeValue);
		return false;
	}

	return true;
}

bool FMaterializeValidator::ValidateFilterType(EKFilterType FilterType, FString& OutError)
{
	// Check if filter type is within valid range
	int32 FilterTypeValue = static_cast<int32>(FilterType);
	
	// Valid filter types range from 0 to the last defined filter type
	// Note: We can't use EKFilterType::MAX directly, so we use a large number
	if (FilterTypeValue < 0 || FilterTypeValue >= 100)
	{
		OutError = FString::Printf(TEXT("Filter type value %d is out of valid range"), FilterTypeValue);
		return false;
	}

	return true;
}

bool FMaterializeValidator::ValidateTextureFormat(UTexture2D* Texture, FString& OutError)
{
	if (!Texture)
	{
		OutError = TEXT("Texture is null");
		return false;
	}

	// Check if texture has valid pixel format
	EPixelFormat PixelFormat = Texture->GetPixelFormat();
	
	// Check for supported formats (RGBA8, BGRA8, FloatRGBA, etc.)
	bool bIsSupportedFormat = 
		PixelFormat == PF_B8G8R8A8 ||
		PixelFormat == PF_R8G8B8A8 ||
		PixelFormat == PF_FloatRGBA ||
		PixelFormat == PF_A32B32G32R32F ||
		PixelFormat == PF_R16G16B16A16_UNORM ||
		PixelFormat == PF_R16G16B16A16_SNORM;

	if (!bIsSupportedFormat)
	{
		OutError = FString::Printf(TEXT("Texture '%s' has unsupported format: %d"), 
			*Texture->GetName(), static_cast<int32>(PixelFormat));
		return false;
	}

	return true;
}

bool FMaterializeValidator::ValidateDimensions(int32 Width, int32 Height, FString& OutError)
{
	// Delegate to error handler for dimension validation
	return FMaterializeErrorHandler::ValidateDimensions(Width, Height, OutError);
}

bool FMaterializeValidator::ValidateCompatibleDimensions(UTexture2D* Texture1, UTexture2D* Texture2, FString& OutError)
{
	// Delegate to error handler for compatible dimensions validation
	return FMaterializeErrorHandler::ValidateCompatibleDimensions(Texture1, Texture2, OutError);
}

bool FMaterializeValidator::ValidateRange(float Value, float Min, float Max, const FString& ParamName, FString& OutError)
{
	// Delegate to error handler for range validation
	return FMaterializeErrorHandler::ValidateRange(Value, Min, Max, ParamName, OutError);
}

bool FMaterializeValidator::ValidateRange(int32 Value, int32 Min, int32 Max, const FString& ParamName, FString& OutError)
{
	// Delegate to error handler for range validation
	return FMaterializeErrorHandler::ValidateRange(Value, Min, Max, ParamName, OutError);
}

bool FMaterializeValidator::ValidateOpacity(float Opacity, FString& OutError)
{
	return ValidateRange(Opacity, 0.0f, 1.0f, TEXT("Opacity"), OutError);
}
