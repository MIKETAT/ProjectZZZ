#include "Character/Combat/AttackDetectionGeometry.h"
#include "Character/Combat/AttackDetection.h"
#include "Kismet/KismetMathLibrary.h"

bool AttackDetectionGeometry::BuildShapeQueryGeometry(const FAttackDetectionSpec& Spec, const FTransform& ReferenceWorldTransform, FAttackShapeQueryGeometry& OutGeometry)
{
	OutGeometry = FAttackShapeQueryGeometry{};
	if (Spec.DetectionMode != EAttackDetectionMode::ShapeQueryInstant && Spec.DetectionMode != EAttackDetectionMode::ShapeQueryContinuous)
	{
		return false;
	}

	BuildShapeWorldTransform(Spec, ReferenceWorldTransform, OutGeometry.WorldTransform);
	OutGeometry.CollisionShape = Spec.ShapeConfig.GetCollisionShape();
	return true;
}

bool AttackDetectionGeometry::BuildWeaponSweepGeometry(const FAttackDetectionSpec& Spec, const float SampleDeltaTime,
	const FTransform& PreviousRoot, const FTransform& PreviousTip, const FTransform& CurrentRoot,
	const FTransform& CurrentTip, TArray<FAttackSweepGeometry>& OutSweeps)
{
	OutSweeps.Reset();
	
	// Check
	if (Spec.DetectionMode != EAttackDetectionMode::WeaponSweep
		|| Spec.MaxSubStepTime <= UE_KINDA_SMALL_NUMBER
		|| Spec.MaxSubStepAngle <= UE_KINDA_SMALL_NUMBER
		|| Spec.MaxSubStepCount < 1
		|| Spec.WeaponSampleCount < 2
		|| SampleDeltaTime < 0.f)
	{
		return false;
	}

	const FQuat PreviousWeaponRotation = PreviousRoot.GetRotation().GetNormalized();
	const FQuat CurrentWeaponRotation = CurrentRoot.GetRotation().GetNormalized();

	// Calc StepCount
	const int32 TimeStepCount = FMath::CeilToInt(SampleDeltaTime / Spec.MaxSubStepTime);
	const float DeltaAngle = FMath::RadiansToDegrees(PreviousWeaponRotation.AngularDistance(CurrentWeaponRotation));
	const int32 AngleStepCount = FMath::CeilToInt(DeltaAngle / Spec.MaxSubStepAngle);
	const int32 SubStepCount = FMath::Clamp(FMath::Max(TimeStepCount, AngleStepCount), 1, Spec.MaxSubStepCount);

	FVector PreviousSubStepRootLocation = PreviousRoot.GetLocation();
	FVector PreviousSubStepTipLocation = PreviousTip.GetLocation();
	
	for (int32 Step = 0; Step < SubStepCount; Step++)
	{
		float Alpha = (Step + 1) / static_cast<float>(SubStepCount);
		
		FVector CurrentSubStepRootLocation = UKismetMathLibrary::VLerp(PreviousRoot.GetLocation(), CurrentRoot.GetLocation(), Alpha);
		FVector CurrentSubStepTipLocation = UKismetMathLibrary::VLerp(PreviousTip.GetLocation(), CurrentTip.GetLocation(), Alpha);
		FQuat CurrentSubStepWeaponRotation = FQuat::Slerp(PreviousWeaponRotation, CurrentWeaponRotation, Alpha).GetNormalized();
		
		// SubStepDetection
		const FTransform ShapeLocalTransform = FTransform(Spec.ShapeLocalRotation.Quaternion());
		const FTransform WeaponWorldTransform = FTransform(CurrentSubStepWeaponRotation);
		const FQuat ShapeWorldRotation = (ShapeLocalTransform * WeaponWorldTransform).GetRotation().GetNormalized(); 
		
		SubStepWeaponSweep(Spec,
			PreviousSubStepRootLocation, PreviousSubStepTipLocation,
			CurrentSubStepRootLocation, CurrentSubStepTipLocation,
			ShapeWorldRotation, OutSweeps
		);

		PreviousSubStepRootLocation = CurrentSubStepRootLocation;
		PreviousSubStepTipLocation = CurrentSubStepTipLocation;
	}
	
	return true;
}

void AttackDetectionGeometry::SubStepWeaponSweep(
	const FAttackDetectionSpec& Spec, 
	const FVector& PreviousRootLocation,
	const FVector& PreviousTipLocation,
	const FVector& CurrentRootLocation,
	const FVector& CurrentTipLocation,
	const FQuat& ShapeRotation,
	TArray<FAttackSweepGeometry>& OutSweeps)
{
	const int32 SampleCount = Spec.WeaponSampleCount;

	for (int32 Step = 0; Step < SampleCount; Step++)
	{
		float SampleAlpha = SampleCount > 1 ? (Step / static_cast<float>(SampleCount - 1)) : 0.f;
		
		FAttackSweepGeometry Geometry;
		Geometry.Start = FMath::Lerp(PreviousRootLocation, PreviousTipLocation, SampleAlpha);		// sample between root and tip
		Geometry.End = FMath::Lerp(CurrentRootLocation, CurrentTipLocation, SampleAlpha);
		Geometry.Rotation = ShapeRotation;
		Geometry.CollisionShape = Spec.ShapeConfig.GetCollisionShape();

		OutSweeps.Add(Geometry);
	}
}

bool AttackDetectionGeometry::BuildActorPathSweepGeometry(const FAttackDetectionSpec& Spec, const FTransform& PreviousReferenceTransform,
	const FTransform& CurrentReferenceTransform, FAttackSweepGeometry& OutSweep)
{
	OutSweep = FAttackSweepGeometry{};
	
	if (Spec.DetectionMode != EAttackDetectionMode::ActorPathSweep)
	{
		return false;
	}

	FTransform PreviousTransform;
	FTransform CurrentTransform;
	BuildShapeWorldTransform(Spec, PreviousReferenceTransform, PreviousTransform);
	BuildShapeWorldTransform(Spec, CurrentReferenceTransform, CurrentTransform);

	OutSweep.Start = PreviousTransform.GetLocation();
	OutSweep.End = CurrentTransform.GetLocation();
	OutSweep.Rotation = CurrentTransform.GetRotation().GetNormalized();
	OutSweep.CollisionShape = Spec.ShapeConfig.GetCollisionShape();
	return true;
}

void AttackDetectionGeometry::BuildShapeWorldTransform(const FAttackDetectionSpec& Spec, const FTransform& ReferenceWorldTransform, FTransform& OutShapeWorldTransform)
{
	const FTransform ShapeLocalTransform = FTransform(Spec.ShapeLocalRotation.Quaternion().GetNormalized(), Spec.ShapeLocalOffset, FVector::OneVector);
	FTransform ReferenceWorldTransformNoScale = ReferenceWorldTransform;
	ReferenceWorldTransformNoScale.SetScale3D(FVector::OneVector);

	OutShapeWorldTransform = ShapeLocalTransform * ReferenceWorldTransformNoScale;
	OutShapeWorldTransform.SetScale3D(FVector::OneVector);
}
