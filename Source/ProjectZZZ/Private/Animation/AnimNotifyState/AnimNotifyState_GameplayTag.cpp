#include "Animation/AnimNotifyState/AnimNotifyState_GameplayTag.h"
#include "Character/CharacterBase.h"

void UAnimNotifyState_GameplayTag::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	SetGameplayTag(MeshComp, true);
}

void UAnimNotifyState_GameplayTag::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	SetGameplayTag(MeshComp, false);
}

void UAnimNotifyState_GameplayTag::SetGameplayTag(USkeletalMeshComponent* MeshComp, const bool bShouldAdd)
{
	if (ACharacterBase* Character{Cast<ACharacterBase>(MeshComp->GetOwner())})
	{
		if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComp())
		{
			if (bShouldAdd)
			{
				ASC->AddLooseGameplayTag(Tag);
			} else
			{
				ASC->RemoveLooseGameplayTag(Tag);
			}
		}
	}
}
