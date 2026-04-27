// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AgentAttributeSet.h"
#include "Net/UnrealNetwork.h"

void UAgentAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	// Adjust for max change
}

void UAgentAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);

}

void UAgentAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UAgentAttributeSet, Impact, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAgentAttributeSet, Energy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAgentAttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAgentAttributeSet, Decibels, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAgentAttributeSet, MaxDecibels, COND_None, REPNOTIFY_Always);
}

void UAgentAttributeSet::OnRep_Impact(const FGameplayAttributeData& OldImpact)
{
}


void UAgentAttributeSet::OnRep_Energy(const FGameplayAttributeData& OldEnergy)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAgentAttributeSet, Energy, OldEnergy);
}

void UAgentAttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldMaxEnergy)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAgentAttributeSet, MaxEnergy, OldMaxEnergy);
}

void UAgentAttributeSet::OnRep_Decibels(const FGameplayAttributeData& OldDecibels)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAgentAttributeSet, Decibels, OldDecibels);
}

void UAgentAttributeSet::OnRep_MaxDecibels(const FGameplayAttributeData& OldMaxDecibels)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAgentAttributeSet, MaxDecibels, OldMaxDecibels);
}
