#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class UMaterializeGraphNode;

/**
 * Factory to create custom visual widgets for Materialize graph nodes
 */
struct FMaterializeGraphNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* InNode) const override;
};
