#include "SCombatActionSegmentInspector.h"
#include "CombatActionEditorTypes.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Styling/StyleColors.h"

#define LOCTEXT_NAMESPACE "SCombatActionSegmentInspector"

void SCombatActionSegmentInspector::Construct(const FArguments& InArgs)
{
	SelectionContext = InArgs._SelectionContext;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(8.f, 6.f))
		[
			SNew(SVerticalBox)

			// Header
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.Text(this, &SCombatActionSegmentInspector::GetHeaderTitleText)
					.Font(FAppStyle::GetFontStyle("NormalFontBold"))
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.BorderBackgroundColor(this, &SCombatActionSegmentInspector::GetHeaderStatusBackgroundColor)
					.Padding(FMargin(6.f, 2.f))
					[
						SNew(STextBlock)
						.Text(this, &SCombatActionSegmentInspector::GetHeaderStatusText)
						.ColorAndOpacity(this, &SCombatActionSegmentInspector::GetHeaderStatusTextColor)
					]
					
				]
			]

			// Separator
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SSeparator)
			]

			// Switcher
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.MaxDesiredHeight(320.f)
				[
					SNew(SScrollBox)

					+SScrollBox::Slot()
					[
						SAssignNew(MainStateSwitcher, SWidgetSwitcher)

						+SWidgetSwitcher::Slot()
						[
							BuildAssetOverviewPage()
						]

						+SWidgetSwitcher::Slot()
						[
							BuildInvalidSelectionPage()
						]

						+SWidgetSwitcher::Slot()
						[
							BuildSelectedSegmentPage()
						]
					]
				]
			]
		]
	];
}

void SCombatActionSegmentInspector::Refresh()
{
	if (!MainStateSwitcher.IsValid())
	{
		return;
	}
	
	RefreshAssetOverview();

	EMainPage MainPage = ResolveMainPage();
	MainStateSwitcher->SetActiveWidgetIndex(static_cast<int32>(MainPage));

	if (MainPage == EMainPage::SelectedSegment && ConfigStateSwitcher.IsValid())
	{
		EConfigPage ConfigPage = ResolveConfigPage();
		ConfigStateSwitcher->SetActiveWidgetIndex(static_cast<int32>(ConfigPage));
	}
}

void SCombatActionSegmentInspector::RefreshAssetOverview()
{
	if (!AssetIssueList.IsValid() || !SelectionContext)
	{
		return;
	}

	AssetIssueList->ClearChildren();

	for (const auto& Issue : SelectionContext->AllIssues)
	{
		AssetIssueList->AddSlot()
		.AutoHeight()
		.Padding(0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(Issue.Message)
			.ColorAndOpacity(this, &SCombatActionSegmentInspector::GetValidationIssueColor, Issue.Severity)
			.AutoWrapText(true)
		];
	}
}

SCombatActionSegmentInspector::EMainPage SCombatActionSegmentInspector::ResolveMainPage() const
{
	if (!SelectionContext)
	{
		return EMainPage::AssetOverview;
	}
	
	EMainPage MainPage{EMainPage::AssetOverview};
	switch (SelectionContext->SelectionStatus)
	{
		case ECombatActionSegmentSelectionStatus::NoSelection:
			MainPage = EMainPage::AssetOverview;
			break;
		case ECombatActionSegmentSelectionStatus::NotifyNotFound:
		case ECombatActionSegmentSelectionStatus::UnsupportedNotifyType:
			MainPage = EMainPage::InvalidSelection;
			break;
		case ECombatActionSegmentSelectionStatus::Selected:
			MainPage = EMainPage::SelectedSegment;
		default:
			break;
	}
	
	return MainPage;
}

