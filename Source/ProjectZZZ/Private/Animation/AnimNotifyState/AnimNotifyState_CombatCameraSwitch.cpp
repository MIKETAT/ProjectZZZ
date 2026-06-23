#include "Animation/AnimNotifyState/AnimNotifyState_CombatCameraSwitch.h"
#include "Character/ZZZPlayerController.h"
#include "Player/PlayerCharacter.h"

void UAnimNotifyState_CombatCameraSwitch::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                      float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	ToggleCombatCamera(MeshComp, true);
}

void UAnimNotifyState_CombatCameraSwitch::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	ToggleCombatCamera(MeshComp, false);
}

AZZZPlayerController* UAnimNotifyState_CombatCameraSwitch::GetPlayerController(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return nullptr;
	}

	APlayerCharacter* Agent{Cast<APlayerCharacter>(MeshComp->GetOwner())};
	if (!Agent)
	{
		return nullptr; 
	}

	AZZZPlayerController* PC{Cast<AZZZPlayerController>(Agent->GetController())};
	if (!PC)
	{
		return nullptr;
	}

	return PC;
}

void UAnimNotifyState_CombatCameraSwitch::ToggleCombatCamera(USkeletalMeshComponent* MeshComp, bool bOpen)
{
	APlayerCharacter* Agent{Cast<APlayerCharacter>(MeshComp->GetOwner())};
	AZZZPlayerController* PC{GetPlayerController(MeshComp)};
	if (!PC || !Agent)
	{
		return;
	}
	
	if (UCharacterCombatComponent* CombatComponent = Agent->GetAgentCombatComponent())
	{
		if (bOpen)
		{
			CombatComponent->ActivateCombatCamera(Mode);
		} else
		{
			CombatComponent->DeactivateCombatCamera(Mode);
		}
	}
}
