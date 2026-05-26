// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimInstanceBase.h"

#include "Animation/CharacterAnimationPreset.h"
#include "Character/CharacterBase.h"
#include "Kismet/KismetMathLibrary.h"



void UAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Character = Cast<ACharacterBase>(GetOwningActor());
}

void UAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!IsValid(Character))
	{
		return;
	}
	
	RefreshLocomotionAnimationStateOnGameThread(DeltaSeconds);
}

void UAnimInstanceBase::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

void UAnimInstanceBase::RefreshLocomotionAnimationStateOnGameThread(const float DeltaSeconds)
{
	check(IsInGameThread());

	const auto& CharLocoState{Character->GetLocomotionState()};

	LocomotionAnimState.DisplacementSinceLastUpdate = CharLocoState.DisplacementSinceLastUpdate;
	LocomotionAnimState.DisplacementSpeed = CharLocoState.DisplacementSpeed;
	LocomotionAnimState.bIsMoving = CharLocoState.bIsMoving;
	
	LocomotionAnimState.WorldLocation = CharLocoState.WorldLocation;
	LocomotionAnimState.WorldRotation = CharLocoState.WorldRotation;
	LocomotionAnimState.WorldVelocity = CharLocoState.WorldVelocity;
	LocomotionAnimState.WorldVelocity2D = CharLocoState.WorldVelocity2D;
	LocomotionAnimState.LocalVelocity2D = CharLocoState.LocalVelocity2D;
	LocomotionAnimState.WorldAcceleration2D = CharLocoState.WorldAcceleration2D;
	LocomotionAnimState.LocalAcceleration2D = CharLocoState.LocalAcceleration2D;
	LocomotionAnimState.Speed2D = CharLocoState.Speed2D;
	LocomotionAnimState.Acceleration2D = CharLocoState.Acceleration2D;
	// todo: Pivot
	//bPivotActive = FVector::DotProduct(LocomotionAnimState.LocalVelocity2D, LocomotionAnimState.LocalAcceleration2D) < 0.f;
}