SCombatActionSegmentInspector::EConfigPage SCombatActionSegmentInspector::ResolveConfigPage() const
{
	if (!SelectionContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid SelectionContext in ResolveConfigPage"));
		return EConfigPage::MissingBinding;
	}
	
	switch (SelectionContext->ResolveStatus)
	{
		case ECombatActionSegmentResolveStatus::MissingBinding:
			return EConfigPage::MissingBinding;
		case ECombatActionSegmentResolveStatus::DuplicateBinding:
			return EConfigPage::DuplicateBinding;
		case ECombatActionSegmentResolveStatus::UnresolvedSpec:
		case ECombatActionSegmentResolveStatus::HasInvalidBinding:
			return EConfigPage::UnresolvedSpec;
		case ECombatActionSegmentResolveStatus::ResolvedSuccessfully:
			return EConfigPage::Resolved;
		default:
			checkNoEntry();
		return EConfigPage::MissingBinding;
	}
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildAssetOverviewPage()
{
	return SAssignNew(AssetIssueList, SVerticalBox);
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildInvalidSelectionPage()
{
	return SNew(SBox)
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetInvalidSelectionMessage)
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildSelectedSegmentPage()
{
	return
		SNew(SVerticalBox)

		// Warning
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildAttackDetectionDisabledBanner()
		]
		
		// SegmentSummary
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SExpandableArea)
			.InitiallyCollapsed(false)
			.AreaTitle(LOCTEXT("SegmentSummary", "Segment"))
			.AreaTitleFont(FAppStyle::Get().GetFontStyle("NormalFontBold"))
			.BodyContent()
			[
				SNew(SBox)
				.Padding(FMargin(8.f, 4.f, 8.f, 6.f))
				[
					BuildSegmentSummary()					
				]
			]
		]

		// Configuration Section
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SExpandableArea)
			.InitiallyCollapsed(false)
			.AreaTitle(LOCTEXT("SegmentConfiguration", "Configuration"))
			.AreaTitleFont(FAppStyle::Get().GetFontStyle("NormalFontBold"))
			.BodyContent()
			[
				SNew(SBox)
				.Padding(FMargin(8.f, 4.f, 8.f, 6.f))
				[
					BuildConfigurationSection()				
				]
			]
		]

		// Validation Section
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildValidationSection()
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildSegmentSummary()
{
	return
		SNew(SGridPanel)
		.FillColumn(1, 1.f)	// Column 1 gets the rest of width

// Name
	// Label
		+SGridPanel::Slot(0, 0)
		.Padding(FMargin(0.f, 2.f, 12.f, 2.f))
		[
			SNew(SBox)
			.WidthOverride(110.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SummaryName", "Name"))
			]
		]
	// Value
		+SGridPanel::Slot(1, 0)
		.Padding(FMargin(0.f, 2.f, 0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetSegmentNameText)
		]

// Trigger
	// Label
		+SGridPanel::Slot(0, 1)
		.Padding(FMargin(0.f, 2.f, 12.f, 2.f))
		[
			SNew(SBox)
			.WidthOverride(110.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SummaryTrigger", "Trigger"))
			]
		]
	// Value
		+SGridPanel::Slot(1, 1)
		.Padding(FMargin(0.f, 2.f, 0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetSegmentTriggerTypeText)
		]
		
//Time
	// Label
		+SGridPanel::Slot(0, 2)
		.Padding(FMargin(0.f, 2.f, 12.f, 2.f))
		[
			SNew(SBox)
			.WidthOverride(110.f)
			.Visibility(this, &SCombatActionSegmentInspector::GetInstantTimeVisibility)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SummaryTime", "Time"))
			]
		]
	// Value
		+SGridPanel::Slot(1, 2)
		.Padding(FMargin(0.f, 2.f, 0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetSegmentInstantTimeText)
			.Visibility(this, &SCombatActionSegmentInspector::GetInstantTimeVisibility)
		]

// Start
	// Label
		+SGridPanel::Slot(0, 3)
		.Padding(FMargin(0.f, 2.f, 12.f, 2.f))
		[
			SNew(SBox)
			.WidthOverride(110.f)
			.Visibility(this, &SCombatActionSegmentInspector::GetStateTimeVisibility)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SummaryStart", "Start"))
			]
		]
	// Value
		+SGridPanel::Slot(1, 3)
		.Padding(FMargin(0.f, 2.f, 0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetSegmentStartTimeText)
			.Visibility(this, &SCombatActionSegmentInspector::GetStateTimeVisibility)
		]

