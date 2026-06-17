// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstanceBase.h"
#include "CharacterAnimInstance.generated.h"

/**
 * 
 */
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
	
	// Setting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting")
	FName DistanceCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bHasMovementInput : 1 {false};

	// Pivot
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Setting | Pivot")
	uint8 bPivotActive : 1 {false};				// Allow Animation State Machine Transition

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting | Pivot")	//, meta=(ClampMin = )
	float PivotEnterAngleDegrees{160.f};		// 进入Pivot所需的速度方向与加速度方向夹角

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting | Pivot")
	float PivotMinSpeedRatio{0.75f};			// 进入Pivot所需速度至少占MaxWalkSpeed比率

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Setting | Pivot")
	float PivotDot{1.f};		// delete after pivot done

	bool bCanTriggerPivot{false};				// Allow Trigger Pivot Transition(Set bPivotActive true)

	float EnterPivotDot{0.f};					// wait for initialize
};
