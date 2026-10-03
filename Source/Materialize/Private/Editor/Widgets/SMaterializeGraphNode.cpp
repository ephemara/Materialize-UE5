#include "Editor/Widgets/SMaterializeGraphNode.h"
#include "Editor/Preview/MaterializePreviewViewport.h"
#include "Graph/Nodes/MaterializeGraphNode.h"
#include "Graph/Nodes/MaterializeGraphNode_Output.h"
#include "SGraphPin.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SViewport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "GraphEditorSettings.h"
#include "SNodePanel.h"

// Preview constants (matching Material Editor)
static const float KSamplePreviewSize = 96.0f;
static const float KSamplePreviewPadding = 5.0f;

void SMaterializeGraphNode::Construct(const FArguments& InArgs, UMaterializeGraphNode* InNode)
{
	this->GraphNode = InNode;
	this->KSampleNode = InNode;
	this->SetCursor(EMouseCursor::CardinalCross);
	
	// Bind content scale
	ContentScale.Bind(this, &SGraphNode::GetContentScale);
}

void SMaterializeGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	LeftNodeBox.Reset();
	RightNodeBox.Reset();
	PreviewViewport.Reset();

	// Create the node content area which creates LeftNodeBox and RightNodeBox
	TSharedRef<SWidget> NodeContent = CreateNodeContentArea();

	GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0)
			.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.1f, 0.95f))
			[
				SNew(SVerticalBox)
				// Title Bar
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Graph.StateNode.ColorSpill"))
					.Padding(FMargin(10, 4))
					.BorderBackgroundColor(GetHeaderColor())
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(GraphNode->GetNodeTitle(ENodeTitleType::FullTitle))
							.TextStyle(FAppStyle::Get(), "Graph.StateNode.NodeTitle")
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNullWidget::NullWidget
						]
						// Title right widget (preview toggle)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							CreateTitleRightWidget()
						]
					]
				]
				// Content Area (pins)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5.0f)
				[
					NodeContent
				]
			]
		];

	// Now create pin widgets
	CreatePinWidgets();
}

TSharedRef<SWidget> SMaterializeGraphNode::CreateNodeContentArea()
{
	// Create pin containers first
	SAssignNew(LeftNodeBox, SVerticalBox);
	SAssignNew(RightNodeBox, SVerticalBox);

	// Count pins to determine preview placement (Material Editor pattern)
	int32 LeftPinCount = 0;
	int32 RightPinCount = 0;
	if (KSampleNode)
	{
		for (const UEdGraphPin* Pin : KSampleNode->Pins)
		{
			if (!Pin->bHidden)
			{
				if (Pin->Direction == EGPD_Input)
					LeftPinCount++;
				else
					RightPinCount++;
			}
		}
	}

	// Build the content area
	TSharedRef<SHorizontalBox> ContentBox = SNew(SHorizontalBox)
		// Input Pins
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			LeftNodeBox.ToSharedRef()
		];

	// Add preview on the side with fewer pins (or center if equal)
	if (LeftPinCount < RightPinCount && LeftPinCount == 0)
	{
		// Preview on left
		ContentBox->AddSlot()
			.AutoWidth()
			.Padding(KSamplePreviewPadding, 0.0f)
			[
				CreatePreviewWidget()
			];
	}
	else if (RightPinCount < LeftPinCount && RightPinCount == 0)
	{
		// Will add preview on right after output pins
	}
	else
	{
		// Preview in center
		ContentBox->AddSlot()
			.HAlign(HAlign_Center)
			.FillWidth(1.0f)
			.Padding(KSamplePreviewPadding, 0.0f)
			[
				CreatePreviewWidget()
			];
	}

	// Output Pins
	ContentBox->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		[
			RightNodeBox.ToSharedRef()
		];

	// If preview goes on right side
	if (RightPinCount < LeftPinCount && RightPinCount == 0)
	{
		ContentBox->AddSlot()
			.AutoWidth()
			.Padding(KSamplePreviewPadding, 0.0f)
			[
				CreatePreviewWidget()
			];
	}

	return ContentBox;
}