// End
	// Label
		+SGridPanel::Slot(0, 4)
		.Padding(FMargin(0.f, 2.f, 12.f, 2.f))
		[
			SNew(SBox)
			.WidthOverride(110.f)
			.Visibility(this, &SCombatActionSegmentInspector::GetStateTimeVisibility)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SummaryEnd", "End"))
			]
		]
	// Value
		+SGridPanel::Slot(1, 4)
		.Padding(FMargin(0.f, 2.f, 0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetSegmentEndTimeText)
			.Visibility(this, &SCombatActionSegmentInspector::GetStateTimeVisibility)
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildConfigurationSection()
{
	return
		SAssignNew(ConfigStateSwitcher, SWidgetSwitcher)
			
		+SWidgetSwitcher::Slot()
		[
			BuildMissingBindingConfigPage()
		]

		+SWidgetSwitcher::Slot()
		[
			BuildDuplicateBindingConfigPage()
		]

		+SWidgetSwitcher::Slot()
		[
			BuildUnresolvedBindingConfigPage()
		]

		+SWidgetSwitcher::Slot()
		[
			BuildResolvedBindingConfigPage()
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildValidationSection()
{
	FLinearColor ErrorBackground = FStyleColors::Error.GetSpecifiedColor();
	ErrorBackground.A = 0.1f;
	FLinearColor WarningBackground = FStyleColors::Warning.GetSpecifiedColor();
	WarningBackground.A = 0.1f;
	FSlateColor ErrorBackgroundColor = FSlateColor(ErrorBackground);
	FSlateColor WarningBackgroundColor = FSlateColor(WarningBackground);
	
	return 
		SNew(SExpandableArea)
		.InitiallyCollapsed(false)
		.AreaTitle(LOCTEXT("SegmentValidation", "Validation"))
		.AreaTitleFont(FAppStyle::Get().GetFontStyle("NormalFontBold"))
		.BodyContent()
		[
			SNew(SBox)
			.Padding(FMargin(8.f, 4.f, 8.f, 6.f))
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 2.f))
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(FMargin(6.f, 4.f))
					.Visibility(this, &SCombatActionSegmentInspector::GetNoAdditionalValidationIssuesVisibility)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NoAdditionalValidationIssues", "No additional validation issues."))
						.ColorAndOpacity(FStyleColors::Foreground)
						.AutoWrapText(true)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 2.f))
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.BorderBackgroundColor(ErrorBackgroundColor)
					.Padding(FMargin(6.f, 4.f))
					.Visibility(this, &SCombatActionSegmentInspector::GetValidationIssuesVisibility)
					[
						SNew(STextBlock)
						.Text(this, &SCombatActionSegmentInspector::GetSelectedItemValidationMessage)
						.ColorAndOpacity(FStyleColors::Error)
						.AutoWrapText(true)
					]
				]
			]
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildMissingBindingConfigPage()
{
	return
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetMissingBindingText)
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildDuplicateBindingConfigPage()
{
	return
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetDuplicateBindingText)
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SGridPanel)
			.FillColumn(1, 1.f)

			+SGridPanel::Slot(0, 0)
			[
				SNew(SBox)
				.WidthOverride(110.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DuplicateTitle", "Matches"))
					.AutoWrapText(true)
				]
			]

			+SGridPanel::Slot(1, 0)
			[
				SNew(STextBlock)
				.Text(this, &SCombatActionSegmentInspector::GetBindCountText)
			]
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildUnresolvedBindingConfigPage()
{
	return
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("UnresolvedConfig", "A matching Binding was found, but its Detection Spec could not be resolved."))
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildResolvedBindingConfigPage()
{
	return
		SNew(SGridPanel)
		.FillColumn(1, 1.f)
		
// Detection Mode
	// Label
		+SGridPanel::Slot(0, 0)
		.Padding(FMargin(0.f, 2.f, 12.f, 2.f))
		[
			SNew(SBox)
			.WidthOverride(110.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DetectionMode", "Detection Mode"))
			]
		]
	// Value
		+SGridPanel::Slot(1, 0)
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetDetectionModeText)
		]
	/*
// Trigger Mode
	// Label
		+SGridPanel::Slot(0, 1)
		.Padding(FMargin(0.f, 2.f, 12.f, 2.f))
		[
			SNew(SBox)
			.WidthOverride(110.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TriggerMode", "Trigger Mode"))
			]
		]
	// Value
		+SGridPanel::Slot(1, 1)
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetTriggerModeText)
		]
		*/
