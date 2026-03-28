// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimInstanceBase.h"

#include "Animation/CharacterAnimationPreset.h"
#include "Character/CharacterBase.h"
#include "Kismet/KismetMathLibrary.h"

// Get Locomotion Animation Asset
#define DEFINE_LOCOMOTION_ANIM_GETTER(FuncName, Field) \
const UAnimSequenceBase* UAnimInstanceBase::FuncName() const \
{ \
	const UCharacterAnimationPreset_Locomotion* Preset = GetLocomotionAnimPreset(); \
	return Preset ? Preset->Field : nullptr; \
}

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
	bHasMovementInput = Character->bHasMovementInput;
}

void UAnimInstanceBase::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	ensure(IsValidLowLevel());
}

DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Idle, LocomotionAnim_Idle)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Idle_AFK, LocomotionAnim_Idle_AFK)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Walk, LocomotionAnim_Walk)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Walk_Start, LocomotionAnim_Walk_Start)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Walk_End, LocomotionAnim_Walk_End)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Walk_Start_End, LocomotionAnim_Walk_Start_End)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Run, LocomotionAnim_Run)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Run_Start, LocomotionAnim_Run_Start)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Run_End, LocomotionAnim_Run_End)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Run_Start_End, LocomotionAnim_Run_Start_End)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Turn_Back, LocomotionAnim_Turn_Back)

bool UAnimInstanceBase::ShouldDistanceMatchToStop() const
{
	const bool HasVelocity{LocomotionAnimState.LocalVelocity2D.Size() > 0.f};
	const bool HasAcceleration{LocomotionAnimState.LocalAcceleration2D.Size() > 0.f};
	return HasVelocity && !HasAcceleration;
}
#undef DEFINE_LOCOMOTION_ANIM_GETTER

const UCharacterAnimationPreset_Locomotion* UAnimInstanceBase::GetLocomotionAnimPreset() const
{
	return AnimPreset_Locomotion.Get();
}

void UAnimInstanceBase::RefreshLocomotionAnimationStateOnGameThread(const float DeltaSeconds)
{
	check(IsInGameThread());

	const auto& LocomotionState{Character->GetLocomotionState()};

	LocomotionAnimState.DisplacementSinceLastUpdate = (Character->GetActorLocation() - LocomotionAnimState.WorldLocation).Size2D();
	LocomotionAnimState.DisplacementSpeed = UKismetMathLibrary::SafeDivide(LocomotionAnimState.DisplacementSinceLastUpdate, DeltaSeconds);
	
	//UE_LOG(LogTemp, Error, TEXT("Displacement = %f  DisplacementSpeed = %f"), LocomotionAnimState.DisplacementSinceLastUpdate, LocomotionAnimState.DisplacementSpeed);
	
	LocomotionAnimState.WorldLocation = LocomotionState.WorldLocation;
	LocomotionAnimState.WorldRotation = LocomotionState.WorldRotation;
	LocomotionAnimState.WorldVelocity = LocomotionState.WorldVelocity;
	LocomotionAnimState.WorldVelocity2D = LocomotionState.WorldVelocity2D;
	LocomotionAnimState.LocalVelocity2D = LocomotionState.LocalVelocity2D;
	LocomotionAnimState.WorldAcceleration2D = LocomotionState.WorldAcceleration2D;
	LocomotionAnimState.LocalAcceleration2D = LocomotionState.LocalAcceleration2D;
	
	bPivotActive = FVector::DotProduct(LocomotionAnimState.LocalVelocity2D, LocomotionAnimState.LocalAcceleration2D) < 0.f;
}
