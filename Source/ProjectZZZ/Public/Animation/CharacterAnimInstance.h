// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstanceBase.h"
#include "CharacterAnimInstance.generated.h"

UCLASS()
class PROJECTZZZ_API UCharacterAnimInstance : public UAnimInstanceBase
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	virtual void NativeBeginPlay() override;
	
public:
	// Get Locomotion Asset
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Idle() const;
	
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Idle_AFK() const;

	/*UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Walk() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Walk_Start() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Walk_End() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Walk_Start_End() const;*/

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Run() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Run_Start() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Run_End() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Run_Start_End() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Sprint() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Turn_Back() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe), DisplayName = "Should Distance Match To Stop")
	bool ShouldDistanceMatchToStop() const;

protected:
	const UCharacterAnimationPreset_Locomotion* GetLocomotionAnimPreset() const; 

	void UpdatePivotState();
public:
	FOnCombatWindowChanged OnCombatWindowChanged;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	TObjectPtr<UCharacterAnimationPreset_Locomotion> AnimPreset_Locomotion{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FAgentLocomotionAnimationState AgentLocoState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State | Pivot")
	FAgentPivotState PivotState;
	
	// Setting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting")
	FName DistanceCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bHasMovementInput : 1 {false};
};
