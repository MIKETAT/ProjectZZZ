// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify/AnimNotify_TriggerAttackDetection.h"

#include "Character/Component/CombatComponentBase.h"

void UAnimNotify_TriggerAttackDetection::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (GetCombatComponentBase(MeshComp))
	{
		GetCombatComponentBase(MeshComp)->TriggerAttackDetectionQuery(ActionTag, SegmentName);	
	}
}


UCombatComponentBase* UAnimNotify_TriggerAttackDetection::GetCombatComponentBase(const USkeletalMeshComponent* MeshComp) const
{
	if (!IsValid(MeshComp))
	{
		return nullptr;
	}

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UCombatComponentBase* CombatComp = Owner->FindComponentByClass<UCombatComponentBase>())
		{
			return CombatComp;
		}
	}
	return nullptr;
}
