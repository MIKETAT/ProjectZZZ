#include "Animation/AnimNotifyState/AnimNotifyState_AttackDetection.h"
#include "Character/Component/CombatComponentBase.h"

void UAnimNotifyState_AttackDetection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (GetCombatComponentBase(MeshComp))
	{
		GetCombatComponentBase(MeshComp)->EnableAttackDetection(Animation, SegmentName);	
	}
}

void UAnimNotifyState_AttackDetection::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (GetCombatComponentBase(MeshComp))
	{
		GetCombatComponentBase(MeshComp)->DisableAttackDetection(Animation, SegmentName);	
	}
}


UCombatComponentBase* UAnimNotifyState_AttackDetection::GetCombatComponentBase(
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
