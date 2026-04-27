#include "Utility/KismetCustomTraceUtils.h"

#if ENABLE_DRAW_DEBUG

void DrawDebugCapsuleTraceMulti_WithOrientation(const UWorld* World, const FVector& Start, const FVector& End,
	const FQuat& Orientation, float Radius, float HalfHeight, EDrawDebugTrace::Type DrawDebugType, bool bHit,
	const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		if (bHit && OutHits.Last().bBlockingHit)
		{
			const FVector BlockingHitPoint = OutHits.Last().Location;
			DrawDebugCapsule(World, Start, HalfHeight, Radius, Orientation, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugCapsule(World, BlockingHitPoint, HalfHeight, Radius, Orientation, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugLine(World, Start, BlockingHitPoint, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugCapsule(World, End, HalfHeight, Radius, Orientation, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugLine(World, BlockingHitPoint, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
		} else
		{
			DrawDebugCapsule(World, Start, HalfHeight, Radius, Orientation, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugCapsule(World, End, HalfHeight, Radius, Orientation, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}

		for (int32 HitIndex = 0; HitIndex < OutHits.Num(); ++HitIndex)
		{
			const FHitResult& Hit = OutHits[HitIndex];
			DrawDebugPoint(World, Hit.ImpactPoint, KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE,
				(Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)), bPersistent, LifeTime);
		}
	}
}

#endif