// Trace Channel
	// Label
		+SGridPanel::Slot(0, 1)
		.Padding(FMargin(0.f, 2.f, 12.f, 2.f))
		[
			SNew(SBox)
			.WidthOverride(110.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TraceChannel", "Trace Channel"))
			]
		]
	// Value
		+SGridPanel::Slot(1, 1)
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SCombatActionSegmentInspector::GetTraceChannelText)
		];
}

TSharedRef<SWidget> SCombatActionSegmentInspector::BuildAttackDetectionDisabledBanner()
{
	FLinearColor WarningBackground = FStyleColors::Warning.GetSpecifiedColor();
	WarningBackground.A = 0.15f;
	return 
		SNew(SBorder)
		.Visibility(this, &SCombatActionSegmentInspector::GetDetectionDisabledWarningVisibility)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FSlateColor(WarningBackground))
		.Padding(FMargin(8.f, 5.f))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DetectionDisabledWarning", "Attack Detection is disabled for this Combat Action."))
			.ColorAndOpacity(FStyleColors::Warning)
			.AutoWrapText(true)
		];
}

FText SCombatActionSegmentInspector::GetHeaderStatusText() const
{
	FText HeaderText;
	if (!SelectionContext)
	{
		return LOCTEXT("NoSelectionMessage", "No Selection.");
	}
	
	switch (SelectionContext->SelectionStatus)
	{
		case ECombatActionSegmentSelectionStatus::NoSelection:
			return LOCTEXT("NoSelectionMessage", "No Selection.");
		case ECombatActionSegmentSelectionStatus::NotifyNotFound:
			return LOCTEXT("NotifyNotFound", "Notify Not Found.");
		case ECombatActionSegmentSelectionStatus::UnsupportedNotifyType:
			return LOCTEXT("UnsupportedNotifyType", "UnSupported Notify Type.");
		case ECombatActionSegmentSelectionStatus::Selected:
			break;
		default:
			break;
	}

	switch (SelectionContext->ResolveStatus)
	{
		case ECombatActionSegmentResolveStatus::NotApplicable:
			HeaderText = LOCTEXT("NotApplicable", "Not Applicable.");		// todo
			break;
		case ECombatActionSegmentResolveStatus::MissingBinding:
			HeaderText = LOCTEXT("MissingBindingMessage", "Notify Missing Binding Segment.");
			break;
		case ECombatActionSegmentResolveStatus::DuplicateBinding:
			HeaderText = LOCTEXT("DuplicateBindingMessage", "Notify has Duplicate Relative Binding Segment.");
			break;
		case ECombatActionSegmentResolveStatus::UnresolvedSpec:
			HeaderText = LOCTEXT("UnresolvedSpecificationMessage", "Failed to Resolve Binding Segment.");
			break;
		case ECombatActionSegmentResolveStatus::HasInvalidBinding:
			HeaderText = LOCTEXT("HasInvalidBindingMessage", "Invalid Binding Segment.");
			break;
		case ECombatActionSegmentResolveStatus::ResolvedSuccessfully:
			HeaderText = LOCTEXT("Resolved", "Resolved.");
			break;
		default:
			checkNoEntry();
			break;
	}
	
	return HeaderText;
}

