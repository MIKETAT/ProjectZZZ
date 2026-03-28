// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterAnimationPreset.h"
#include "State/LocomotionAnimationState.h"
#include "AnimInstanceBase.generated.h"

class ACharacterBase;
class UCharacterAnimationPreset_Locomotion;
/**
 * 
 */
UCLASS()
class PROJECTZZZ_API UAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual  void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	virtual void NativeBeginPlay() override;
	
public:
	// Get Locomotion Asset
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Idle() const;
	
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Idle_AFK() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Walk() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Walk_Start() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Walk_End() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Walk_Start_End() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Run() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Run_Start() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Run_End() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Run_Start_End() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	const UAnimSequenceBase* GetLocomotionAnim_Turn_Back() const;

public:
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe), DisplayName = "Should Distance Match To Stop")
	bool ShouldDistanceMatchToStop() const;
	
protected:
	const UCharacterAnimationPreset_Locomotion* GetLocomotionAnimPreset() const; 

private:

	void RefreshLocomotionAnimationStateOnGameThread(const float DeltaSeconds);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	TObjectPtr<UCharacterAnimationPreset_Locomotion> AnimPreset_Locomotion{nullptr};


		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient)
	TObjectPtr<ACharacterBase> Character;
	
	// Setting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting")
	FName DistanceCurveName;

	// State Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FLocomotionAnimationState LocomotionAnimState;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bHasMovementInput : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bPivotActive : 1 {false};

};
