#include "Misc/AutomationTest.h"
#include "KLayerEvaluator.h"
#include "KLayerStack.h"
#include "Engine/Texture2D.h"

// =============================================================================
// ALPHA BLENDING AND LAYER ORDERING TESTS
// =============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerAlphaBlendingTest,
	"Materialize.Layer.AlphaBlending",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerAlphaBlendingTest::RunTest(const FString& Parameters)
{
	// Test 1: Verify layer ordering (bottom-to-top)
	{
		FKLayerStack Stack;
		Stack.Width = 256;
		Stack.Height = 256;

		// Add three layers
		FKLayer Layer0;
		Layer0.Name = FName(TEXT("Bottom"));
		Layer0.bEnabled = true;
		Stack.Layers.Add(Layer0);

		FKLayer Layer1;
		Layer1.Name = FName(TEXT("Middle"));
		Layer1.bEnabled = true;
		Stack.Layers.Add(Layer1);

		FKLayer Layer2;
		Layer2.Name = FName(TEXT("Top"));
		Layer2.bEnabled = true;
		Stack.Layers.Add(Layer2);

		TArray<int32> VisibleIndices = Stack.GetVisibleLayerIndices();
		
		TestEqual(TEXT("Should have 3 visible layers"), VisibleIndices.Num(), 3);
		TestEqual(TEXT("First layer should be index 0 (bottom)"), VisibleIndices[0], 0);
		TestEqual(TEXT("Second layer should be index 1 (middle)"), VisibleIndices[1], 1);
		TestEqual(TEXT("Third layer should be index 2 (top)"), VisibleIndices[2], 2);
	}

	// Test 2: Verify solo layer behavior
	{
		FKLayerStack Stack;
		Stack.Width = 256;
		Stack.Height = 256;

		FKLayer Layer0;
		Layer0.Name = FName(TEXT("Layer0"));
		Layer0.bEnabled = true;
		Layer0.bSolo = false;
		Stack.Layers.Add(Layer0);

		FKLayer Layer1;
		Layer1.Name = FName(TEXT("Layer1"));
		Layer1.bEnabled = true;
		Layer1.bSolo = true; // Solo this layer
		Stack.Layers.Add(Layer1);

		FKLayer Layer2;
		Layer2.Name = FName(TEXT("Layer2"));
		Layer2.bEnabled = true;
		Layer2.bSolo = false;
		Stack.Layers.Add(Layer2);

		TArray<int32> VisibleIndices = Stack.GetVisibleLayerIndices();
		
		TestEqual(TEXT("Should have only 1 visible layer (solo)"), VisibleIndices.Num(), 1);
		TestEqual(TEXT("Solo layer should be index 1"), VisibleIndices[0], 1);
	}

	// Test 3: Verify disabled layer filtering
	{
		FKLayerStack Stack;
		Stack.Width = 256;
		Stack.Height = 256;

		FKLayer Layer0;
		Layer0.Name = FName(TEXT("Layer0"));
		Layer0.bEnabled = true;
		Stack.Layers.Add(Layer0);

		FKLayer Layer1;
		Layer1.Name = FName(TEXT("Layer1"));
		Layer1.bEnabled = false; // Disabled
		Stack.Layers.Add(Layer1);

		FKLayer Layer2;
		Layer2.Name = FName(TEXT("Layer2"));
		Layer2.bEnabled = true;
		Stack.Layers.Add(Layer2);

		TArray<int32> VisibleIndices = Stack.GetVisibleLayerIndices();
		
		TestEqual(TEXT("Should have 2 visible layers"), VisibleIndices.Num(), 2);
		TestEqual(TEXT("First visible should be index 0"), VisibleIndices[0], 0);
		TestEqual(TEXT("Second visible should be index 2"), VisibleIndices[1], 2);
	}

	// Test 4: Verify opacity validation
	{
		FKLayerStack Stack;
		Stack.Width = 256;
		Stack.Height = 256;

		FKLayer Layer;
		Layer.Name = FName(TEXT("TestLayer"));
		Layer.bEnabled = true;
		Layer.Opacity = 1.5f; // Invalid opacity
		Stack.Layers.Add(Layer);

		FString Error;
		bool bValid = UKLayerEvaluator::ValidateLayerStack(Stack, Error);
		
		TestFalse(TEXT("Stack with invalid opacity should fail validation"), bValid);
		TestTrue(TEXT("Error message should mention opacity"), Error.Contains(TEXT("opacity")));
	}

	// Test 5: Verify blend mode validation
	{
		FKLayerStack Stack;
		Stack.Width = 256;
		Stack.Height = 256;

		FKLayer Layer;
		Layer.Name = FName(TEXT("TestLayer"));
		Layer.bEnabled = true;
		Layer.Opacity = 0.8f;
		Layer.BlendMode = static_cast<EKLayerBlendMode>(999); // Invalid blend mode
		Stack.Layers.Add(Layer);

		FString Error;
		bool bValid = UKLayerEvaluator::ValidateLayerStack(Stack, Error);
		
		TestFalse(TEXT("Stack with invalid blend mode should fail validation"), bValid);
		TestTrue(TEXT("Error message should mention blend mode"), Error.Contains(TEXT("blend mode")));
	}

	return true;
}

