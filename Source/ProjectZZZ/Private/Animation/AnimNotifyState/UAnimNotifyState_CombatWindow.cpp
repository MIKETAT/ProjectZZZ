// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotifyState/UAnimNotifyState_CombatWindow.h"

#include "Animation/AnimInstanceBase.h"

void UUAnimNotifyState_CombatWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                 float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	BroadcastCombatWindowChanged(MeshComp, Animation, true);
}

void UUAnimNotifyState_CombatWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	BroadcastCombatWindowChanged(MeshComp, Animation, false);
}
void UUAnimNotifyState_CombatWindow::BroadcastCombatWindowChanged(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, bool bIsOpen)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	if (UAnimInstanceBase* AnimInstance = Cast<UAnimInstanceBase>(MeshComp->GetAnimInstance()))
	{
		if (UAnimMontage* SourceMontage = Cast<UAnimMontage>(Animation))
		{
			AnimInstance->OnCombatWindowChanged.Broadcast(Tag, bIsOpen, SourceMontage);
			UE_LOG(LogTemp, Warning, TEXT("Combat Window.  Tag = %s, IsOpen = %s"), *Tag.ToString(), bIsOpen ? TEXT("true") : TEXT("false"));
		}
	}
}