FText SCombatActionSegmentInspector::GetInvalidSelectionMessage() const
{
	if (!SelectionContext)
	{
		return LOCTEXT("InvalidSelectionStatus", "Invalid Selection");
	}

	switch (SelectionContext->SelectionStatus)
	{
		case ECombatActionSegmentSelectionStatus::NotifyNotFound:
			return LOCTEXT("NotifyNotFoundStatus", "Notify Not Found");
		case ECombatActionSegmentSelectionStatus::UnsupportedNotifyType:
			return LOCTEXT("UnsupportedNotifyTypeStatus", "Unsupported Notify Type");
		default:
			return LOCTEXT("UnknownSelectionStatus", "Unknown selection state.");	// 不应走到这里
	}
}

FText SCombatActionSegmentInspector::GetSegmentNameText() const
{
	return SelectionContext
	? FText::FromName(SelectionContext->SegmentName)
	: LOCTEXT("SegmentNameText", "Unknown");
}

FText SCombatActionSegmentInspector::GetSegmentTriggerTypeText() const
{
	if (!SelectionContext)
	{
		return LOCTEXT("SegmentNameText", "Unknown");
	}

	switch (SelectionContext->ItemType)
	{
		case ECombatActionTimelineItemType::InstantNotify:
			return LOCTEXT("InstantNotifyStatus", "Instant");
		case ECombatActionTimelineItemType::NotifyState:
			return LOCTEXT("NotifyStateStatus", "Continuous");
		default:
			return LOCTEXT("SegmentNameText", "Unknown");
	}
}

FText SCombatActionSegmentInspector::GetSegmentStartTimeText() const
{
	return FormatTime(SelectionContext ? SelectionContext->StartTime : 0.f);
}

FText SCombatActionSegmentInspector::GetSegmentEndTimeText() const
{
	return FormatTime(SelectionContext ? SelectionContext->EndTime : 0.f);
}

FText SCombatActionSegmentInspector::GetSegmentInstantTimeText() const
{
	return FormatTime(SelectionContext ? SelectionContext->StartTime : 0.f);
}

FText SCombatActionSegmentInspector::GetMissingBindingText() const
{
	FName SegmentName = SelectionContext ? SelectionContext->SegmentName : NAME_None;
	return FText::Format(LOCTEXT("SegmentConfiguration_MissingBinding", "No matching Attack Detection Config found for Segment {0}"),
		FText::FromName(SegmentName));
}

FText SCombatActionSegmentInspector::GetDuplicateBindingText() const
{
	FName SegmentName = SelectionContext ? SelectionContext->SegmentName : NAME_None;
	return FText::Format(LOCTEXT("SegmentConfiguration_DuplicateBinding", "Multiple Attack Detection Config entries match Segment {0}"),
		FText::FromName(SegmentName));
}

FText SCombatActionSegmentInspector::GetBindCountText() const
{
	return FText::AsNumber(SelectionContext ? SelectionContext->BindingCount : 0);
}

FText SCombatActionSegmentInspector::GetDetectionModeText() const
{
	const FAttackDetectionSpec* Spec = GetResolvedSpec();
	if (!Spec)
	{
		return LOCTEXT("InvalidSpec", "Invalid Spec");
	}

	const UEnum* Enum = StaticEnum<EAttackDetectionMode>();
	if (!Enum)
	{
		return LOCTEXT("InvalidEnum", "Unknown");
	}
	
	FText Text = Enum->GetDisplayNameTextByValue(static_cast<int64>(Spec->DetectionMode));
	return Text.IsEmpty() ? LOCTEXT("InvalidText", "Unknown") : Text;
}

