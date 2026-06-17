#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "AnimationModifier_PivotDistanceCurve.generated.h"

UENUM(BlueprintType)
enum class EPivotArcDistanceAxis : uint8
{
	X   UMETA(DisplayName = "X"),
	Y   UMETA(DisplayName = "Y"),
	Z   UMETA(DisplayName = "Z"),
	XY  UMETA(DisplayName = "XY"),
	XZ  UMETA(DisplayName = "XZ"),
	YZ  UMETA(DisplayName = "YZ"),
	XYZ UMETA(DisplayName = "XYZ")
};

UENUM(BlueprintType)
enum class EPivotPointDetectionMode : uint8
{
	AutoMinRootMotionSpeed UMETA(DisplayName = "Auto: Min Root Motion Speed"),
	ManualTime            UMETA(DisplayName = "Manual Time")
};

/**
 * Generates a single Lyra-style signed distance curve for Pivot / TurnBack animations.
 *
 * Difference from Lyra's original DistanceCurveModifier:
 * - Lyra uses net displacement from pivot point to sample time.
 * - This modifier uses accumulated root-motion path distance.
 *
 * Result:
 * - before pivot point: negative
 * - at pivot point: 0
 * - after pivot point: positive
 * - curve is monotonic increasing
 */
UCLASS()
class PROJECTZZZ_API UAnimationModifier_PivotDistanceCurve : public UAnimationModifier
{
	GENERATED_BODY()

public:
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	virtual void OnRevert_Implementation(UAnimSequence* AnimationSequence) override;

private:
	float CalculateMagnitude(const FVector& Vector) const;
	float CalculateMagnitudeSq(const FVector& Vector) const;
	float FindPivotTimeByMinRootMotionSpeed(UAnimSequence* AnimationSequence) const;

public:
	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve")
	FName CurveName = TEXT("Distance");

	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve")
	EPivotArcDistanceAxis Axis = EPivotArcDistanceAxis::XY;

	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve")
	EPivotPointDetectionMode PivotPointDetectionMode = EPivotPointDetectionMode::AutoMinRootMotionSpeed;

	/**
	 * Used only when PivotPointDetectionMode == ManualTime.
	 * Unit: seconds.
	 */
	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ManualPivotTime = 0.f;

	/**
	 * Curve sample rate.
	 */
	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve", meta = (ClampMin = "1", UIMin = "1"))
	int32 SampleRate = 60;

	/**
	 * Search sample rate for automatic pivot point detection.
	 */
	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve", meta = (ClampMin = "1", UIMin = "1"))
	int32 MinSpeedSearchSampleRate = 120;

	/**
	 * Ignore animation boundary when searching min speed.
	 * This avoids selecting frame 0 or the final frame as pivot because of boundary zero delta.
	 */
	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve", meta = (ClampMin = "0.0", ClampMax = "0.49", UIMin = "0.0", UIMax = "0.49"))
	float SearchStartRatio = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve", meta = (ClampMin = "0.51", ClampMax = "1.0", UIMin = "0.51", UIMax = "1.0"))
	float SearchEndRatio = 0.95f;

	UPROPERTY(EditAnywhere, Category = "Pivot Distance Curve")
	uint8 bRemoveExistingCurve : 1 = true;
};