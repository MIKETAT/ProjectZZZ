#include "SCombatActionTimelineTrack.h"

SCombatActionTimelineTrack::SCombatActionTimelineTrack() {}

void SCombatActionTimelineTrack::Construct(const FArguments& Args)
{
	TimelineItems = Args._TimelineItems;
	MontageLength = Args._MontageLength;
	CurrentTime = Args._CurrentTime;
	SelectedGuid = Args._SelectedGuid;
	OnItemSelected = Args._OnItemSelected;
	OnItemDragged = Args._OnItemDragged;
	OnItemDragFinished = Args._OnItemDragFinished;
}

int32 SCombatActionTimelineTrack::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FSlateBrush* Brush = FAppStyle::GetBrush("WhiteBrush");
	
	if (!Brush)
	{
		return LayerId;
	}
	
	// Draw BackGround
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Brush, ESlateDrawEffect::None, BackgroundColor);
	
	if (!TimelineItems || MontageLength.Get() <= UE_SMALL_NUMBER)
	{
		return LayerId;
	}
	
	// Draw NotifyState Body
	for (const FCombatActionTimelineItem& Item : *TimelineItems)
	{
		if (Item.ItemType != ECombatActionTimelineItemType::NotifyState)
		{
			continue;
		}

		FCombatTimelineItemGeometry Geometry = GetStateNotifyItemGeometry(Item, LocalSize);
		if (Geometry.GeometryType != ECombatTimelineItemGeometryType::Invalid)
		{
			DrawRect(Geometry.BodyRect, AllottedGeometry, OutDrawElements, LayerId + 1, Brush, NotifyBodyColor);	
		}
	}

	// Draw HandleRect
	for (const FCombatActionTimelineItem& Item : *TimelineItems)
	{
		FLinearColor Tint = NotifyHandleColor;
		TOptional<FGuid> OptionalGuid = SelectedGuid.Get();
		if (OptionalGuid.IsSet())
		{
			Tint = OptionalGuid.GetValue() == Item.NotifyGuid ? SelectedHandleColor : Tint;
		}
		
		FCombatTimelineItemGeometry Geometry;
		if (Item.ItemType == ECombatActionTimelineItemType::InstantNotify)
		{
			Geometry = GetInstantNotifyItemGeometry(Item, LocalSize);
		} else if (Item.ItemType == ECombatActionTimelineItemType::NotifyState)
		{
			Geometry = GetStateNotifyItemGeometry(Item, LocalSize);
		}

		if (Geometry.GeometryType == ECombatTimelineItemGeometryType::Invalid)
		{
			continue;	
		}
		
		// Draw StartHandleRect
		DrawHandle(Geometry.StartHandleRect, AllottedGeometry, OutDrawElements, LayerId + 2, Tint);

		// Draw EndHandleRect
		if (Item.ItemType == ECombatActionTimelineItemType::NotifyState)
		{
			DrawHandle(Geometry.EndHandleRect, AllottedGeometry, OutDrawElements, LayerId + 2, Tint);
		}
	}
	
	return LayerId + 2;
}

FVector2D SCombatActionTimelineTrack::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(100.f, 40.f);
}

