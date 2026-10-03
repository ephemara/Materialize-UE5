#include "Editor/MaterializeGraphFactory.h"
#include "Graph/MaterializeGraph.h"
#include "Graph/MaterializeGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"

#define LOCTEXT_NAMESPACE "KSampleGraphFactory"

UMaterializeGraphFactory::UMaterializeGraphFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UMaterializeGraph::StaticClass();
}

UObject* UMaterializeGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UMaterializeGraph* NewGraph = NewObject<UMaterializeGraph>(InParent, Class, Name, Flags | RF_Transactional);

	// Create a graph to serve as the root
	if (NewGraph)
	{
		NewGraph->Schema = UMaterializeGraphSchema::StaticClass();
		
		// Initialize the graph with a default schema
		// NewGraph->GetSchema()->CreateDefaultNodesForGraph(*NewGraph);
	}

	return NewGraph;
}

FText UMaterializeGraphFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Materialize Graph");
}

#undef LOCTEXT_NAMESPACE
