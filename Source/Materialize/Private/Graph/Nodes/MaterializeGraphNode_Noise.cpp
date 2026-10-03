#include "Graph/Nodes/MaterializeGraphNode_Noise.h"
#include "Graph/MaterializeGraphSchema.h"
#include "KLayerEvaluator.h"
#include "Graph/MaterializeGraphRuntime.h"

#define LOCTEXT_NAMESPACE "KSampleGraphNode_Noise"

void UMaterializeGraphNode_Noise::AllocateDefaultPins()
{
	// Noise output — full texture (matches PC_Texture so schema allows connections)
	CreatePin(EGPD_Output, UMaterializeGraphSchema::PC_Texture, TEXT("Output"));
}

FText UMaterializeGraphNode_Noise::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	// Display the noise type in the title for clarity
	FString NoiseTypeName;
	switch (Params.NoiseType)
	{
		case EKProceduralNoiseType::Perlin:
			NoiseTypeName = TEXT("Perlin");
			break;
		case EKProceduralNoiseType::Worley:
			NoiseTypeName = TEXT("Voronoi");
			break;
		case EKProceduralNoiseType::FBM:
			NoiseTypeName = TEXT("FBM");
			break;
		case EKProceduralNoiseType::Simplex:
			NoiseTypeName = TEXT("Simplex");
			break;
		case EKProceduralNoiseType::Turbulence:
			NoiseTypeName = TEXT("Turbulence");
			break;
		case EKProceduralNoiseType::Cellular:
			NoiseTypeName = TEXT("Cellular");
			break;
		default:
			NoiseTypeName = TEXT("Noise");
			break;
	}
	
	return FText::FromString(FString::Printf(TEXT("%s Noise"), *NoiseTypeName));
}

FLinearColor UMaterializeGraphNode_Noise::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.5f, 0.8f); // Blue
}

UTexture2D* UMaterializeGraphNode_Noise::Execute(FMaterializeGraphContext& Context, int32 Width, int32 Height)
{
	// Generate procedural noise texture using the existing KLayerEvaluator backend
	FString Error;
	UTexture2D* Result = UKLayerEvaluator::GenerateProceduralTexture(Params, Width, Height, Error);
	
	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("Materialize Noise Node: Failed to generate procedural texture - %s"), *Error);
		return nullptr;
	}
	
	return Result;
}

#undef LOCTEXT_NAMESPACE
