// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotifyState/AnimNotifyState_SuppressOrientRotationToMovement.h"

#include "Character/CharacterBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAnimNotifyState_SuppressOrientRotationToMovement::NotifyBegin(USkeletalMeshComponent* MeshComp,
                                                                    UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	SetOrientRotationToMovement(MeshComp, false);

	AnimInstance = MeshComp->GetAnimInstance();

	Character = Cast<ACharacter>(MeshComp->GetOwner());
}

void UAnimNotifyState_SuppressOrientRotationToMovement::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	SetOrientRotationToMovement(MeshComp, true);
}

void UAnimNotifyState_SuppressOrientRotationToMovement::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!AnimInstance.IsValid())
	{
		AnimInstance = MeshComp->GetAnimInstance();
		return;
	}

	if (!Character.IsValid())
	{
		Character = Cast<ACharacter>(MeshComp->GetOwner());
		return;
	}
	
	
	float CurrentYaw{0.f};
	AnimInstance->GetCurveValue(RotationYawCurveName, CurrentYaw);
	
	if (bFirstUpdate)
	{
		if (FMath::IsNearlyZero(CurrentYaw))
		{
			LastYaw = CurrentYaw;
			bFirstUpdate = false;	
		}
		return;
	}

	float YawDeltaSinceLastUpdate{CurrentYaw - LastYaw};	// Add Delta Yaw = -179.885971
	
	UE_LOG(LogTemp, Error, TEXT("Current Yaw = %f, LastYaw = %f, Add Delta Yaw = %f, Is FirstUpdate = %d"), CurrentYaw, LastYaw, YawDeltaSinceLastUpdate, bFirstUpdate);
	
	LastYaw = CurrentYaw;

	Character->AddActorLocalRotation(FRotator(0.f, YawDeltaSinceLastUpdate, 0.f));
}

void UAnimNotifyState_SuppressOrientRotationToMovement::SetOrientRotationToMovement(USkeletalMeshComponent* MeshComp, bool bOrientRotationToMovement)
{
	if (!MeshComp)
	{
		return;
	}

	if (!Character.IsValid())
	{
		Character = Cast<ACharacterBase>(MeshComp->GetOwner());
	}
	
	if (Character.IsValid())
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->bOrientRotationToMovement = bOrientRotationToMovement;
		}
	}
}