/*FText SCombatActionSegmentInspector::GetTriggerModeText() const
{
	const FAttackDetectionSpec* Spec = GetResolvedSpec();
	if (!Spec)
	{
		return LOCTEXT("InvalidSpec", "Invalid Spec");
	}

	const UEnum* Enum = StaticEnum<EAttackDetectionTriggerMode>();
	if (!Enum)
	{
		return LOCTEXT("InvalidTriggerMode", "Unknown");
	}
	
	FText Text = Enum->GetDisplayNameTextByValue(static_cast<int64>(Spec->TriggerMode));
	return Text.IsEmpty() ? LOCTEXT("InvalidText", "Unknown") : Text;
}*/

FText SCombatActionSegmentInspector::GetTraceChannelText() const
{
	const FAttackDetectionSpec* Spec = GetResolvedSpec();
	if (!Spec)
	{
		return LOCTEXT("InvalidSpec", "Invalid Spec");
	}

	const UEnum* Enum = StaticEnum<ECollisionChannel>();
	if (!Enum)
	{
		return LOCTEXT("InvalidTraceChannel", "Unknown");
	}

	FText Text = Enum->GetDisplayNameTextByValue(static_cast<int64>(Spec->TraceChannel.GetValue()));
	return Text.IsEmpty() ? LOCTEXT("InvalidText", "Unknown") : Text;
}

FText SCombatActionSegmentInspector::GetSelectedItemValidationMessage() const
{
	TArray<FText> IssueMessage;
	for (const auto& Issue : SelectionContext->Issues)
	{
		IssueMessage.Add(Issue.Message);
	}
	return FText::Join(FText::FromString(LINE_TERMINATOR), IssueMessage);
}

FText SCombatActionSegmentInspector::GetAssetOverviewText() const
{
	TArray<FText> IssueMessage;
	for (const auto& Issue : SelectionContext->AllIssues)
	{
		IssueMessage.Add(Issue.Message);
	}
	return FText::Join(FText::FromString(LINE_TERMINATOR), IssueMessage);
}

FText SCombatActionSegmentInspector::GetHeaderTitleText() const
{
	if (!SelectionContext || SelectionContext->SelectionStatus == ECombatActionSegmentSelectionStatus::NoSelection)
	{
		return FText(LOCTEXT("NoSelectionTitle", "AssetOverview"));
	} else
	{
		return FText(LOCTEXT("NoSelectionTitle", "Selected Attack Detection"));
	}
}

