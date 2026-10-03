#include "Misc/AutomationTest.h"
#include "KLayerEvaluator.h"
#include "KLayerStack.h"
#include "Engine/Texture2D.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationEmptyStackTest,
	"Materialize.Layer.Validation.EmptyStack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationEmptyStackTest::RunTest(const FString& Parameters)
{
	// Test empty layer stack rejection
	FKLayerStack EmptyStack;
	FString Error;
	
	TestFalse(TEXT("Empty layer stack should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(EmptyStack, Error));
	TestTrue(TEXT("Error message should be set"), !Error.IsEmpty());
	TestTrue(TEXT("Error should mention empty stack"), Error.Contains(TEXT("empty")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationInvalidDimensionsTest,
	"Materialize.Layer.Validation.InvalidDimensions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationInvalidDimensionsTest::RunTest(const FString& Parameters)
{
	// Test invalid dimensions
	FKLayerStack Stack;
	Stack.Width = -1;
	Stack.Height = 1024;
	Stack.Layers.Add(FKLayer(TEXT("TestLayer"), EKLayerType::Fill));
	
	FString Error;
	TestFalse(TEXT("Negative width should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention invalid dimensions"), Error.Contains(TEXT("invalid dimensions")));
	
	// Test excessive dimensions
	Stack.Width = 16384;
	Stack.Height = 16384;
	Error.Empty();
	
	TestFalse(TEXT("Excessive dimensions should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention maximum"), Error.Contains(TEXT("maximum")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationInvalidOpacityTest,
	"Materialize.Layer.Validation.InvalidOpacity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationInvalidOpacityTest::RunTest(const FString& Parameters)
{
	// Test invalid opacity
	FKLayerStack Stack;
	Stack.Width = 1024;
	Stack.Height = 1024;
	
	FKLayer Layer(TEXT("TestLayer"), EKLayerType::Fill);
	Layer.Opacity = 1.5f; // Invalid
	Stack.Layers.Add(Layer);
	
	FString Error;
	TestFalse(TEXT("Opacity > 1.0 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention opacity"), Error.Contains(TEXT("opacity")));
	
	// Test negative opacity
	Stack.Layers[0].Opacity = -0.5f;
	Error.Empty();
	
	TestFalse(TEXT("Negative opacity should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationValidStackTest,
	"Materialize.Layer.Validation.ValidStack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationValidStackTest::RunTest(const FString& Parameters)
{
	// Test valid layer stack acceptance
	FKLayerStack Stack;
	Stack.Width = 1024;
	Stack.Height = 1024;
	
	FKLayer Layer(TEXT("TestLayer"), EKLayerType::Fill);
	Layer.Opacity = 0.8f;
	Layer.FillColor = FLinearColor::White;
	Stack.Layers.Add(Layer);
	
	FString Error;
	TestTrue(TEXT("Valid layer stack should pass validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error message should be empty"), Error.IsEmpty());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationBlendModeTest,
	"Materialize.Layer.Validation.BlendMode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationBlendModeTest::RunTest(const FString& Parameters)
{
	// Test valid blend modes
	TestTrue(TEXT("Normal blend mode should be valid"),
		UKLayerEvaluator::ValidateBlendMode(EKLayerBlendMode::Normal));
	TestTrue(TEXT("Multiply blend mode should be valid"),
		UKLayerEvaluator::ValidateBlendMode(EKLayerBlendMode::Multiply));
	TestTrue(TEXT("Screen blend mode should be valid"),
		UKLayerEvaluator::ValidateBlendMode(EKLayerBlendMode::Screen));
	TestTrue(TEXT("HardMix blend mode should be valid"),
		UKLayerEvaluator::ValidateBlendMode(EKLayerBlendMode::HardMix));
	
	// Test invalid blend mode (cast from out-of-range value)
	EKLayerBlendMode InvalidMode = static_cast<EKLayerBlendMode>(255);
	TestFalse(TEXT("Out-of-range blend mode should be invalid"),
		UKLayerEvaluator::ValidateBlendMode(InvalidMode));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationFilterTypeTest,
	"Materialize.Layer.Validation.FilterType",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationFilterTypeTest::RunTest(const FString& Parameters)
{
	// Test valid filter types
	TestTrue(TEXT("Blur filter should be valid"),
		UKLayerEvaluator::ValidateFilterType(EKFilterType::Blur));
	TestTrue(TEXT("Sharpen filter should be valid"),
		UKLayerEvaluator::ValidateFilterType(EKFilterType::Sharpen));
	TestTrue(TEXT("EdgeDetect filter should be valid"),
		UKLayerEvaluator::ValidateFilterType(EKFilterType::EdgeDetect));
	TestTrue(TEXT("AutoLevels filter should be valid"),
		UKLayerEvaluator::ValidateFilterType(EKFilterType::AutoLevels));
	
	// Test invalid filter type
	EKFilterType InvalidFilter = static_cast<EKFilterType>(255);
	TestFalse(TEXT("Out-of-range filter type should be invalid"),
		UKLayerEvaluator::ValidateFilterType(InvalidFilter));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationProceduralParamsTest,
	"Materialize.Layer.Validation.ProceduralParams",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationProceduralParamsTest::RunTest(const FString& Parameters)
{
	FKLayerStack Stack;
	Stack.Width = 1024;
	Stack.Height = 1024;
	
	// Test invalid scale
	FKLayer Layer(TEXT("ProceduralLayer"), EKLayerType::Procedural);
	Layer.ProceduralParams.Scale = 150.0f; // Invalid (max 100.0)
	Stack.Layers.Add(Layer);
	
	FString Error;
	TestFalse(TEXT("Scale > 100.0 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention scale"), Error.Contains(TEXT("scale")));
	
	// Test invalid octaves
	Stack.Layers[0].ProceduralParams.Scale = 1.0f;
	Stack.Layers[0].ProceduralParams.Octaves = 20; // Invalid (max 16)
	Error.Empty();
	
	TestFalse(TEXT("Octaves > 16 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention octaves"), Error.Contains(TEXT("octaves")));
	
	// Test invalid persistence
	Stack.Layers[0].ProceduralParams.Octaves = 4;
	Stack.Layers[0].ProceduralParams.Persistence = 1.5f; // Invalid (max 1.0)
	Error.Empty();
	
	TestFalse(TEXT("Persistence > 1.0 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention persistence"), Error.Contains(TEXT("persistence")));
	
	// Test valid procedural params
	Stack.Layers[0].ProceduralParams.Persistence = 0.5f;
	Stack.Layers[0].ProceduralParams.Lacunarity = 2.0f;
	Error.Empty();
	
	TestTrue(TEXT("Valid procedural params should pass validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationFilterParamsTest,
	"Materialize.Layer.Validation.FilterParams",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationFilterParamsTest::RunTest(const FString& Parameters)
{
	FKLayerStack Stack;
	Stack.Width = 1024;
	Stack.Height = 1024;
	
	// Test invalid filter intensity
	FKLayer Layer(TEXT("FilterLayer"), EKLayerType::Filter);
	Layer.FilterParams.Intensity = 150.0f; // Invalid (max 100.0)
	Stack.Layers.Add(Layer);
	
	FString Error;
	TestFalse(TEXT("Intensity > 100.0 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention intensity"), Error.Contains(TEXT("intensity")));
	
	// Test invalid kernel size
	Stack.Layers[0].FilterParams.Intensity = 1.0f;
	Stack.Layers[0].FilterParams.KernelSize = 50; // Invalid (max 32)
	Error.Empty();
	
	TestFalse(TEXT("KernelSize > 32 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention kernel size"), Error.Contains(TEXT("kernel size")));
	
	// Test valid filter params
	Stack.Layers[0].FilterParams.KernelSize = 5;
	Error.Empty();
	
	TestTrue(TEXT("Valid filter params should pass validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKLayerValidationAdjustmentParamsTest,
	"Materialize.Layer.Validation.AdjustmentParams",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FKLayerValidationAdjustmentParamsTest::RunTest(const FString& Parameters)
{
	FKLayerStack Stack;
	Stack.Width = 1024;
	Stack.Height = 1024;
	
	// Test invalid gamma
	FKLayer Layer(TEXT("AdjustmentLayer"), EKLayerType::Adjustment);
	Layer.AdjustmentParams.Gamma = 15.0f; // Invalid (max 9.9)
	Stack.Layers.Add(Layer);
	
	FString Error;
	TestFalse(TEXT("Gamma > 9.9 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention Gamma"), Error.Contains(TEXT("Gamma")));
	
	// Test invalid hue shift
	Stack.Layers[0].AdjustmentParams.Gamma = 1.0f;
	Stack.Layers[0].AdjustmentParams.HueShift = 200.0f; // Invalid (max 180.0)
	Error.Empty();
	
	TestFalse(TEXT("HueShift > 180.0 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention HueShift"), Error.Contains(TEXT("HueShift")));
	
	// Test invalid saturation
	Stack.Layers[0].AdjustmentParams.HueShift = 0.0f;
	Stack.Layers[0].AdjustmentParams.SaturationAdjust = 2.0f; // Invalid (max 1.0)
	Error.Empty();
	
	TestFalse(TEXT("SaturationAdjust > 1.0 should fail validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	TestTrue(TEXT("Error should mention SaturationAdjust"), Error.Contains(TEXT("SaturationAdjust")));
	
	// Test valid adjustment params
	Stack.Layers[0].AdjustmentParams.SaturationAdjust = 0.5f;
	Stack.Layers[0].AdjustmentParams.Brightness = 0.2f;
	Stack.Layers[0].AdjustmentParams.Contrast = -0.1f;
	Error.Empty();
	
	TestTrue(TEXT("Valid adjustment params should pass validation"),
		UKLayerEvaluator::ValidateLayerStack(Stack, Error));
	
	return true;
}
