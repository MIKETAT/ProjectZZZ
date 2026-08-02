#pragma once
#include "CombatActionEditorTypes.h"
#include "CombatActionTimelineItem.h"

struct FCombatTimelineHitResult;


class SCombatActionTimelineTrack : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCombatActionTimelineTrack) : _TimelineItems(nullptr), _MontageLength(0.f), _CurrentTime(0.f),
		_SelectedGuid(TOptional<FGuid>()), _OnItemSelected() {}
		
		SLATE_ARGUMENT(const TArray<FCombatActionTimelineItem>*, TimelineItems)
			
		SLATE_ATTRIBUTE(float, MontageLength)

		SLATE_ATTRIBUTE(float, CurrentTime)

		SLATE_ATTRIBUTE(TOptional<FGuid>, SelectedGuid)

		SLATE_EVENT(FOnCombatTimelineItemSelected, OnItemSelected)

		SLATE_EVENT(FOnCombatTimelineItemDragged, OnItemDragged)

		SLATE_EVENT(FOnCombatTimelineItemDragFinished, OnItemDragFinished)
		
	SLATE_END_ARGS()
	
public:
	SCombatActionTimelineTrack();

	void Construct(const FArguments& Args);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	
private:
	float TimeToLocalX(const float Time, const float TrackWidth) const;

	float LocalXToTime(const float LocalX, const float TrackWidth) const;
	
	FSlateRect GetHandleRect(const FVector2f& Center) const;

	FSlateRect GetBodyRect(const FVector2f& StartPosition, const FVector2f& EndPosition) const;

	FCombatTimelineItemGeometry GetItemGeometry(const FCombatActionTimelineItem& Item, const FVector2D& TrackSize) const;
	
	FCombatTimelineItemGeometry GetInstantNotifyItemGeometry(const FCombatActionTimelineItem& Item, const FVector2D& TrackSize) const;

	FCombatTimelineItemGeometry GetStateNotifyItemGeometry(const FCombatActionTimelineItem& Item, const FVector2D& TrackSize) const;

	bool IsGeometryValid(const FCombatActionTimelineItem& Item, const FVector2D& TrackSize) const;

	void DrawRect(const FSlateRect& Rect, const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
					int32 LayerId, const FSlateBrush* Brush, const FLinearColor& Tint) const;

	void DrawHandle(const FSlateRect& Rect, const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
					int32 LayerId, const FLinearColor& Tint) const;
	
	FCombatTimelineHitResult HitTestItem(const FVector2D LocalMousePosition, const FVector2D TrackSize) const;

	bool IsInRect(const FVector2D LocalMousePosition, const FSlateRect& Rect) const;

	bool IsInCircle(const FVector2D LocalMousePosition, const FSlateRect& Bounds) const;

	void ResetDragStatus(bool bNotifyDragFinish);
	
private:
	const TArray<FCombatActionTimelineItem>* TimelineItems{nullptr};
	
	TAttribute<float> MontageLength;

	TAttribute<float> CurrentTime;

	TAttribute<TOptional<FGuid>> SelectedGuid;

	FOnCombatTimelineItemSelected OnItemSelected;

	FOnCombatTimelineItemDragged OnItemDragged;

	FOnCombatTimelineItemDragFinished OnItemDragFinished;

	const float HandleSize = 16.f;

	FCombatTimelineHitResult ActiveHitResult;

	FSlateRoundedBoxBrush HandleBrush{FLinearColor::White, FVector2f{HandleSize, HandleSize}};

	float MouseDownTimelineTime{0.f};

	FVector2D MouseDownScreenPosition{FVector2D::ZeroVector};

	bool bHasDragUpdate{false};
	
// Settings	
	const FLinearColor BackgroundColor{0.04f, 0.20f, 0.04f, 1.f};
	
	const FLinearColor NotifyBodyColor{0.1f, 0.1f, 0.8f, 1.f};

	const FLinearColor NotifyHandleColor{0.9f, 0.2f, 0.4f, 1.f};

	const FLinearColor SelectedHandleColor{0.6f, 0.9f, 0.4f, 1.f};
};