TSharedRef<SWidget> SMaterializeGraphNode::CreatePreviewWidget()
{
	if (!KSampleNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("Materialize Preview Widget: KSampleNode is null!"));
		return SNullWidget::NullWidget;
	}

	if (!KSampleNode->ShouldShowPreview())
	{
		UE_LOG(LogTemp, Log, TEXT("Materialize Preview Widget: Node %s - ShouldShowPreview=false (bEnablePreview=%d, bPreviewCollapsed=%d)"),
			*KSampleNode->GetName(), KSampleNode->bEnablePreview, KSampleNode->bPreviewCollapsed);
		return SNullWidget::NullWidget;
	}

	UE_LOG(LogTemp, Log, TEXT("Materialize Preview Widget: Creating viewport for node %s"), *KSampleNode->GetName());

	// Create the viewport widget for live GPU preview
	TSharedPtr<SViewport> ViewportWidget = 
		SNew(SViewport)
		.RenderDirectlyToWindow(true)
		.EnableGammaCorrection(false);

	// Create and assign the preview viewport interface
	PreviewViewport = MakeShareable(new FMaterializePreviewViewport(KSampleNode));
	ViewportWidget->SetViewportInterface(PreviewViewport.ToSharedRef());

	return SNew(SBox)
		.WidthOverride(KSamplePreviewSize)
		.HeightOverride(KSamplePreviewSize)
		.Visibility(this, &SMaterializeGraphNode::GetPreviewVisibility)
		[
			SNew(SBorder)
			.Padding(KSamplePreviewPadding)
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					ViewportWidget.ToSharedRef()
				]
				// Fallback text when no preview texture
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Preview")))
					.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 0.5f))
					.Visibility_Lambda([this]()
					{
						return (KSampleNode && KSampleNode->GetPreviewTexture()) 
							? EVisibility::Collapsed 
							: EVisibility::Visible;
					})
				]
			]
		];
}

TSharedRef<SWidget> SMaterializeGraphNode::CreateTitleRightWidget()
{
	if (!KSampleNode || !KSampleNode->bEnablePreview)
	{
		return SNullWidget::NullWidget;
	}

	return SNew(SHorizontalBox)
		// Preview toggle button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
			.OnClicked(this, &SMaterializeGraphNode::OnTogglePreviewClicked)
			.Cursor(EMouseCursor::Default)
			.ToolTipText(FText::FromString(TEXT("Toggle Preview")))
			[
				SNew(SImage)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Image(FAppStyle::GetBrush(TEXT("Icons.Preview")))
			]
		]
		// Collapse/Expand checkbox
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SMaterializeGraphNode::OnPreviewToggleChanged)
			.IsChecked(this, &SMaterializeGraphNode::IsPreviewChecked)
			.Cursor(EMouseCursor::Default)
			.Style(FAppStyle::Get(), "Graph.Node.AdvancedView")
			[
				SNew(SImage)
				.Image(this, &SMaterializeGraphNode::GetPreviewArrow)
			]
		];
}

void SMaterializeGraphNode::CreatePinWidgets()
{
	if (!KSampleNode) return;

	for (UEdGraphPin* Pin : KSampleNode->Pins)
	{
		if (!Pin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SGraphNode::CreatePinWidget(Pin);
			if (NewPin.IsValid())
			{
				AddPin(NewPin.ToSharedRef());
			}
		}
	}
}

void SMaterializeGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bIsInput = PinObj && PinObj->Direction == EGPD_Input;
	
	if (bIsInput)
	{
		LeftNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else
	{
		RightNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				PinToAdd
			];
		OutputPins.Add(PinToAdd);
	}
}

FLinearColor SMaterializeGraphNode::GetHeaderColor() const
{
	if (KSampleNode)
	{
		return KSampleNode->GetNodeTitleColor();
	}
	return FLinearColor(0.2f, 0.2f, 0.2f);
}

const FSlateBrush* SMaterializeGraphNode::GetNodeBodyBrush() const
{
	return FAppStyle::GetBrush("Graph.StateNode.Body");
}

void SMaterializeGraphNode::OnPreviewToggleChanged(const ECheckBoxState NewCheckedState)
{
	if (KSampleNode)
	{
		const bool bCollapsed = (NewCheckedState != ECheckBoxState::Checked);
		if (KSampleNode->bPreviewCollapsed != bCollapsed)
		{
			KSampleNode->bPreviewCollapsed = bCollapsed;
			UpdateGraphNode();
		}
	}
}

ECheckBoxState SMaterializeGraphNode::IsPreviewChecked() const
{
	return (KSampleNode && !KSampleNode->bPreviewCollapsed) 
		? ECheckBoxState::Checked 
		: ECheckBoxState::Unchecked;
}

const FSlateBrush* SMaterializeGraphNode::GetPreviewArrow() const
{
	const bool bCollapsed = KSampleNode ? KSampleNode->bPreviewCollapsed : true;
	return FAppStyle::GetBrush(bCollapsed ? TEXT("Icons.ChevronDown") : TEXT("Icons.ChevronUp"));
}

FReply SMaterializeGraphNode::OnTogglePreviewClicked()
{
	if (KSampleNode)
	{
		KSampleNode->TogglePreviewCollapsed();
		UpdateGraphNode();
	}
	return FReply::Handled();
}

EVisibility SMaterializeGraphNode::GetPreviewVisibility() const
{
	return (KSampleNode && KSampleNode->ShouldShowPreview()) 
		? EVisibility::Visible 
		: EVisibility::Collapsed;
}
