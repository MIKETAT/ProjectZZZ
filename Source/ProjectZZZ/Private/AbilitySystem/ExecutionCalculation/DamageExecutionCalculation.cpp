// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecutionCalculation/DamageExecutionCalculation.h"

#include "Utility/ZZZGameplayTag.h"

UDamageExecutionCalculation::UDamageExecutionCalculation()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().DefenceDef);
	RelevantAttributesToCapture.Add(DamageStatics().ImpactDef);
	RelevantAttributesToCapture.Add(DamageStatics().StunDMGMultiplierDef);
}

void UDamageExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                         FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

	UAbilitySystemComponent* TargetAbilitySystemComponent{ExecutionParams.GetTargetAbilitySystemComponent()};
	UAbilitySystemComponent* SourceAbilitySystemComponent{ExecutionParams.GetSourceAbilitySystemComponent()};

	//const FGameplayTagContainer& TargetTags{ExecutionParams.GetPassedInTags()}; ?
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* SourceTags{Spec.CapturedSourceTags.GetAggregatedTags()};
	const FGameplayTagContainer* TargetTags{Spec.CapturedTargetTags.GetAggregatedTags()};
	
	FAggregatorEvaluateParameters EvaluateParams;

	float Attack{0.f};
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackDef, EvaluateParams, Attack);

	float Impact{0.f};
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ImpactDef, EvaluateParams, Impact);
	
	float Defence{0.f};
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenceDef, EvaluateParams, Defence);

	float StunDMGMultiplier{1.f};
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().StunDMGMultiplierDef, EvaluateParams, StunDMGMultiplier);
	
	float DamageMultiplier{Spec.GetSetByCallerMagnitude(Combat::Data::DamageMultiplier, false, 1.f)};
	float DazeMultiplier{Spec.GetSetByCallerMagnitude(Combat::Data::DazeMultiplier, false, 1.f)};
	
	float Damage = Attack * DamageMultiplier;
	if (TargetTags && TargetTags->HasTagExact(Combat::Status::Daze))
	{
		Damage *= StunDMGMultiplier;
	}
	Damage = Damage - Defence;
	
	if (Damage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UBaseCombatAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -Damage));
		UE_LOG(LogTemp, Error, TEXT("About to cause %f Damage"), Damage);
	}

	float Daze = Impact * DazeMultiplier;
	
	if (Daze > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UEnemyAttributeSet::GetDazeAttribute(), EGameplayModOp::Additive, Daze));
	}

	// Cause Dead/Daze
}