// =============================================================================
// BLEND MODE CORRECTNESS TESTS
// =============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerBlendModeValidationTest,
	"Materialize.Layer.BlendModeValidation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerBlendModeValidationTest::RunTest(const FString& Parameters)
{
	// Test all blend modes are within valid range
	TArray<EKLayerBlendMode> BlendModes = {
		EKLayerBlendMode::Normal,
		EKLayerBlendMode::Multiply,
		EKLayerBlendMode::Screen,
		EKLayerBlendMode::Overlay,
		EKLayerBlendMode::SoftLight,
		EKLayerBlendMode::HardLight,
		EKLayerBlendMode::Add,
		EKLayerBlendMode::Subtract,
		EKLayerBlendMode::Difference,
		EKLayerBlendMode::Exclusion,
		EKLayerBlendMode::Darken,
		EKLayerBlendMode::Lighten,
		EKLayerBlendMode::ColorDodge,
		EKLayerBlendMode::ColorBurn,
		EKLayerBlendMode::LinearDodge,
		EKLayerBlendMode::LinearBurn,
		EKLayerBlendMode::VividLight,
		EKLayerBlendMode::LinearLight,
		EKLayerBlendMode::PinLight,
		EKLayerBlendMode::HardMix
	};

	for (EKLayerBlendMode Mode : BlendModes)
	{
		bool bValid = UKLayerEvaluator::ValidateBlendMode(Mode);
		TestTrue(FString::Printf(TEXT("Blend mode %d should be valid"), static_cast<int32>(Mode)), bValid);
	}

	// Test invalid blend mode
	EKLayerBlendMode InvalidMode = static_cast<EKLayerBlendMode>(999);
	bool bInvalidResult = UKLayerEvaluator::ValidateBlendMode(InvalidMode);
	TestFalse(TEXT("Invalid blend mode should fail validation"), bInvalidResult);

	return true;
}

// =============================================================================
// LAYER STACK VALIDATION TESTS
// =============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerStackValidationTest,
	"Materialize.Layer.StackValidation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerStackValidationTest::RunTest(const FString& Parameters)
{
	// Test 1: Empty stack should fail
	{
		FKLayerStack EmptyStack;
		EmptyStack.Width = 256;
		EmptyStack.Height = 256;

		FString Error;
		bool bValid = UKLayerEvaluator::ValidateLayerStack(EmptyStack, Error);
		
		TestFalse(TEXT("Empty stack should fail validation"), bValid);
		TestTrue(TEXT("Error should mention empty stack"), Error.Contains(TEXT("empty")));
	}

	// Test 2: Invalid dimensions should fail
	{
		FKLayerStack Stack;
		Stack.Width = 0;
		Stack.Height = 256;

		FKLayer Layer;
		Layer.Name = FName(TEXT("TestLayer"));
		Layer.bEnabled = true;
		Stack.Layers.Add(Layer);

		FString Error;
		bool bValid = UKLayerEvaluator::ValidateLayerStack(Stack, Error);
		
		TestFalse(TEXT("Stack with invalid dimensions should fail validation"), bValid);
		TestTrue(TEXT("Error should mention dimensions"), Error.Contains(TEXT("dimensions")));
	}

	// Test 3: Excessive dimensions should fail
	{
		FKLayerStack Stack;
		Stack.Width = 16384; // Exceeds maximum
		Stack.Height = 256;

		FKLayer Layer;
		Layer.Name = FName(TEXT("TestLayer"));
		Layer.bEnabled = true;
		Stack.Layers.Add(Layer);

		FString Error;
		bool bValid = UKLayerEvaluator::ValidateLayerStack(Stack, Error);
		
		TestFalse(TEXT("Stack with excessive dimensions should fail validation"), bValid);
		TestTrue(TEXT("Error should mention maximum"), Error.Contains(TEXT("maximum")) || Error.Contains(TEXT("exceed")));
	}

	// Test 4: Valid stack should pass
	{
		FKLayerStack Stack;
		Stack.Width = 512;
		Stack.Height = 512;

		FKLayer Layer;
		Layer.Name = FName(TEXT("TestLayer"));
		Layer.bEnabled = true;
		Layer.Opacity = 0.8f;
		Layer.BlendMode = EKLayerBlendMode::Normal;
		Layer.LayerType = EKLayerType::Fill;
		Layer.FillColor = FLinearColor::White;
		Stack.Layers.Add(Layer);

		FString Error;
		bool bValid = UKLayerEvaluator::ValidateLayerStack(Stack, Error);
		
		TestTrue(TEXT("Valid stack should pass validation"), bValid);
		TestTrue(TEXT("Error should be empty for valid stack"), Error.IsEmpty());
	}

	return true;
}