FReply SCombatActionTimelineTrack::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Left Mouse Click
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	
	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	FCombatTimelineHitResult HitResult = HitTestItem(LocalPosition, MyGeometry.GetLocalSize());

	if (!HitResult.IsValid())
	{
		OnItemSelected.ExecuteIfBound(HitResult);
		ActiveHitResult = HitResult;
		return FReply::Unhandled();
	}
	
	OnItemSelected.ExecuteIfBound(HitResult);
	ActiveHitResult = HitResult;
	MouseDownTimelineTime = LocalXToTime(LocalPosition.X, MyGeometry.GetLocalSize().X);
	MouseDownScreenPosition = MouseEvent.GetScreenSpacePosition();
	bHasDragUpdate = false;
	
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SCombatActionTimelineTrack::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) || !HasMouseCapture() || !ActiveHitResult.IsValid())
	{
		return FReply::Unhandled();
	}

	const FVector2D ScreenSpacePosition = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(ScreenSpacePosition);
	const FVector2D DragDeltaDistance = ScreenSpacePosition - MouseDownScreenPosition;
	const float DragTriggerDistanceSquare = FSlateApplication::Get().GetDragTriggerDistanceSquared();
	
	const float CurrentTimelineTime = LocalXToTime(LocalPosition.X, MyGeometry.GetLocalSize().X);
	const float DragDeltaTime = CurrentTimelineTime - MouseDownTimelineTime;
	
	if (!bHasDragUpdate)
	{
		if (DragDeltaDistance.SizeSquared() < DragTriggerDistanceSquare)
		{
			return FReply::Handled();
		}
		bHasDragUpdate = true;
	}
	
	OnItemDragged.ExecuteIfBound(ActiveHitResult, DragDeltaTime);

	return FReply::Handled();
}

FReply SCombatActionTimelineTrack::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	
	ResetDragStatus(true);
	
	return SLeafWidget::OnMouseButtonUp(MyGeometry, MouseEvent).ReleaseMouseCapture();
}

void SCombatActionTimelineTrack::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	ResetDragStatus(true);
	SLeafWidget::OnMouseCaptureLost(CaptureLostEvent);
}

float SCombatActionTimelineTrack::TimeToLocalX(const float Time, const float TrackWidth) const
{
	const float Length = MontageLength.Get();
	if (Length <= 0 || TrackWidth <= 0)
	{
		return 0.f;
	}
	return FMath::Clamp(Time / Length, 0.f, 1.f) * TrackWidth;
}

float SCombatActionTimelineTrack::LocalXToTime(const float LocalX, const float TrackWidth) const
{
	const float Length = MontageLength.Get();
	if (LocalX < UE_SMALL_NUMBER || TrackWidth <= UE_SMALL_NUMBER)
	{
		return 0.f;
	}
	return FMath::Clamp(LocalX / TrackWidth, 0, 1.f) * Length;
}

FSlateRect SCombatActionTimelineTrack::GetHandleRect(const FVector2f& Center) const
{
	const float Left = Center.X - HandleSize * 0.5f;
	const float Top = Center.Y - HandleSize * 0.5f;
	const float Right = Center.X + HandleSize * 0.5f;
	const float Bottom = Center.Y + HandleSize * 0.5f;
	return FSlateRect(Left, Top, Right, Bottom);
}

FSlateRect SCombatActionTimelineTrack::GetBodyRect(const FVector2f& StartPosition, const FVector2f& EndPosition) const
{
	const float Left = StartPosition.X;
	const float Top = StartPosition.Y - HandleSize * 0.5f;
	const float Right = EndPosition.X;
	const float Bottom = EndPosition.Y + HandleSize * 0.5f;
	return FSlateRect(Left, Top, Right, Bottom);
}

FCombatTimelineItemGeometry SCombatActionTimelineTrack::GetItemGeometry(const FCombatActionTimelineItem& Item,
	const FVector2D& TrackSize) const
{
	FCombatTimelineItemGeometry Geometry;
	if (Item.ItemType == ECombatActionTimelineItemType::InstantNotify)
	{
		Geometry = GetInstantNotifyItemGeometry(Item, TrackSize);
	} else if (Item.ItemType == ECombatActionTimelineItemType::NotifyState)
	{
		Geometry = GetStateNotifyItemGeometry(Item, TrackSize);
	}
	return Geometry;
}

