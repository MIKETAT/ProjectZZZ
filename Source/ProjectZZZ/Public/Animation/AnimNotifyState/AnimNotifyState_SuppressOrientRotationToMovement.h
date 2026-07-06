// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_SuppressOrientRotationToMovement.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTZZZ_API UAnimNotifyState_SuppressOrientRotationToMovement : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

private:
	void SetOrientRotationToMovement(USkeletalMeshComponent* MeshComp, bool bOrientRotationToMovement);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RotationYawCurveName{FName("RotationYaw")};
	
private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UAnimInstance> AnimInstance{nullptr};

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> Character{nullptr};
	
	bool bFirstUpdate{true};

	float LastYaw{0.f};

};
