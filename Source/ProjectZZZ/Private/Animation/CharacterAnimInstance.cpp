#include "Animation/CharacterAnimInstance.h"

#include "Character/CharacterBase.h"

// Get Locomotion Animation Asset
#define DEFINE_LOCOMOTION_ANIM_GETTER(FuncName, Field) \
const UAnimSequenceBase* UCharacterAnimInstance::FuncName() const \
{ \
	const UCharacterAnimationPreset_Locomotion* Preset = GetLocomotionAnimPreset(); \
	return Preset ? Preset->Field : nullptr; \
}

void UCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character)
	{
		bHasMovementInput = Character->bHasMovementInput;	
	}
}

void UCharacterAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Idle, LocomotionAnim_Idle)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Idle_AFK, LocomotionAnim_Idle_AFK)
/*DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Walk, LocomotionAnim_Walk)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Walk_Start, LocomotionAnim_Walk_Start)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Walk_End, LocomotionAnim_Walk_End)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Walk_Start_End, LocomotionAnim_Walk_Start_End)*/
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Run, LocomotionAnim_Run)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Run_Start, LocomotionAnim_Run_Start)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Run_End, LocomotionAnim_Run_End)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Run_Start_End, LocomotionAnim_Run_Start_End)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Sprint, LocomotionAnim_Sprint)
DEFINE_LOCOMOTION_ANIM_GETTER(GetLocomotionAnim_Turn_Back, LocomotionAnim_Turn_Back)

bool UCharacterAnimInstance::ShouldDistanceMatchToStop() const
{
	const bool HasVelocity{LocomotionAnimState.LocalVelocity2D.Size() > 0.f};
	const bool HasAcceleration{LocomotionAnimState.LocalAcceleration2D.Size() > 0.f};
	return HasVelocity && !HasAcceleration;
}
#undef DEFINE_LOCOMOTION_ANIM_GETTER

const UCharacterAnimationPreset_Locomotion* UCharacterAnimInstance::GetLocomotionAnimPreset() const
{
	return AnimPreset_Locomotion.Get();
}