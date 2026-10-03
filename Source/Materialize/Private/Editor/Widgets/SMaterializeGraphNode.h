#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UMaterializeGraphNode;
class FMaterializePreviewViewport;

/**
 * Custom visual representation of Materialize graph nodes
 * Uses Epic Material Editor patterns for live GPU preview
 */
class SMaterializeGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SMaterializeGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UMaterializeGraphNode* InNode);

	//~ Begin SGraphNode Interface
	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	virtual const FSlateBrush* GetNodeBodyBrush() const override;
	//~ End SGraphNode Interface

protected:
	/** Create the preview thumbnail area with live viewport */
	TSharedRef<SWidget> CreatePreviewWidget();

	/** Create the title bar right widget (preview toggle) */
	TSharedRef<SWidget> CreateTitleRightWidget();

	/** Get the header background color based on node type */
	FLinearColor GetHeaderColor() const;

	/** Preview visibility toggle */
	void OnPreviewToggleChanged(const ECheckBoxState NewCheckedState);
	ECheckBoxState IsPreviewChecked() const;
	const FSlateBrush* GetPreviewArrow() const;

	/** Toggle preview button handler */
	FReply OnTogglePreviewClicked();

	/** Preview visibility getter */
	EVisibility GetPreviewVisibility() const;

	/** Pin containers */
	TSharedPtr<SVerticalBox> LeftNodeBox;
	TSharedPtr<SVerticalBox> RightNodeBox;

	/** Live preview viewport (renders node output on GPU) */
	TSharedPtr<FMaterializePreviewViewport> PreviewViewport;

	/** Cached reference to the Materialize node */
	UMaterializeGraphNode* KSampleNode = nullptr;
};