EVisibility SCombatActionSegmentInspector::GetInstantTimeVisibility() const
{
	if (!SelectionContext)
	{
		return EVisibility::Collapsed;
	}

	return SelectionContext->ItemType == ECombatActionTimelineItemType::InstantNotify
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

EVisibility SCombatActionSegmentInspector::GetStateTimeVisibility() const
{
	if (!SelectionContext)
	{
		return EVisibility::Collapsed;
	}
	return SelectionContext->ItemType == ECombatActionTimelineItemType::NotifyState
	? EVisibility::Visible
	: EVisibility::Collapsed;
}

EVisibility SCombatActionSegmentInspector::GetDetectionDisabledWarningVisibility() const
{
	return SelectionContext ? (SelectionContext->bEnableDetection ? EVisibility::Collapsed : EVisibility::Visible) : EVisibility::Collapsed;
}

EVisibility SCombatActionSegmentInspector::GetNoAdditionalValidationIssuesVisibility() const
{
	if (!SelectionContext)
	{
		return EVisibility::Collapsed;
	}
	return HasAnyDisplayedValidationIssue() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SCombatActionSegmentInspector::GetValidationIssuesVisibility() const
{
	if (!SelectionContext)
	{
		return EVisibility::Collapsed;
	}
	return HasAnyDisplayedValidationIssue() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCombatActionSegmentInspector::GetAssetOverviewVisibility() const
{
	if (!SelectionContext) {
		return EVisibility::Visible;
	}
	return SelectionContext->SelectionStatus == ECombatActionSegmentSelectionStatus::NoSelection
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

FSlateColor SCombatActionSegmentInspector::GetHeaderStatusTextColor() const
{
	return FSlateColor(GetHeaderStatusAccentColor());
}

FSlateColor SCombatActionSegmentInspector::GetHeaderStatusBackgroundColor() const
{
	FLinearColor Color = GetHeaderStatusAccentColor();
	Color.A = 0.15f;
	return FSlateColor(Color);
}

FLinearColor SCombatActionSegmentInspector::GetHeaderStatusAccentColor() const
{
	if (!SelectionContext)
	{
		return FStyleColors::Foreground.GetSpecifiedColor();
	}

	FLinearColor Color{FStyleColors::Foreground.GetSpecifiedColor()};
	switch (SelectionContext->SelectionStatus)
	{
		case ECombatActionSegmentSelectionStatus::NoSelection:
			return FStyleColors::Foreground.GetSpecifiedColor();
		case ECombatActionSegmentSelectionStatus::NotifyNotFound:
		case ECombatActionSegmentSelectionStatus::UnsupportedNotifyType:
			return FStyleColors::Warning.GetSpecifiedColor();
		default:
			break;
	}

	switch (SelectionContext->ResolveStatus)
	{
		case ECombatActionSegmentResolveStatus::NotApplicable:
		case ECombatActionSegmentResolveStatus::MissingBinding:
		case ECombatActionSegmentResolveStatus::DuplicateBinding:
		case ECombatActionSegmentResolveStatus::UnresolvedSpec:
		case ECombatActionSegmentResolveStatus::HasInvalidBinding:
			Color = FStyleColors::Error.GetSpecifiedColor();
			break;
		case ECombatActionSegmentResolveStatus::ResolvedSuccessfully:
			Color = FStyleColors::Success.GetSpecifiedColor();
			break;
		default:
			checkNoEntry();
			break;
	}
	return Color;
}

FSlateColor SCombatActionSegmentInspector::GetValidationIssueColor(ECombatActionValidationSeverity Severity) const
{
	switch (Severity)
	{
		case ECombatActionValidationSeverity::Warning:
			return FSlateColor(FStyleColors::Warning.GetSpecifiedColor());
		case ECombatActionValidationSeverity::Error:
			return FSlateColor(FStyleColors::Error.GetSpecifiedColor());
		default:
			return FSlateColor(FStyleColors::Foreground.GetSpecifiedColor());
	}
}

FText SCombatActionSegmentInspector::FormatTime(const float Time) const
{
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MinimumFractionalDigits = 2;
	FormattingOptions.MaximumFractionalDigits = 2;
	FormattingOptions.UseGrouping = false;

	return FText::Format(LOCTEXT("TimeSecondsFormat", "{0} s"), FText::AsNumber(Time, &FormattingOptions));
}

const FAttackDetectionSpec* SCombatActionSegmentInspector::GetResolvedSpec() const
{
	if (!SelectionContext)
	{
		return nullptr;
	}

	if (SelectionContext->ResolveStatus != ECombatActionSegmentResolveStatus::ResolvedSuccessfully)
	{
		return nullptr;
	}

	if (!SelectionContext->ResolvedSpec.IsSet())
	{
		return nullptr;
	}

	return &SelectionContext->ResolvedSpec.GetValue();
}

bool SCombatActionSegmentInspector::HasValidationFlag(const ECombatActionTimelineValidationFlags Flag) const
{
	return SelectionContext && EnumHasAnyFlags(SelectionContext->ValidationFlags, Flag);
}

EVisibility SCombatActionSegmentInspector::GetValidationFlagVisibility(const ECombatActionTimelineValidationFlags Flag) const
{
	return HasValidationFlag(Flag) ? EVisibility::Visible : EVisibility::Collapsed;
}

// Check Exclusive Validation Flag
bool SCombatActionSegmentInspector::HasAnyDisplayedValidationIssue() const
{
	if (!SelectionContext)
	{
		return false;
	}

	return  SelectionContext->ValidationFlags != ECombatActionTimelineValidationFlags::None;
}

#undef LOCTEXT_NAMESPACE
