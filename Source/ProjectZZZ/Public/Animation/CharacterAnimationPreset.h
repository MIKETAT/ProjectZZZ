// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterAnimationPreset.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class PROJECTZZZ_API UCharacterAnimationPreset_Locomotion : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Idle", DisplayName = "Locomotion Anim Idle")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Idle{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Idle_AFK", DisplayName = "Locomotion Anim Idle_AFK")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Idle_AFK{nullptr};

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Walk", DisplayName = "Locomotion Anim Walk")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Walk{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Walk_Start", DisplayName = "Locomotion Anim Walk_Start")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Walk_Start{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Walk_End", DisplayName = "Locomotion Anim Walk_End")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Walk_End{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Walk_Start_End", DisplayName = "Locomotion Anim Walk_Start_End")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Walk_Start_End{nullptr};*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Run", DisplayName = "Locomotion Anim Run")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Run{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Run_Start", DisplayName = "Locomotion Anim Run_Start")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Run_Start{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Run_End", DisplayName = "Locomotion Anim Run_End")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Run_End{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Run_Start_End", DisplayName = "Locomotion Anim Run_Start_End")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Run_Start_End{nullptr};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Sprint", DisplayName = "Locomotion Anim Sprint")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Sprint{nullptr};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion Animation | Turn_Back", DisplayName = "Locomotion Anim Turn_Back")
	TObjectPtr<UAnimSequenceBase> LocomotionAnim_Turn_Back{nullptr};
};
