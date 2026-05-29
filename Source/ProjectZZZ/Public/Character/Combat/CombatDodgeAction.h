// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatStep.h"
#include "CombatDodgeAction.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FDodgeDirectionEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Montage;
	
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "-180", ClampMax = "180"))
	float MinAngle{0.f};
    
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "-180", ClampMax = "180"))
	float MaxAngle{0.f};
};

UCLASS()
class PROJECTZZZ_API UCombatDodgeAction : public UCombatActionStep
{
	GENERATED_BODY()

public:
	virtual UAnimMontage* GetAnimMontage(const FCharacterFrameDataBus& Data) const override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FDodgeDirectionEntry> DodgeEntries;
};
