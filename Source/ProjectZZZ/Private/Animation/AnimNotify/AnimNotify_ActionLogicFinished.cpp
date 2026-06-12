#include "Animation/AnimNotify/AnimNotify_ActionLogicFinished.h"
#include "Player/PlayerCharacter.h"

void UAnimNotify_ActionLogicFinished::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                             const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	if (APlayerCharacter* Agent = Cast<APlayerCharacter>(MeshComp->GetOwner()))
	{
		if (UCharacterCombatComponent* CombatComponent = Agent->GetAgentCombatComponent())
		{
			CombatComponent->NotifyActionLogicFinished(ActionTag);
		}
	}
}
