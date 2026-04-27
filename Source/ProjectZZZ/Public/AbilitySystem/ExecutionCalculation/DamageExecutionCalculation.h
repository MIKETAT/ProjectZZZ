// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AbilitySystem/AgentAttributeSet.h"
#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "DamageExecutionCalculation.generated.h"


struct FDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Attack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Impact);
	
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defence);
	DECLARE_ATTRIBUTE_CAPTUREDEF(StunDMGMultiplier);
	/*DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Daze);*/
	

	FDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseCombatAttributeSet, Attack, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAgentAttributeSet, Impact, Source, false);\
		
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseCombatAttributeSet, Defence, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UEnemyAttributeSet, StunDMGMultiplier, Target, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseCombatAttributeSet, Health, Target, false);
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UEnemyAttributeSet, Daze, Target, false);
	}
};

static const FDamageStatics& DamageStatics()
{
	static FDamageStatics DamageStatics;
	return DamageStatics;
}

UCLASS()
class PROJECTZZZ_API UDamageExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

	UDamageExecutionCalculation();
	
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
