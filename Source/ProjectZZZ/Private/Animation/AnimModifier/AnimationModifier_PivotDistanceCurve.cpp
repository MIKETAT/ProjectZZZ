#include "Animation/AnimModifier/AnimationModifier_PivotDistanceCurve.h"

#include "Animation/AnimSequence.h"
#include "AnimationBlueprintLibrary.h"
#include "EngineLogs.h"

namespace PivotArcDistanceCurve
{
	struct FSample
	{
		float Time = 0.f;
		float CumulativeDistance = 0.f;
	};

	static void AddUniqueSortedTime(TArray<float>& Times, const float Time, const float Tolerance = KINDA_SMALL_NUMBER)
	{
		for (const float ExistingTime : Times)
		{
			if (FMath::IsNearlyEqual(ExistingTime, Time, Tolerance))
			{
				return;
			}
		}

		Times.Add(Time);
		Times.Sort();
	}
}

void UAnimationModifier_PivotDistanceCurve::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnApply_Implementation(AnimationSequence);

	if (!AnimationSequence)
	{
		UE_LOG(LogAnimation, Error, TEXT("PivotArcDistanceCurve failed. Reason: Invalid Animation."));
		return;
	}

	if (!AnimationSequence->HasRootMotion())
	{
		UE_LOG(LogAnimation, Error,
			TEXT("PivotArcDistanceCurve failed. Reason: Root motion is disabled on animation [%s]."),
			*GetNameSafe(AnimationSequence));

		return;
	}

	if (CurveName.IsNone())
	{
		UE_LOG(LogAnimation, Error,
			TEXT("PivotArcDistanceCurve failed. Reason: CurveName is None. Animation=[%s]."),
			*GetNameSafe(AnimationSequence));

		return;
	}

	const float AnimLength = AnimationSequence->GetPlayLength();

	if (AnimLength <= 0.f)
	{
		UE_LOG(LogAnimation, Error,
			TEXT("PivotArcDistanceCurve failed. Reason: Invalid animation length. Animation=[%s]."),
			*GetNameSafe(AnimationSequence));

		return;
	}

	float PivotTime = 0.f;

	if (PivotPointDetectionMode == EPivotPointDetectionMode::ManualTime)
	{
		PivotTime = FMath::Clamp(ManualPivotTime, 0.f, AnimLength);
	}
	else
	{
		PivotTime = FindPivotTimeByMinRootMotionSpeed(AnimationSequence);
	}

	if (bRemoveExistingCurve &&
		UAnimationBlueprintLibrary::DoesCurveExist(AnimationSequence, CurveName, ERawCurveTrackTypes::RCT_Float))
	{
		const bool bRemoveNameFromSkeleton = false;
		UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, CurveName, bRemoveNameFromSkeleton);
	}

	const bool bMetaDataCurve = false;
	UAnimationBlueprintLibrary::AddCurve(
		AnimationSequence,
		CurveName,
		ERawCurveTrackTypes::RCT_Float,
		bMetaDataCurve);

	// Build sample times.
	TArray<float> Times;

	const float SampleInterval = 1.f / static_cast<float>(FMath::Max(1, SampleRate));
	const int32 NumSteps = FMath::CeilToInt(AnimLength / SampleInterval);

	Times.Reserve(NumSteps + 2);

	for (int32 Step = 0; Step <= NumSteps; ++Step)
	{
		const float Time = FMath::Min(Step * SampleInterval, AnimLength);
		PivotArcDistanceCurve::AddUniqueSortedTime(Times, Time);
	}

	// Ensure the pivot point is an exact curve key.
	PivotArcDistanceCurve::AddUniqueSortedTime(Times, PivotTime);

	// Accumulate root-motion path distance.
	TArray<PivotArcDistanceCurve::FSample> Samples;
	Samples.Reserve(Times.Num());

	float CumulativeDistance = 0.f;

	for (int32 Index = 0; Index < Times.Num(); ++Index)
	{
		const float Time = Times[Index];

		if (Index > 0)
		{
			const float PrevTime = Times[Index - 1];

			const FVector RootMotionDelta =
				AnimationSequence
					->ExtractRootMotionFromRange(PrevTime, Time, FAnimExtractContext())
					.GetTranslation();

			CumulativeDistance += CalculateMagnitude(RootMotionDelta);
		}

		PivotArcDistanceCurve::FSample Sample;
		Sample.Time = Time;
		Sample.CumulativeDistance = CumulativeDistance;

		Samples.Add(Sample);
	}

	// Find cumulative distance at pivot time.
	float PivotCumulativeDistance = 0.f;
	bool bFoundPivotSample = false;

	for (const PivotArcDistanceCurve::FSample& Sample : Samples)
	{
		if (FMath::IsNearlyEqual(Sample.Time, PivotTime, KINDA_SMALL_NUMBER))
		{
			PivotCumulativeDistance = Sample.CumulativeDistance;
			bFoundPivotSample = true;
			break;
		}
	}

	if (!bFoundPivotSample)
	{
		UE_LOG(LogAnimation, Error,
			TEXT("PivotArcDistanceCurve failed. Reason: Pivot sample not found. Animation=[%s], PivotTime=%.4f."),
			*GetNameSafe(AnimationSequence),
			PivotTime);

		return;
	}

	for (const PivotArcDistanceCurve::FSample& Sample : Samples)
	{
		const float CurveValue = Sample.CumulativeDistance - PivotCumulativeDistance;

		UAnimationBlueprintLibrary::AddFloatCurveKey(
			AnimationSequence,
			CurveName,
			Sample.Time,
			CurveValue);
	}

	UE_LOG(LogAnimation, Display,
		TEXT("PivotArcDistanceCurve generated curve [%s] for animation [%s]. PivotTime=%.4f, PivotDistance=%.2f, TotalDistance=%.2f"),
		*CurveName.ToString(),
		*GetNameSafe(AnimationSequence),
		PivotTime,
		PivotCumulativeDistance,
		Samples.Num() > 0 ? Samples.Last().CumulativeDistance : 0.f);

	AnimationSequence->MarkPackageDirty();
}

