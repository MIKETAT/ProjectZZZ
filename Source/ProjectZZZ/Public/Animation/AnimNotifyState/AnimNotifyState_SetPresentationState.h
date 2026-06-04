// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Character/CharacterBase.h"
#include "AnimNotifyState_SetPresentationState.generated.h"

enum class ECharacterPresentationState : uint8;
/**
 * 
 */
UCLASS()
class PROJECTZZZ_API UAnimNotifyState_SetPresentationState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECharacterPresentationState State;

private:
	UPROPERTY(Transient)
	ECharacterPresentationState PreviousState{ECharacterPresentationState::ActiveInvisible};
};
