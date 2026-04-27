// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/EnemyAttributeSet.h"

#include "Net/UnrealNetwork.h"

void UEnemyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetDazeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxDaze());
	}
	else if (Attribute == GetMaxDazeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxDaze());
	}
	else if (Attribute == GetStunDMGMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 1.0f, GetStunDMGMultiplier());
	}
}

void UEnemyAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue,
	float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
}

void UEnemyAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, Daze, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, MaxDaze, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, StunDMGMultiplier, COND_None, REPNOTIFY_Always);
}

void UEnemyAttributeSet::OnRep_Daze(const FGameplayAttributeData& OldDaze)
{
}

void UEnemyAttributeSet::OnRep_MaxDaze(const FGameplayAttributeData& OldMaxDaze)
{
}

void UEnemyAttributeSet::OnRep_StunDMGMultiplier(const FGameplayAttributeData& OldStunDMGMultiplier)
{
}
