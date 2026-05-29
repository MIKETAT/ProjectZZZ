#include "Animation/AnimNotifyState/AnimNotifyState_CombatWindow.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/CharacterAnimInstance.h"

void UAnimNotifyState_CombatWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	BroadcastCombatWindowChanged(MeshComp, Animation, true);
}

void UAnimNotifyState_CombatWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	BroadcastCombatWindowChanged(MeshComp, Animation, false);
}

void UAnimNotifyState_CombatWindow::BroadcastCombatWindowChanged(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, bool bIsOpen)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	if (UCharacterAnimInstance* AnimInstance = Cast<UCharacterAnimInstance>(MeshComp->GetAnimInstance()))
	{
		if (UAnimMontage* SourceMontage = Cast<UAnimMontage>(Animation))
		{
			AnimInstance->OnCombatWindowChanged.Broadcast(Tag, bIsOpen, SourceMontage);
		}
	}
}


