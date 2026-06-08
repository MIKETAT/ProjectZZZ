// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionWarpCalcMethod.generated.h"

UENUM(BlueprintType)
enum class EMotionWarpCalculationRules : uint8
{
	None,
	EnemyRelativeFacing,		// Location + Offset
	PiercingLine,		//
	CameraRelative,      //
	BackToOrigin,
};

USTRUCT(BlueprintType)
struct FMotionWarpCalcMethod
{
	GENERATED_BODY()

	virtual ~FMotionWarpCalcMethod() = default;
};

USTRUCT(BlueprintType)
struct FMotionWarpCalcMethod_EnemyRelative : public FMotionWarpCalcMethod
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	bool bCloseToTarget{false};
	
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!bCloseToTarget"))
	FVector OffsetFromTarget{FVector::ZeroVector};
	
};

USTRUCT(BlueprintType)
struct FMotionWarpCalcMethod_PiercingLine : public FMotionWarpCalcMethod
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float PiercingLength{0.f};
};
