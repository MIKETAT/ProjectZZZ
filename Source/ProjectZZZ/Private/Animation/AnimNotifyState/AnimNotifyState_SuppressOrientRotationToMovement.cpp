// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotifyState/AnimNotifyState_SuppressOrientRotationToMovement.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAnimNotifyState_SuppressOrientRotationToMovement::NotifyBegin(USkeletalMeshComponent* MeshComp,
                                                                    UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	SetOrientRotationToMovement(MeshComp, false);
}

void UAnimNotifyState_SuppressOrientRotationToMovement::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	SetOrientRotationToMovement(MeshComp, true);
}

void UAnimNotifyState_SuppressOrientRotationToMovement::SetOrientRotationToMovement(USkeletalMeshComponent* MeshComp, bool bOrientRotationToMovement)
{
	if (!MeshComp)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner()))
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->bOrientRotationToMovement = bOrientRotationToMovement;
		}
	}
}
