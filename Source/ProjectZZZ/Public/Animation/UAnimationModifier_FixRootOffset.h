// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "UAnimationModifier_FixRootOffset.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTZZZ_API UUAnimationModifier_FixRootOffset : public UAnimationModifier
{
	GENERATED_BODY()

public:
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;

public:
	UPROPERTY(EditAnywhere)
	FName PelvisBoneName;
};
