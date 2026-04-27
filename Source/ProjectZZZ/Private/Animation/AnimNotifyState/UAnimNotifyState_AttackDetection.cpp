// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotifyState/UAnimNotifyState_AttackDetection.h"
#include "Character/Component/CombatComponentBase.h"

void UUAnimNotifyState_AttackDetection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (GetCombatComponentBase(MeshComp))
	{
		GetCombatComponentBase(MeshComp)->EnableAttackDetection(ActionStep.Get(), ShapeConfig);	
	}
}

void UUAnimNotifyState_AttackDetection::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (GetCombatComponentBase(MeshComp))
	{
		GetCombatComponentBase(MeshComp)->DisableAttackDetection();	
	}
}


UCombatComponentBase* UUAnimNotifyState_AttackDetection::GetCombatComponentBase(
	const USkeletalMeshComponent* MeshComp) const
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