void UAnimationModifier_PivotDistanceCurve::OnRevert_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnRevert_Implementation(AnimationSequence);

	if (!AnimationSequence || CurveName.IsNone())
	{
		return;
	}

	if (UAnimationBlueprintLibrary::DoesCurveExist(AnimationSequence, CurveName, ERawCurveTrackTypes::RCT_Float))
	{
		const bool bRemoveNameFromSkeleton = false;
		UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, CurveName, bRemoveNameFromSkeleton);
		AnimationSequence->MarkPackageDirty();
	}
}

float UAnimationModifier_PivotDistanceCurve::FindPivotTimeByMinRootMotionSpeed(UAnimSequence* AnimationSequence) const
{
	if (!AnimationSequence)
	{
		return 0.f;
	}

	const float AnimLength = AnimationSequence->GetPlayLength();

	if (AnimLength <= 0.f)
	{
		return 0.f;
	}

	const float SearchStartTime = AnimLength * FMath::Clamp(SearchStartRatio, 0.f, 0.49f);
	const float SearchEndTime = AnimLength * FMath::Clamp(SearchEndRatio, 0.51f, 1.f);

	const float SearchInterval =
		1.f / static_cast<float>(FMath::Max(1, MinSpeedSearchSampleRate));

	float BestTime = SearchStartTime;
	float BestSpeedSq = TNumericLimits<float>::Max();

	for (float Time = SearchStartTime; Time <= SearchEndTime; Time += SearchInterval)
	{
		const float StartTime = Time;
		const float EndTime = FMath::Min(Time + SearchInterval, AnimLength);

		if (EndTime <= StartTime)
		{
			continue;
		}

		const FVector RootMotionDelta =
			AnimationSequence
				->ExtractRootMotionFromRange(StartTime, EndTime, FAnimExtractContext())
				.GetTranslation();

		const float DeltaTime = EndTime - StartTime;
		const float SpeedSq = CalculateMagnitudeSq(RootMotionDelta) / FMath::Square(DeltaTime);

		if (SpeedSq < BestSpeedSq)
		{
			BestSpeedSq = SpeedSq;
			BestTime = StartTime;
		}
	}

	return FMath::Clamp(BestTime, 0.f, AnimLength);
}

float UAnimationModifier_PivotDistanceCurve::CalculateMagnitude(const FVector& Vector) const
{
	switch (Axis)
	{
	case EPivotArcDistanceAxis::X:
		return FMath::Abs(Vector.X);

	case EPivotArcDistanceAxis::Y:
		return FMath::Abs(Vector.Y);

	case EPivotArcDistanceAxis::Z:
		return FMath::Abs(Vector.Z);

	case EPivotArcDistanceAxis::XY:
	case EPivotArcDistanceAxis::XZ:
	case EPivotArcDistanceAxis::YZ:
	case EPivotArcDistanceAxis::XYZ:
		return FMath::Sqrt(CalculateMagnitudeSq(Vector));

	default:
		checkNoEntry();
		return 0.f;
	}
}

float UAnimationModifier_PivotDistanceCurve::CalculateMagnitudeSq(const FVector& Vector) const
{
	switch (Axis)
	{
	case EPivotArcDistanceAxis::X:
		return FMath::Square(Vector.X);

	case EPivotArcDistanceAxis::Y:
		return FMath::Square(Vector.Y);

	case EPivotArcDistanceAxis::Z:
		return FMath::Square(Vector.Z);

	case EPivotArcDistanceAxis::XY:
		return Vector.X * Vector.X + Vector.Y * Vector.Y;

	case EPivotArcDistanceAxis::XZ:
		return Vector.X * Vector.X + Vector.Z * Vector.Z;

	case EPivotArcDistanceAxis::YZ:
		return Vector.Y * Vector.Y + Vector.Z * Vector.Z;

	case EPivotArcDistanceAxis::XYZ:
		return Vector.X * Vector.X + Vector.Y * Vector.Y + Vector.Z * Vector.Z;

	default:
		checkNoEntry();
		return 0.f;
	}
}