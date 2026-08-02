#pragma once

#include "CoreMinimal.h"

struct FAttackDetectionSpec;

struct PROJECTZZZ_API FAttackShapeQueryGeometry
{
	FTransform WorldTransform{FTransform::Identity};

	FCollisionShape CollisionShape;
};

struct PROJECTZZZ_API FAttackSweepGeometry
{
	FVector Start{FVector::ZeroVector};

	FVector End{FVector::ZeroVector};
	
	FQuat Rotation{FQuat::Identity};

	FCollisionShape CollisionShape;
};

namespace AttackDetectionGeometry
{
	PROJECTZZZ_API bool BuildShapeQueryGeometry(const FAttackDetectionSpec& Spec, const FTransform& ReferenceWorldTransform, FAttackShapeQueryGeometry& OutGeometry);

	PROJECTZZZ_API bool BuildWeaponSweepGeometry(
		const FAttackDetectionSpec& Spec,
		const float SampleDeltaTime,
		const FTransform& PreviousRoot,
		const FTransform& PreviousTip,
		const FTransform& CurrentRoot,
		const FTransform& CurrentTip,
		TArray<FAttackSweepGeometry>& OutSweeps);

	void SubStepWeaponSweep(
		const FAttackDetectionSpec& Spec, 
		const FVector& PreviousRootLocation,
		const FVector& PreviousTipLocation,
		const FVector& CurrentRootLocation,
		const FVector& CurrentTipLocation,
		const FQuat& ShapeRotation,
		TArray<FAttackSweepGeometry>& OutSweeps);

	PROJECTZZZ_API bool BuildActorPathSweepGeometry(
		const FAttackDetectionSpec& Spec,
		const FTransform& PreviousReferenceTransform,
		const FTransform& CurrentReferenceTransform,
		FAttackSweepGeometry& OutSweep);

	void BuildShapeWorldTransform(
		const FAttackDetectionSpec& Spec,
		const FTransform& ReferenceWorldTransform,
		FTransform& OutShapeWorldTransform);
}
	