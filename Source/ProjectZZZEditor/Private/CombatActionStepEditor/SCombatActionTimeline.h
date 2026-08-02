#pragma once
#include "SCombatActionTimelineTrack.h"

struct FCombatActionTimelineItem;

class SCombatActionTimeline : public SCompoundWidget 
{
public:
	SLATE_BEGIN_ARGS(SCombatActionTimeline) {}

		SLATE_ARGUMENT(float, HeaderWidth)
		
		SLATE_EVENT(FOnClicked, OnPlayPauseClick)

		SLATE_EVENT(FOnClicked, OnResetClick)

		SLATE_ATTRIBUTE(float, NormalizedPosition)

		SLATE_EVENT(FOnFloatValueChanged, OnScrubValueChanged)

		SLATE_ATTRIBUTE(FText, PreviewTimeText)

		// Track
		SLATE_ARGUMENT(const TArray<FCombatActionTimelineItem>*, TimelineItems)
		
		SLATE_ATTRIBUTE(float, MontageLength)

		SLATE_ATTRIBUTE(float, CurrentTime)

		SLATE_ATTRIBUTE(TOptional<FGuid>, SelectedGuid)

		SLATE_EVENT(FOnCombatTimelineItemSelected, OnItemSelected)

		SLATE_EVENT(FOnCombatTimelineItemDragged, OnItemDragged)

		SLATE_EVENT(FOnCombatTimelineItemDragFinished, OnItemDragFinished)
		
	SLATE_END_ARGS()

	SCombatActionTimeline() {}
	
	void Construct(const FArguments& Args);

	void RefreshTrack();
	
private:
	
	TSharedPtr<SCombatActionTimelineTrack> TimelineTrack{nullptr};
	
};