FCombatTimelineItemGeometry SCombatActionTimelineTrack::GetInstantNotifyItemGeometry(
	const FCombatActionTimelineItem& Item, const FVector2D& TrackSize) const
{
	const float TrackWidth = TrackSize.X;
	const float TrackHeight = TrackSize.Y;
	
	FCombatTimelineItemGeometry Geometry;
	if (!IsGeometryValid(Item, TrackSize) || Item.ItemType != ECombatActionTimelineItemType::InstantNotify)  // || TrackHeight <
	{
		Geometry.GeometryType = ECombatTimelineItemGeometryType::Invalid;
		return Geometry;
	}
	
	float CenterX = TimeToLocalX(Item.StartTime, TrackWidth);
	float CenterY = TrackHeight * 0.5f;

	Geometry.GeometryType = ECombatTimelineItemGeometryType::InstantNotifyGeometry;
	Geometry.StartHandleRect = GetHandleRect({CenterX, CenterY});
	Geometry.StartAnchorX = CenterX;
	
	return Geometry;
}

FCombatTimelineItemGeometry SCombatActionTimelineTrack::GetStateNotifyItemGeometry(
	const FCombatActionTimelineItem& Item, const FVector2D& TrackSize) const
{
	const float TrackWidth = TrackSize.X;
	const float TrackHeight = TrackSize.Y;

	FCombatTimelineItemGeometry Geometry;
	if (!IsGeometryValid(Item, TrackSize) || Item.ItemType != ECombatActionTimelineItemType::NotifyState)  // || TrackHeight <
	{
		Geometry.GeometryType = ECombatTimelineItemGeometryType::Invalid;
		return Geometry;
	}

	const float StartX = TimeToLocalX(Item.StartTime, TrackWidth);
	const float EndX = TimeToLocalX(Item.EndTime, TrackWidth);
	const float CenterY = TrackHeight * 0.5f;

	Geometry.GeometryType = ECombatTimelineItemGeometryType::StateNotifyGeometry;
	Geometry.StartHandleRect = GetHandleRect({StartX, CenterY});
	Geometry.EndHandleRect = GetHandleRect({EndX, CenterY});
	Geometry.BodyRect = GetBodyRect({StartX, CenterY}, {EndX, CenterY});
	Geometry.StartAnchorX = StartX;
	Geometry.EndAnchorX = EndX;
	return Geometry;
}

bool SCombatActionTimelineTrack::IsGeometryValid(const FCombatActionTimelineItem& Item,
	const FVector2D& TrackSize) const
{
	const float TrackWidth = TrackSize.X;
	const float TrackHeight = TrackSize.Y;
	if (TrackWidth <= UE_SMALL_NUMBER || TrackHeight <= UE_SMALL_NUMBER)
	{
		return false;
	}

	if (MontageLength.Get() <= UE_SMALL_NUMBER || Item.StartTime > Item.EndTime)
	{
		return false;
	}
	
	return true;
}

void SCombatActionTimelineTrack::DrawRect(const FSlateRect& Rect, const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FSlateBrush* Brush, const FLinearColor& Tint) const
{
	if (Rect.Left >= Rect.Right || Rect.Top >= Rect.Bottom || !Brush)
	{
		return;
	}

	const FVector2f Position{Rect.Left, Rect.Top};
	const FVector2f Size{Rect.Right - Rect.Left, Rect.Bottom - Rect.Top};
	
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(Size,FSlateLayoutTransform{Position}),
		Brush,
		ESlateDrawEffect::None,
		Tint
	);
}

void SCombatActionTimelineTrack::DrawHandle(const FSlateRect& Rect, const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FLinearColor& Tint) const
{
	if (Rect.Left >= Rect.Right || Rect.Top >= Rect.Bottom)
	{
		return;
	}

	const FVector2f Position{Rect.Left, Rect.Top};
	const FVector2f Size{Rect.Right - Rect.Left, Rect.Bottom - Rect.Top};

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(Size,FSlateLayoutTransform{Position}),
		&HandleBrush,
		ESlateDrawEffect::None,
		Tint
	);
	

}

