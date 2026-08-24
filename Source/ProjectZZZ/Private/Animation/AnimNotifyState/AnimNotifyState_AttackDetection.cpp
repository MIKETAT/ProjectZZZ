#include "Animation/AnimNotifyState/AnimNotifyState_AttackDetection.h"
#include "Character/Component/CombatComponentBase.h"
#include "Character/Component/HitDetectionComponent.h"

void UAnimNotifyState_AttackDetection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                   float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (GetCombatComponentBase(MeshComp))
	{
		GetCombatComponentBase(MeshComp)->EnableAttackDetection(Animation, SegmentName);	
	}

	if (UHitDetectionComponent* HitDetectionComponent = GetHitDetectionComponent(MeshComp))
	{
		HitDetectionComponent->EnableHitDetection(EventReference);
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

	if (UHitDetectionComponent* HitDetectionComponent = GetHitDetectionComponent(MeshComp))
	{
		HitDetectionComponent->DisableHitDetection(EventReference);
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

UHitDetectionComponent* UAnimNotifyState_AttackDetection::GetHitDetectionComponent(const USkeletalMeshComponent* MeshComp) const
{
	if (!IsValid(MeshComp))
	{
		return nullptr;
	}

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UHitDetectionComponent* HitDetectionComponent = Owner->FindComponentByClass<UHitDetectionComponent>())
		{
			return HitDetectionComponent;
		}
	}

	return nullptr;
}
