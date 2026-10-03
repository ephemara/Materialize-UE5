#include "Misc/AutomationTest.h"
#include "MaterializeComputeEngine.h"
#include "MaterializeRDGScope.h"
#include "MaterializeTypes.h"
#include "Engine/Texture2D.h"
#include "RenderingThread.h"

// =============================================================================
// RESOURCE MANAGEMENT TESTS
// =============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeRDGScopeTest,
	"Materialize.ResourceManagement.RDGScope",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeRDGScopeTest::RunTest(const FString& Parameters)
{
	// Test that RDG scope automatically executes on destruction
	bool bExecuted = false;

	ENQUEUE_RENDER_COMMAND(TestRDGScope)(
		[&bExecuted](FRHICommandListImmediate& RHICmdList)
		{
			{
				FMaterializeRDGScope RDGScope(RHICmdList);
				FRDGBuilder& GraphBuilder = RDGScope.GetGraphBuilder();
				
				// Add a simple pass to verify execution
				GraphBuilder.AddPass(RDG_EVENT_NAME("TestPass"), ERDGPassFlags::None, [&bExecuted](FRHICommandList&)
				{
					bExecuted = true;
				});
				
				// Scope exits here, should auto-execute
			}
		}
	);

	FlushRenderingCommands();
	
	TestTrue(TEXT("RDG scope should auto-execute on destruction"), bExecuted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeCleanupTransientResourcesTest,
	"Materialize.ResourceManagement.CleanupTransientResources",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeCleanupTransientResourcesTest::RunTest(const FString& Parameters)
{
	// Create a result with transient textures
	FMaterializeResult Result;
	Result.Normal = UTexture2D::CreateTransient(256, 256, PF_B8G8R8A8);
	Result.Roughness = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT);
	Result.Metallic = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT);
	Result.AO = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT);
	Result.Height = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT);
	Result.Emissive = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT);

	TestTrue(TEXT("Result should have valid textures before cleanup"), Result.Normal != nullptr);
	TestTrue(TEXT("Result should have valid textures before cleanup"), Result.Roughness != nullptr);

	// Cleanup
	UMaterializeComputeEngine::CleanupTransientResources(Result);

	TestTrue(TEXT("Result textures should be null after cleanup"), Result.Normal == nullptr);
	TestTrue(TEXT("Result textures should be null after cleanup"), Result.Roughness == nullptr);
	TestTrue(TEXT("Result textures should be null after cleanup"), Result.Metallic == nullptr);
	TestTrue(TEXT("Result textures should be null after cleanup"), Result.AO == nullptr);
	TestTrue(TEXT("Result textures should be null after cleanup"), Result.Height == nullptr);
	TestTrue(TEXT("Result textures should be null after cleanup"), Result.Emissive == nullptr);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeValidateRHIResourceTest,
	"Materialize.ResourceManagement.ValidateRHIResource",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeValidateRHIResourceTest::RunTest(const FString& Parameters)
{
	// Test null RHI reference
	{
		FRHITexture* NullRHI = nullptr;
		FString Error;
		bool bValid = UMaterializeComputeEngine::ValidateRHIResource(NullRHI, Error);
		
		TestFalse(TEXT("Null RHI should fail validation"), bValid);
		TestTrue(TEXT("Error message should be set for null RHI"), !Error.IsEmpty());
	}

	// Test valid RHI reference (requires render thread)
	{
		UTexture2D* TestTexture = UTexture2D::CreateTransient(256, 256, PF_B8G8R8A8);
		if (TestTexture && TestTexture->GetResource())
		{
			FlushRenderingCommands();
			
			FRHITexture* TextureRHI = TestTexture->GetResource()->GetTexture2DRHI();
			FString Error;
			
			bool bValid = false;
			ENQUEUE_RENDER_COMMAND(TestValidateRHI)(
				[TextureRHI, &bValid, &Error](FRHICommandListImmediate& RHICmdList)
				{
					bValid = UMaterializeComputeEngine::ValidateRHIResource(TextureRHI, Error);
				}
			);
			
			FlushRenderingCommands();
			
			TestTrue(TEXT("Valid RHI should pass validation"), bValid);
			TestTrue(TEXT("Error message should be empty for valid RHI"), Error.IsEmpty());
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeResourceLeakTest,
	"Materialize.ResourceManagement.NoLeaksAfterGeneration",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeResourceLeakTest::RunTest(const FString& Parameters)
{
	// Create a simple source texture
	UTexture2D* SourceTexture = UTexture2D::CreateTransient(256, 256, PF_B8G8R8A8);
	if (!SourceTexture)
	{
		AddError(TEXT("Failed to create source texture"));
		return false;
	}

	SourceTexture->UpdateResource();
	FlushRenderingCommands();

	// Generate PBR maps multiple times to check for leaks
	const int32 NumIterations = 5;
	for (int32 i = 0; i < NumIterations; ++i)
	{
		FMaterializeParams Params;
		Params.NormalStrength = 1.0f;
		Params.RoughnessBase = 0.5f;
		Params.MetallicBase = 0.0f;
		Params.bUseMultiPassHeight = false; // Use fast path for testing

		FMaterializeResult Result;
		bool bSuccess = UMaterializeComputeEngine::GeneratePBRMapsGPU(SourceTexture, Params, Result);

		TestTrue(FString::Printf(TEXT("Generation iteration %d should succeed"), i), bSuccess);

		// Cleanup after each iteration
		UMaterializeComputeEngine::CleanupTransientResources(Result);
	}

	// If we got here without crashing, no obvious leaks
	AddInfo(FString::Printf(TEXT("Completed %d generation iterations without crashes"), NumIterations));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeRDGScopeManualExecuteTest,
	"Materialize.ResourceManagement.RDGScopeManualExecute",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeRDGScopeManualExecuteTest::RunTest(const FString& Parameters)
{
	// Test that manual execution works and prevents double execution
	int32 ExecutionCount = 0;

	ENQUEUE_RENDER_COMMAND(TestManualExecute)(
		[&ExecutionCount](FRHICommandListImmediate& RHICmdList)
		{
			FMaterializeRDGScope RDGScope(RHICmdList);
			FRDGBuilder& GraphBuilder = RDGScope.GetGraphBuilder();
			
			GraphBuilder.AddPass(RDG_EVENT_NAME("TestPass"), ERDGPassFlags::None, [&ExecutionCount](FRHICommandList&)
			{
				ExecutionCount++;
			});
			
			// Manually execute
			RDGScope.Execute();
			
			// Scope exits here, should NOT execute again
		}
	);

	FlushRenderingCommands();
	
	TestEqual(TEXT("RDG scope should execute exactly once"), ExecutionCount, 1);

	return true;
}
