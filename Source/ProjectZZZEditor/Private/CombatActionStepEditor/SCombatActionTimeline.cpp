#include "SCombatActionTimeline.h"
#include "Widgets/Input/SSlider.h"

#define LOCTEXT_NAMESPACE "SCombatActionTimeline"

void SCombatActionTimeline::Construct(const FArguments& Args)
{
	ChildSlot
	[
		SNew(SGridPanel)
		.FillColumn(1, 1.f)

		// Button. Play/Pause/Reset
		+SGridPanel::Slot(0, 0)
		[
			SNew(SBox)
			.WidthOverride(Args._HeaderWidth)
			[
				SNew(SHorizontalBox)
				
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("PlayPauseButton", "Play / Pause"))
					.OnClicked(Args._OnPlayPauseClick)
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetButton", "Reset"))
					.OnClicked(Args._OnResetClick)
				]
			]
		]

		// Slider
		+SGridPanel::Slot(1, 0)
		[
			SNew(SSlider)
			.Value(Args._NormalizedPosition)
			.OnValueChanged(Args._OnScrubValueChanged)
			.IndentHandle(false)
		]

		// Text
		+SGridPanel::Slot(2, 0)
		[
			SNew(STextBlock)
			.Text(Args._PreviewTimeText)
		]

		// Title
		+SGridPanel::Slot(0, 1)
		[
			SNew(SBox)
			.WidthOverride(Args._HeaderWidth)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AttackDetectionTrack", "Attack Detection"))
			]
		]

		+SGridPanel::Slot(1, 1)
			[
				SAssignNew(TimelineTrack, SCombatActionTimelineTrack)
				.TimelineItems(Args._TimelineItems)
				.MontageLength(Args._MontageLength)
				.CurrentTime(Args._CurrentTime)
				.SelectedGuid(Args._SelectedGuid)
				.OnItemSelected(Args._OnItemSelected)
				.OnItemDragged(Args._OnItemDragged)
				.OnItemDragFinished(Args._OnItemDragFinished)
			]
	];
}

void SCombatActionTimeline::RefreshTrack()
{
	if (TimelineTrack.IsValid())
	{
		TimelineTrack->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

#undef LOCTEXT_NAMESPACE

/*
┌──────────────────┬─────────────────────────┬────────────┐
│ Button           │ Slider                  │ Text       │
├──────────────────┼─────────────────────────┼────────────┤
│ Attack Detection │ Timeline Track          │ Empty      │
└──────────────────┴─────────────────────────┴────────────┘
	   Column 0               Column 1             Column 2
*/