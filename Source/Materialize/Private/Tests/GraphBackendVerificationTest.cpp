// Copyright K-Studio. All Rights Reserved.
// Graph Backend Verification Test - Checkpoint Task 9

#include "Misc/AutomationTest.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/MaterializeGraphExecutor.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Graph/Nodes/MaterializeGraphNode_Noise.h"
#include "Graph/Nodes/MaterializeGraphNode_ChannelOutput.h"
#include "Engine/Texture2D.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphBackendVerificationTest,
	"Materialize.Graph.BackendVerification",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphBackendVerificationTest::RunTest(const FString& Parameters)
{
	UE_LOG(LogTemp, Log, TEXT("=== Materialize Graph Backend Verification Test ==="));

	// Step 1: Create a simple test graph programmatically
	UE_LOG(LogTemp, Log, TEXT("Step 1: Creating test graph..."));
	
	UMaterializeGraph* TestGraph = NewObject<UMaterializeGraph>();
	if (!TestGraph)
	{
		AddError(TEXT("Failed to create test graph"));
		return false;
	}

	// Initialize the graph with schema
	TestGraph->Schema = UMaterializeGraphSchema::StaticClass();
	UE_LOG(LogTemp, Log, TEXT("  Graph created successfully"));

	// Step 2: Add a Perlin noise node
	UE_LOG(LogTemp, Log, TEXT("Step 2: Adding Perlin noise node..."));
	
	UMaterializeGraphNode_Noise* NoiseNode = Cast<UMaterializeGraphNode_Noise>(
		TestGraph->AddNode(UMaterializeGraphNode_Noise::StaticClass(), FVector2D(0, 0))
	);
	
	if (!NoiseNode)
	{
		AddError(TEXT("Failed to create noise node"));
		return false;
	}

	// Configure noise parameters
	NoiseNode->Params.NoiseType = EKProceduralNoiseType::Perlin;
	NoiseNode->Params.Scale = 5.0f;
	NoiseNode->Params.Octaves = 4;
	NoiseNode->Params.Persistence = 0.5f;
	NoiseNode->Params.Lacunarity = 2.0f;
	NoiseNode->Params.Seed = 42;
	
	UE_LOG(LogTemp, Log, TEXT("  Noise node created: Type=%d, Scale=%.2f, Octaves=%d"),
		(int32)NoiseNode->Params.NoiseType, NoiseNode->Params.Scale, NoiseNode->Params.Octaves);

	// Step 3: Add a BaseColor output node
	UE_LOG(LogTemp, Log, TEXT("Step 3: Adding BaseColor output node..."));
	
	UMaterializeGraphNode_ChannelOutput* OutputNode = Cast<UMaterializeGraphNode_ChannelOutput>(
		TestGraph->AddNode(UMaterializeGraphNode_ChannelOutput::StaticClass(), FVector2D(300, 0))
	);
	
	if (!OutputNode)
	{
		AddError(TEXT("Failed to create output node"));
		return false;
	}

	// Configure output channel
	OutputNode->OutputChannel = EMaterializeOutputChannel::BaseColor;
	UE_LOG(LogTemp, Log, TEXT("  Output node created: Channel=BaseColor"));

	// Step 4: Connect noise node to output node
	UE_LOG(LogTemp, Log, TEXT("Step 4: Connecting nodes..."));
	
	// Find the output pin on the noise node
	UEdGraphPin* NoiseOutputPin = nullptr;
	for (UEdGraphPin* Pin : NoiseNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			NoiseOutputPin = Pin;
			break;
		}
	}

	// Find the input pin on the output node
	UEdGraphPin* OutputInputPin = nullptr;
	for (UEdGraphPin* Pin : OutputNode->Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			OutputInputPin = Pin;
			break;
		}
	}

	if (!NoiseOutputPin || !OutputInputPin)
	{
		AddError(TEXT("Failed to find pins for connection"));
		return false;
	}

	// Make the connection
	NoiseOutputPin->MakeLinkTo(OutputInputPin);
	UE_LOG(LogTemp, Log, TEXT("  Nodes connected successfully"));

	// Step 5: Execute the graph and verify shader dispatches occur
	UE_LOG(LogTemp, Log, TEXT("Step 5: Executing graph..."));
	
	FMaterializeGraphExecutor Executor;
	FMaterializeGraphExecutionResult Result = Executor.Execute(TestGraph, 512, 512);

	// Step 6: Verify execution results
	UE_LOG(LogTemp, Log, TEXT("Step 6: Verifying execution results..."));
	
	// Check for errors
	if (Result.Errors.Num() > 0)
	{
		for (const FString& Error : Result.Errors)
		{
			AddError(FString::Printf(TEXT("Execution error: %s"), *Error));
		}
		return false;
	}

	// Verify nodes were executed
	if (Result.NodesExecuted == 0)
	{
		AddError(TEXT("No nodes were executed"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("  Nodes executed: %d"), Result.NodesExecuted);

	// Verify textures were created
	if (Result.TexturesCreated == 0)
	{
		AddError(TEXT("No textures were created"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("  Textures created: %d"), Result.TexturesCreated);

	// Verify execution time is reasonable
	if (Result.ExecutionTimeMs <= 0.0f)
	{
		AddError(TEXT("Invalid execution time"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("  Execution time: %.2f ms"), Result.ExecutionTimeMs);

	// Step 7: Verify output textures are generated
	UE_LOG(LogTemp, Log, TEXT("Step 7: Verifying output textures..."));
	
	if (!Result.BaseColor)
	{
		AddError(TEXT("BaseColor texture was not generated"));
		return false;
	}

	// Verify texture dimensions
	if (Result.BaseColor->GetSizeX() != 512 || Result.BaseColor->GetSizeY() != 512)
	{
		AddError(FString::Printf(TEXT("BaseColor texture has incorrect dimensions: %dx%d (expected 512x512)"),
			Result.BaseColor->GetSizeX(), Result.BaseColor->GetSizeY()));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("  BaseColor texture: %dx%d"), 
		Result.BaseColor->GetSizeX(), Result.BaseColor->GetSizeY());

	// Verify texture is valid
	if (!Result.BaseColor->IsValidLowLevel())
	{
		AddError(TEXT("BaseColor texture is not valid"));
		return false;
	}

	// Step 8: Summary
	UE_LOG(LogTemp, Log, TEXT("=== Graph Backend Verification PASSED ==="));
	UE_LOG(LogTemp, Log, TEXT("Summary:"));
	UE_LOG(LogTemp, Log, TEXT("  - Graph created with 2 nodes (Noise + Output)"));
	UE_LOG(LogTemp, Log, TEXT("  - Nodes connected successfully"));
	UE_LOG(LogTemp, Log, TEXT("  - Graph executed in %.2f ms"), Result.ExecutionTimeMs);
	UE_LOG(LogTemp, Log, TEXT("  - %d nodes executed, %d textures created"), 
		Result.NodesExecuted, Result.TexturesCreated);
	UE_LOG(LogTemp, Log, TEXT("  - BaseColor output texture generated: %dx%d"),
		Result.BaseColor->GetSizeX(), Result.BaseColor->GetSizeY());

	return true;
}
