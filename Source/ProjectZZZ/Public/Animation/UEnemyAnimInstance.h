// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstanceBase.h"
#include "UEnemyAnimInstance.generated.h"


UCLASS()
class PROJECTZZZ_API UUEnemyAnimInstance : public UAnimInstanceBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FEnemyLocomotionAnimationState EnemyLocoState;
};
