// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Character/Combat/AttackDetection.h"
#include "UAnimNotifyState_AttackDetection.generated.h"

class UCombatActionStep;
class UCombatComponentBase;

UCLASS()
class PROJECTZZZ_API UAnimNotifyState_AttackDetection : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
private:

	UCombatComponentBase* GetCombatComponentBase(const USkeletalMeshComponent* MeshComp) const;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitShapeConfig ShapeConfig;
};
