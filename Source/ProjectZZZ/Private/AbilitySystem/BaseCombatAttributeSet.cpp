// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/BaseCombatAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


void UBaseCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseCombatAttributeSet, Health, OldHealth);
}

void UBaseCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseCombatAttributeSet, MaxHealth, OldMaxHealth);
}

void UBaseCombatAttributeSet::OnRep_Attack(const FGameplayAttributeData& OldMaxAttack)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseCombatAttributeSet, Attack, OldMaxAttack);
}

void UBaseCombatAttributeSet::OnRep_Defence(const FGameplayAttributeData& OldMaxDefence)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseCombatAttributeSet, Defence, OldMaxDefence);
}

void UBaseCombatAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseCombatAttributeSet, Attack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseCombatAttributeSet, Defence, COND_None, REPNOTIFY_Always);
}

void UBaseCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	// Adjust for max change

	/*else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}*/

}

void UBaseCombatAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetAttackAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetAttack());
	}
	else if (Attribute == GetDefenceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetDefence());
	}
}

void UBaseCombatAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}

	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(GetMaxHealth(), 0.f));
	}
	else if (Data.EvaluatedData.Attribute == GetAttackAttribute())
	{
		SetAttack(FMath::Max(GetAttack(), 0.f));
	}
	else if (Data.EvaluatedData.Attribute == GetDefenceAttribute())
	{
		SetDefence(FMath::Max(GetDefence(), 0.f));
	}
}
