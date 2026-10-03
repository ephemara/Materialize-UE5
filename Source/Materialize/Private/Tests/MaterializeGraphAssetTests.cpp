#include "Misc/AutomationTest.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "Graph/Nodes/MaterializeGraphNode_Noise.h"
#include "Graph/Nodes/MaterializeGraphNode_Blend.h"
#include "Graph/Nodes/MaterializeGraphNode_Output.h"
#include "Graph/Nodes/MaterializeGraphNode_ChannelOutput.h"
#include "EdGraph/EdGraphPin.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphAddNodeTest,
	"Materialize.Graph.Asset.AddNode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphAddNodeTest::RunTest(const FString& Parameters)
{
	// Create a graph
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	TestNotNull(TEXT("Graph should be created"), Graph);
	
	// Test adding a valid node
	UMaterializeGraphNode* Node = Graph->AddNode(UMaterializeGraphNode_Noise::StaticClass(), FVector2D(100.0f, 200.0f));
	TestNotNull(TEXT("Node should be created"), Node);
	TestEqual(TEXT("Node should be at correct X position"), Node->NodePosX, 100);
	TestEqual(TEXT("Node should be at correct Y position"), Node->NodePosY, 200);
	TestEqual(TEXT("Graph should contain 1 node"), Graph->Nodes.Num(), 1);
	TestTrue(TEXT("Graph Nodes array should contain the node"), Graph->Nodes.Contains(Node));
	
	// Test adding multiple nodes
	UMaterializeGraphNode* Node2 = Graph->AddNode(UMaterializeGraphNode_Blend::StaticClass(), FVector2D(300.0f, 400.0f));
	TestNotNull(TEXT("Second node should be created"), Node2);
	TestEqual(TEXT("Graph should contain 2 nodes"), Graph->Nodes.Num(), 2);
	
	// Test adding null node class
	UMaterializeGraphNode* NullNode = Graph->AddNode(nullptr, FVector2D(0.0f, 0.0f));
	TestNull(TEXT("Adding null node class should return nullptr"), NullNode);
	TestEqual(TEXT("Graph should still contain 2 nodes"), Graph->Nodes.Num(), 2);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphRemoveNodeTest,
	"Materialize.Graph.Asset.RemoveNode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphRemoveNodeTest::RunTest(const FString& Parameters)
{
	// Create a graph with nodes
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	UMaterializeGraphNode* Node1 = Graph->AddNode(UMaterializeGraphNode_Noise::StaticClass(), FVector2D(0.0f, 0.0f));
	UMaterializeGraphNode* Node2 = Graph->AddNode(UMaterializeGraphNode_Blend::StaticClass(), FVector2D(100.0f, 0.0f));
	
	TestEqual(TEXT("Graph should start with 2 nodes"), Graph->Nodes.Num(), 2);
	
	// Test removing a valid node
	bool bRemoved = Graph->RemoveNode(Node1);
	TestTrue(TEXT("RemoveNode should return true"), bRemoved);
	TestEqual(TEXT("Graph should contain 1 node after removal"), Graph->Nodes.Num(), 1);
	TestFalse(TEXT("Graph should not contain removed node"), Graph->Nodes.Contains(Node1));
	TestTrue(TEXT("Graph should still contain Node2"), Graph->Nodes.Contains(Node2));
	
	// Test removing null node
	bool bRemovedNull = Graph->RemoveNode(nullptr);
	TestFalse(TEXT("Removing null node should return false"), bRemovedNull);
	TestEqual(TEXT("Graph should still contain 1 node"), Graph->Nodes.Num(), 1);
	
	// Test removing node not in graph
	UMaterializeGraphNode* OrphanNode = NewObject<UMaterializeGraphNode_Noise>();
	bool bRemovedOrphan = Graph->RemoveNode(OrphanNode);
	TestFalse(TEXT("Removing node not in graph should return false"), bRemovedOrphan);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphConnectNodesTest,
	"Materialize.Graph.Asset.ConnectNodes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphConnectNodesTest::RunTest(const FString& Parameters)
{
	// Create a graph with nodes
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	UMaterializeGraphNode* NoiseNode = Graph->AddNode(UMaterializeGraphNode_Noise::StaticClass(), FVector2D(0.0f, 0.0f));
	UMaterializeGraphNode* BlendNode = Graph->AddNode(UMaterializeGraphNode_Blend::StaticClass(), FVector2D(200.0f, 0.0f));
	
	TestNotNull(TEXT("NoiseNode should be created"), NoiseNode);
	TestNotNull(TEXT("BlendNode should be created"), BlendNode);
	
	// Ensure nodes have pins
	TestTrue(TEXT("NoiseNode should have pins"), NoiseNode->Pins.Num() > 0);
	TestTrue(TEXT("BlendNode should have pins"), BlendNode->Pins.Num() > 0);
	
	// Find output pin on NoiseNode and input pin on BlendNode
	UEdGraphPin* OutputPin = nullptr;
	for (UEdGraphPin* Pin : NoiseNode->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Output)
		{
			OutputPin = Pin;
			break;
		}
	}
	
	UEdGraphPin* InputPin = nullptr;
	for (UEdGraphPin* Pin : BlendNode->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input)
		{
			InputPin = Pin;
			break;
		}
	}
	
	if (OutputPin && InputPin)
	{
		int32 OutputPinIndex = NoiseNode->Pins.IndexOfByKey(OutputPin);
		int32 InputPinIndex = BlendNode->Pins.IndexOfByKey(InputPin);
		
		// Test valid connection
		bool bConnected = Graph->ConnectNodes(NoiseNode, OutputPinIndex, BlendNode, InputPinIndex);
		TestTrue(TEXT("ConnectNodes should succeed with valid pins"), bConnected);
		TestTrue(TEXT("Output pin should have connection"), OutputPin->LinkedTo.Num() > 0);
		TestTrue(TEXT("Input pin should have connection"), InputPin->LinkedTo.Num() > 0);
	}
	
	// Test connecting with null nodes
	bool bConnectedNull = Graph->ConnectNodes(nullptr, 0, BlendNode, 0);
	TestFalse(TEXT("ConnectNodes should fail with null OutputNode"), bConnectedNull);
	
	bConnectedNull = Graph->ConnectNodes(NoiseNode, 0, nullptr, 0);
	TestFalse(TEXT("ConnectNodes should fail with null InputNode"), bConnectedNull);
	
	// Test connecting with invalid pin indices
	bool bConnectedInvalid = Graph->ConnectNodes(NoiseNode, 999, BlendNode, 0);
	TestFalse(TEXT("ConnectNodes should fail with invalid OutputPinIndex"), bConnectedInvalid);
	
	bConnectedInvalid = Graph->ConnectNodes(NoiseNode, 0, BlendNode, 999);
	TestFalse(TEXT("ConnectNodes should fail with invalid InputPinIndex"), bConnectedInvalid);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphSerializationTest,
	"Materialize.Graph.Asset.Serialization",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphSerializationTest::RunTest(const FString& Parameters)
{
	// Create a graph with nodes
	UMaterializeGraph* OriginalGraph = NewObject<UMaterializeGraph>();
	UMaterializeGraphNode* Node1 = OriginalGraph->AddNode(UMaterializeGraphNode_Noise::StaticClass(), FVector2D(100.0f, 200.0f));
	UMaterializeGraphNode* Node2 = OriginalGraph->AddNode(UMaterializeGraphNode_Blend::StaticClass(), FVector2D(300.0f, 400.0f));
	UMaterializeGraphNode* Node3 = OriginalGraph->AddNode(UMaterializeGraphNode_Output::StaticClass(), FVector2D(500.0f, 600.0f));
	
	TestEqual(TEXT("Original graph should have 3 nodes"), OriginalGraph->Nodes.Num(), 3);
	
	// Serialize to memory
	TArray<uint8> SerializedData;
	FMemoryWriter Writer(SerializedData);
	OriginalGraph->Serialize(Writer);
	
	TestTrue(TEXT("Serialized data should not be empty"), SerializedData.Num() > 0);
	
	// Create a new graph and deserialize
	UMaterializeGraph* DeserializedGraph = NewObject<UMaterializeGraph>();
	FMemoryReader Reader(SerializedData);
	DeserializedGraph->Serialize(Reader);
	
	// Verify deserialized graph has same number of nodes
	TestEqual(TEXT("Deserialized graph should have 3 nodes"), DeserializedGraph->Nodes.Num(), 3);
	
	// Note: Full validation of node properties would require more complex setup
	// This test validates that the serialization mechanism works at a basic level
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphNodeGuidTest,
	"Materialize.Graph.Asset.NodeGuid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphNodeGuidTest::RunTest(const FString& Parameters)
{
	// Create a graph with nodes
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	UMaterializeGraphNode* Node1 = Graph->AddNode(UMaterializeGraphNode_Noise::StaticClass(), FVector2D(0.0f, 0.0f));
	UMaterializeGraphNode* Node2 = Graph->AddNode(UMaterializeGraphNode_Blend::StaticClass(), FVector2D(100.0f, 0.0f));
	
	// Verify each node has a valid GUID
	TestTrue(TEXT("Node1 should have valid GUID"), Node1->NodeGuid.IsValid());
	TestTrue(TEXT("Node2 should have valid GUID"), Node2->NodeGuid.IsValid());
	
	// Verify GUIDs are unique
	TestNotEqual(TEXT("Node GUIDs should be unique"), Node1->NodeGuid, Node2->NodeGuid);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphChannelOutputNodeTest,
	"Materialize.Graph.Asset.ChannelOutputNode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphChannelOutputNodeTest::RunTest(const FString& Parameters)
{
	// Create a graph with a channel output node
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	UMaterializeGraphNode_ChannelOutput* OutputNode = Cast<UMaterializeGraphNode_ChannelOutput>(
		Graph->AddNode(UMaterializeGraphNode_ChannelOutput::StaticClass(), FVector2D(0.0f, 0.0f))
	);
	
	TestNotNull(TEXT("Channel output node should be created"), OutputNode);
	
	// Test default channel
	TestEqual(TEXT("Default channel should be BaseColor"), 
		OutputNode->OutputChannel, EMaterializeOutputChannel::BaseColor);
	
	// Test channel name retrieval
	FName ChannelName = OutputNode->GetChannelName();
	TestEqual(TEXT("Channel name should be 'BaseColor'"), ChannelName, FName(TEXT("BaseColor")));
	
	// Test expected pin type for BaseColor
	EMaterializeOutputPinType PinType = OutputNode->GetExpectedPinType();
	TestEqual(TEXT("BaseColor should expect Color pin type"), 
		PinType, EMaterializeOutputPinType::Color);
	
	// Test changing channel to Roughness
	OutputNode->OutputChannel = EMaterializeOutputChannel::Roughness;
	ChannelName = OutputNode->GetChannelName();
	TestEqual(TEXT("Channel name should be 'Roughness'"), ChannelName, FName(TEXT("Roughness")));
	
	PinType = OutputNode->GetExpectedPinType();
	TestEqual(TEXT("Roughness should expect Scalar pin type"), 
		PinType, EMaterializeOutputPinType::Scalar);
	
	// Test Normal channel
	OutputNode->OutputChannel = EMaterializeOutputChannel::Normal;
	PinType = OutputNode->GetExpectedPinType();
	TestEqual(TEXT("Normal should expect Normal pin type"), 
		PinType, EMaterializeOutputPinType::Normal);
	
	// Test that output node has exactly one input pin
	TestEqual(TEXT("Output node should have exactly 1 pin"), OutputNode->Pins.Num(), 1);
	
	if (OutputNode->Pins.Num() > 0)
	{
		UEdGraphPin* Pin = OutputNode->Pins[0];
		TestEqual(TEXT("Pin should be input direction"), Pin->Direction, EGPD_Input);
	}
	
	// Test node title
	FText NodeTitle = OutputNode->GetNodeTitle(ENodeTitleType::FullTitle);
	TestTrue(TEXT("Node title should contain 'Output'"), NodeTitle.ToString().Contains(TEXT("Output")));
	
	// Test that node cannot be duplicated
	TestFalse(TEXT("Channel output node should not be duplicatable"), OutputNode->CanDuplicateNode());
	
	// Test that node can be deleted
	TestTrue(TEXT("Channel output node should be deletable"), OutputNode->CanUserDeleteNode());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaterializeGraphMultipleChannelOutputsTest,
	"Materialize.Graph.Asset.MultipleChannelOutputs",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMaterializeGraphMultipleChannelOutputsTest::RunTest(const FString& Parameters)
{
	// Create a graph with multiple channel output nodes
	UMaterializeGraph* Graph = NewObject<UMaterializeGraph>();
	
	// Add BaseColor output
	UMaterializeGraphNode_ChannelOutput* BaseColorOutput = Cast<UMaterializeGraphNode_ChannelOutput>(
		Graph->AddNode(UMaterializeGraphNode_ChannelOutput::StaticClass(), FVector2D(0.0f, 0.0f))
	);
	BaseColorOutput->OutputChannel = EMaterializeOutputChannel::BaseColor;
	
	// Add Roughness output
	UMaterializeGraphNode_ChannelOutput* RoughnessOutput = Cast<UMaterializeGraphNode_ChannelOutput>(
		Graph->AddNode(UMaterializeGraphNode_ChannelOutput::StaticClass(), FVector2D(200.0f, 0.0f))
	);
	RoughnessOutput->OutputChannel = EMaterializeOutputChannel::Roughness;
	
	// Add Normal output
	UMaterializeGraphNode_ChannelOutput* NormalOutput = Cast<UMaterializeGraphNode_ChannelOutput>(
		Graph->AddNode(UMaterializeGraphNode_ChannelOutput::StaticClass(), FVector2D(400.0f, 0.0f))
	);
	NormalOutput->OutputChannel = EMaterializeOutputChannel::Normal;
	
	TestEqual(TEXT("Graph should have 3 output nodes"), Graph->Nodes.Num(), 3);
	
	// Verify each output has correct channel
	TestEqual(TEXT("First output should be BaseColor"), 
		BaseColorOutput->OutputChannel, EMaterializeOutputChannel::BaseColor);
	TestEqual(TEXT("Second output should be Roughness"), 
		RoughnessOutput->OutputChannel, EMaterializeOutputChannel::Roughness);
	TestEqual(TEXT("Third output should be Normal"), 
		NormalOutput->OutputChannel, EMaterializeOutputChannel::Normal);
	
	// Verify each has different expected pin types
	TestEqual(TEXT("BaseColor expects Color"), 
		BaseColorOutput->GetExpectedPinType(), EMaterializeOutputPinType::Color);
	TestEqual(TEXT("Roughness expects Scalar"), 
		RoughnessOutput->GetExpectedPinType(), EMaterializeOutputPinType::Scalar);
	TestEqual(TEXT("Normal expects Normal"), 
		NormalOutput->GetExpectedPinType(), EMaterializeOutputPinType::Normal);
	
	return true;
}
