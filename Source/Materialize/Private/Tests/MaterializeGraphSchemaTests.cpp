// Copyright K-Studio. All Rights Reserved.

// NOTE: These tests are temporarily disabled due to UE5 API changes
// UEdGraphPin cannot be instantiated directly with NewObject in UE5
// Tests need to be rewritten to use actual graph nodes
#if 0

#include "Misc/AutomationTest.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "EdGraph/EdGraphPin.h"

/**
 * Test: Pin Compatibility - Normal to Normal (Valid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaNormalToNormalTest,
	"Materialize.Graph.Schema.PinCompatibility.NormalToNormal",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaNormalToNormalTest::RunTest(const FString& Parameters)
{
	// Create a test graph
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create two mock pins with Normal type
	UEdGraphPin* OutputPin = NewObject<UEdGraphPin>();
	OutputPin->Direction = EGPD_Output;
	OutputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Normal;

	UEdGraphPin* InputPin = NewObject<UEdGraphPin>();
	InputPin->Direction = EGPD_Input;
	InputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Normal;

	// Test connection
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin, InputPin);
	
	TestTrue(TEXT("Normal to Normal connection should be allowed"), 
		Response.Response == CONNECT_RESPONSE_MAKE);

	return true;
}

/**
 * Test: Pin Compatibility - Normal to Texture (Invalid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaNormalToTextureTest,
	"Materialize.Graph.Schema.PinCompatibility.NormalToTexture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaNormalToTextureTest::RunTest(const FString& Parameters)
{
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create pins with incompatible types
	UEdGraphPin* OutputPin = NewObject<UEdGraphPin>();
	OutputPin->Direction = EGPD_Output;
	OutputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Normal;

	UEdGraphPin* InputPin = NewObject<UEdGraphPin>();
	InputPin->Direction = EGPD_Input;
	InputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Texture;

	// Test connection
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin, InputPin);
	
	TestTrue(TEXT("Normal to Texture connection should be disallowed"), 
		Response.Response == CONNECT_RESPONSE_DISALLOW);

	return true;
}

/**
 * Test: Pin Compatibility - Texture to Vector (Valid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaTextureToVectorTest,
	"Materialize.Graph.Schema.PinCompatibility.TextureToVector",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaTextureToVectorTest::RunTest(const FString& Parameters)
{
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create pins with compatible types (Color/Texture and Vector)
	UEdGraphPin* OutputPin = NewObject<UEdGraphPin>();
	OutputPin->Direction = EGPD_Output;
	OutputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Texture;

	UEdGraphPin* InputPin = NewObject<UEdGraphPin>();
	InputPin->Direction = EGPD_Input;
	InputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Vector;

	// Test connection
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin, InputPin);
	
	TestTrue(TEXT("Texture to Vector connection should be allowed"), 
		Response.Response == CONNECT_RESPONSE_MAKE);

	return true;
}

/**
 * Test: Pin Compatibility - Vector to Texture (Valid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaVectorToTextureTest,
	"Materialize.Graph.Schema.PinCompatibility.VectorToTexture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaVectorToTextureTest::RunTest(const FString& Parameters)
{
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create pins with compatible types (Vector and Color/Texture)
	UEdGraphPin* OutputPin = NewObject<UEdGraphPin>();
	OutputPin->Direction = EGPD_Output;
	OutputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Vector;

	UEdGraphPin* InputPin = NewObject<UEdGraphPin>();
	InputPin->Direction = EGPD_Input;
	InputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Texture;

	// Test connection
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin, InputPin);
	
	TestTrue(TEXT("Vector to Texture connection should be allowed"), 
		Response.Response == CONNECT_RESPONSE_MAKE);

	return true;
}

/**
 * Test: Pin Compatibility - Scalar to Scalar (Valid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaScalarToScalarTest,
	"Materialize.Graph.Schema.PinCompatibility.ScalarToScalar",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaScalarToScalarTest::RunTest(const FString& Parameters)
{
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create pins with Scalar type
	UEdGraphPin* OutputPin = NewObject<UEdGraphPin>();
	OutputPin->Direction = EGPD_Output;
	OutputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Scalar;

	UEdGraphPin* InputPin = NewObject<UEdGraphPin>();
	InputPin->Direction = EGPD_Input;
	InputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Scalar;

	// Test connection
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin, InputPin);
	
	TestTrue(TEXT("Scalar to Scalar connection should be allowed"), 
		Response.Response == CONNECT_RESPONSE_MAKE);

	return true;
}

/**
 * Test: Pin Compatibility - Scalar to Mask (Valid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaScalarToMaskTest,
	"Materialize.Graph.Schema.PinCompatibility.ScalarToMask",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaScalarToMaskTest::RunTest(const FString& Parameters)
{
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create pins with Scalar and Mask types (should be compatible)
	UEdGraphPin* OutputPin = NewObject<UEdGraphPin>();
	OutputPin->Direction = EGPD_Output;
	OutputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Scalar;

	UEdGraphPin* InputPin = NewObject<UEdGraphPin>();
	InputPin->Direction = EGPD_Input;
	InputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Mask;

	// Test connection
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin, InputPin);
	
	TestTrue(TEXT("Scalar to Mask connection should be allowed"), 
		Response.Response == CONNECT_RESPONSE_MAKE);

	return true;
}

/**
 * Test: Pin Compatibility - Scalar to Texture (Invalid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaScalarToTextureTest,
	"Materialize.Graph.Schema.PinCompatibility.ScalarToTexture",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaScalarToTextureTest::RunTest(const FString& Parameters)
{
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create pins with incompatible types
	UEdGraphPin* OutputPin = NewObject<UEdGraphPin>();
	OutputPin->Direction = EGPD_Output;
	OutputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Scalar;

	UEdGraphPin* InputPin = NewObject<UEdGraphPin>();
	InputPin->Direction = EGPD_Input;
	InputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Texture;

	// Test connection
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin, InputPin);
	
	TestTrue(TEXT("Scalar to Texture connection should be disallowed"), 
		Response.Response == CONNECT_RESPONSE_DISALLOW);

	return true;
}

/**
 * Test: Pin Compatibility - Same Direction (Invalid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaSameDirectionTest,
	"Materialize.Graph.Schema.PinCompatibility.SameDirection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaSameDirectionTest::RunTest(const FString& Parameters)
{
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create two output pins (same direction)
	UEdGraphPin* OutputPin1 = NewObject<UEdGraphPin>();
	OutputPin1->Direction = EGPD_Output;
	OutputPin1->PinType.PinCategory = UMaterializeGraphSchema::PC_Texture;

	UEdGraphPin* OutputPin2 = NewObject<UEdGraphPin>();
	OutputPin2->Direction = EGPD_Output;
	OutputPin2->PinType.PinCategory = UMaterializeGraphSchema::PC_Texture;

	// Test connection
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin1, OutputPin2);
	
	TestTrue(TEXT("Same direction connection should be disallowed"), 
		Response.Response == CONNECT_RESPONSE_DISALLOW);

	return true;
}

/**
 * Test: Pin Compatibility - Null Pin (Invalid)
 * Validates: Requirement 4.4 - Pin type compatibility rules
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSchemaNullPinTest,
	"Materialize.Graph.Schema.PinCompatibility.NullPin",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSchemaNullPinTest::RunTest(const FString& Parameters)
{
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	const UMaterializeGraphSchema* Schema = Cast<UMaterializeGraphSchema>(Graph->GetSchema());
	
	if (!Schema)
	{
		AddError(TEXT("Failed to get graph schema"));
		return false;
	}

	// Create one valid pin
	UEdGraphPin* OutputPin = NewObject<UEdGraphPin>();
	OutputPin->Direction = EGPD_Output;
	OutputPin->PinType.PinCategory = UMaterializeGraphSchema::PC_Texture;

	// Test connection with null pin
	FPinConnectionResponse Response = Schema->CanCreateConnection(OutputPin, nullptr);
	
	TestTrue(TEXT("Null pin connection should be disallowed"), 
		Response.Response == CONNECT_RESPONSE_DISALLOW);

	return true;
}


#endif // 0
