// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_GameplayTag.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTZZZ_API UAnimNotifyState_GameplayTag : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	void SetGameplayTag(USkeletalMeshComponent* MeshComp, const bool bShouldAdd);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag{FGameplayTag::EmptyTag};
};