FCombatTimelineHitResult SCombatActionTimelineTrack::HitTestItem(const FVector2D LocalMousePosition, const FVector2D TrackSize) const
{
	if (!TimelineItems)
	{
		return FCombatTimelineHitResult();
	}
	
	// Test NotifyState Start/End Handle Rect
	for (int32 Index = TimelineItems->Num() - 1; Index >= 0; --Index)
	{
		const FCombatActionTimelineItem& Item = (*TimelineItems)[Index];
		if (Item.ItemType != ECombatActionTimelineItemType::NotifyState)
		{
			continue;
		}
		FCombatTimelineItemGeometry Geometry = GetStateNotifyItemGeometry(Item, TrackSize);
		if (Geometry.GeometryType == ECombatTimelineItemGeometryType::Invalid)
		{
			continue;
		}
		
		if (IsInCircle(LocalMousePosition, Geometry.StartHandleRect))
		{
			return FCombatTimelineHitResult{Item.NotifyGuid, ECombatTimelineHitPart::StateStartHandle};
		} else if (IsInCircle(LocalMousePosition, Geometry.EndHandleRect))
		{
			return FCombatTimelineHitResult{Item.NotifyGuid, ECombatTimelineHitPart::StateEndHandle};
		}
	}

	// Test Instant Start Handle Rect
	for (int32 Index = TimelineItems->Num() - 1; Index >= 0; --Index)
	{
		const FCombatActionTimelineItem& Item = (*TimelineItems)[Index];
		if (Item.ItemType != ECombatActionTimelineItemType::InstantNotify)
		{
			continue;
		}
		FCombatTimelineItemGeometry Geometry = GetInstantNotifyItemGeometry(Item, TrackSize);
		if (Geometry.GeometryType == ECombatTimelineItemGeometryType::Invalid)
		{
			continue;
		}
		
		if (IsInCircle(LocalMousePosition, Geometry.StartHandleRect))
		{
			return FCombatTimelineHitResult{Item.NotifyGuid, ECombatTimelineHitPart::InstantHandle};
		}
	}

	// Test NotifyState Body Rect
	for (int32 Index = TimelineItems->Num() - 1; Index >= 0; --Index)
	{
		const FCombatActionTimelineItem& Item = (*TimelineItems)[Index];
		if (Item.ItemType != ECombatActionTimelineItemType::NotifyState)
		{
			continue;
		}
		FCombatTimelineItemGeometry Geometry = GetStateNotifyItemGeometry(Item, TrackSize);
		if (Geometry.GeometryType == ECombatTimelineItemGeometryType::Invalid)
		{
			continue;
		}
		
		if (IsInRect(LocalMousePosition, Geometry.BodyRect))
		{
			return FCombatTimelineHitResult{Item.NotifyGuid, ECombatTimelineHitPart::StateBody};
		}	
	}
	
	return FCombatTimelineHitResult{};
}

bool SCombatActionTimelineTrack::IsInRect(const FVector2D LocalMousePosition, const FSlateRect& Rect) const
{
	return LocalMousePosition.X >= Rect.Left
		&& LocalMousePosition.X <= Rect.Right
		&& LocalMousePosition.Y >= Rect.Top
		&& LocalMousePosition.Y <= Rect.Bottom;
}

bool SCombatActionTimelineTrack::IsInCircle(const FVector2D LocalMousePosition, const FSlateRect& Bounds) const
{
	const FVector2D Center{(Bounds.Left+Bounds.Right) * 0.5f, (Bounds.Top + Bounds.Bottom) * 0.5f};
	const float Radius = FMath::Min(Bounds.Right - Bounds.Left, Bounds.Bottom - Bounds.Top) * 0.5f;
	const FVector2D Distance = LocalMousePosition - Center;
	return Distance.SizeSquared() <= FMath::Square(Radius);	
}

void SCombatActionTimelineTrack::ResetDragStatus(bool bNotifyDragFinish)
{
	ActiveHitResult.Reset();
	
	if (bNotifyDragFinish && bHasDragUpdate)
	{
		OnItemDragFinished.ExecuteIfBound();
		bHasDragUpdate = false;
	}

	MouseDownTimelineTime = 0.f;
	MouseDownScreenPosition = FVector2D::ZeroVector;
}
