// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetSystemLibrary.h"

static const float KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE = 16.f;


#if ENABLE_DRAW_DEBUG
PROJECTZZZ_API void DrawDebugCapsuleTraceMulti_WithOrientation(const UWorld* World, const FVector& Start, const FVector& End, const FQuat& Orientation, float Radius, float HalfHeight, EDrawDebugTrace::Type DrawDebugType, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);

#endif