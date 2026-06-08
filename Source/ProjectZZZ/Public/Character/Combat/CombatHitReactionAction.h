// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatStep.h"
#include "CombatHitReactionAction.generated.h"

UENUM(BlueprintType)
enum class EHitReactionDirection : uint8
{
	None,
	Front,
	Back,
	/*
	 * HitReaction Anim only support Front and Back
	Left,
	Right
	*/
};


UCLASS()
class PROJECTZZZ_API UCombatHitReactionAction : public UCombatActionStep
{
	GENERATED_BODY()

public:
	UCombatHitReactionAction();

};

USTRUCT(BlueprintType)
struct FDirectionalHitReactionActions
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCombatActionStep> FrontHit;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCombatActionStep> BackHit;
};