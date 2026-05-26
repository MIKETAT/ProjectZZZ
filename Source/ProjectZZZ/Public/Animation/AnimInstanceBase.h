// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterAnimationPreset.h"
#include "GameplayTagContainer.h"
#include "State/LocomotionAnimationState.h"
#include "AnimInstanceBase.generated.h"

class ACharacterBase;
class UCharacterAnimationPreset_Locomotion;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCombatWindowChanged, FGameplayTag, WindowTag, bool, bIsOpen, UAnimMontage*, SourceMontage);

UCLASS()
class PROJECTZZZ_API UAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	virtual void NativeBeginPlay() override;

private:
	void RefreshLocomotionAnimationStateOnGameThread(const float DeltaSeconds);

protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient)
	TObjectPtr<ACharacterBase> Character;

	// State Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FLocomotionAnimationState LocomotionAnimState;

};
