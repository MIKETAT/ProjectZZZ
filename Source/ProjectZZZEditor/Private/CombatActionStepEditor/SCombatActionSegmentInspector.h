#pragma once
#include "CombatActionTimelineItem.h"

enum class ECombatActionValidationSeverity : uint8;
struct FAttackDetectionSpec;
class SWidgetSwitcher;
struct FCombatActionSegmentSelectionContext;


class SCombatActionSegmentInspector : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCombatActionSegmentInspector) {}

		SLATE_ARGUMENT(const FCombatActionSegmentSelectionContext*, SelectionContext)
		
	SLATE_END_ARGS()
	
	SCombatActionSegmentInspector() : SelectionContext(nullptr) {}

	void Construct(const FArguments& InArgs);

	
	//virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	void Refresh();

	void RefreshAssetOverview();
	
private:
	enum class EMainPage : uint8
	{
		AssetOverview = 0,
		InvalidSelection = 1,
		SelectedSegment = 2
	};

	enum class EConfigPage : uint8
	{
		MissingBinding = 0,
		DuplicateBinding = 1,
		UnresolvedSpec = 2,
		Resolved = 3,
	};

	EMainPage ResolveMainPage() const;

	EConfigPage ResolveConfigPage() const;

	TSharedRef<SWidget> BuildAssetOverviewPage();

	TSharedRef<SWidget> BuildInvalidSelectionPage();

	TSharedRef<SWidget> BuildSelectedSegmentPage();

	TSharedRef<SWidget> BuildSegmentSummary();

	TSharedRef<SWidget> BuildConfigurationSection();
	
	TSharedRef<SWidget> BuildValidationSection();

	TSharedRef<SWidget> BuildMissingBindingConfigPage();

	TSharedRef<SWidget> BuildDuplicateBindingConfigPage();

	TSharedRef<SWidget> BuildUnresolvedBindingConfigPage();

	TSharedRef<SWidget> BuildResolvedBindingConfigPage();

	TSharedRef<SWidget> BuildAttackDetectionDisabledBanner();
	
// Texts
	FText GetHeaderStatusText() const;
	
	FText GetInvalidSelectionMessage() const;

	FText GetSegmentNameText() const;

	FText GetSegmentTriggerTypeText() const;

	FText GetSegmentStartTimeText() const;

	FText GetSegmentEndTimeText() const;

	FText GetSegmentInstantTimeText() const;

	FText GetMissingBindingText() const;

	FText GetDuplicateBindingText() const;

	FText GetBindCountText() const;

	FText GetDetectionModeText() const;

	//FText GetTriggerModeText() const;

	FText GetTraceChannelText() const;

	FText GetSelectedItemValidationMessage() const;

	FText GetAssetOverviewText() const;

	FText GetHeaderTitleText() const;

// Visibility
	EVisibility GetInstantTimeVisibility() const;

	EVisibility GetStateTimeVisibility() const;

	EVisibility GetDetectionDisabledWarningVisibility() const;

	EVisibility GetNoAdditionalValidationIssuesVisibility() const;

	EVisibility GetValidationIssuesVisibility() const;

	EVisibility GetAssetOverviewVisibility() const;

// Color
	FSlateColor GetHeaderStatusTextColor() const;

	FSlateColor GetHeaderStatusBackgroundColor() const;

	FLinearColor GetHeaderStatusAccentColor() const;

	FSlateColor GetValidationIssueColor(ECombatActionValidationSeverity Severity) const;
	
	// Helper Function
	FText FormatTime(const float Time) const;
	
	const FAttackDetectionSpec* GetResolvedSpec() const;

	bool HasValidationFlag(const ECombatActionTimelineValidationFlags Flag) const;
	
	EVisibility GetValidationFlagVisibility(const ECombatActionTimelineValidationFlags Flag) const;

	bool HasAnyDisplayedValidationIssue() const;
private:
	const FCombatActionSegmentSelectionContext* SelectionContext{nullptr};

	TSharedPtr<SWidgetSwitcher> MainStateSwitcher;

	TSharedPtr<SWidgetSwitcher> ConfigStateSwitcher;

	TSharedPtr<SVerticalBox> AssetIssueList;
};
