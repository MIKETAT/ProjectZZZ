#include "AbilitySystem/EnemyAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UEnemyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetDazeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxDaze());
	}
	else if (Attribute == GetStunDMGMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 1.0f, GetStunDMGMultiplier()); 
	}
}

void UEnemyAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void UEnemyAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, Daze, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, MaxDaze, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UEnemyAttributeSet, StunDMGMultiplier, COND_None, REPNOTIFY_Always);
}

void UEnemyAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDazeAttribute())
	{
		SetDaze(FMath::Clamp(GetDaze(), 0.0f, GetMaxDaze()));
	}
	else if (Data.EvaluatedData.Attribute == GetStunDMGMultiplierAttribute())
	{
		SetStunDMGMultiplier(FMath::Clamp(GetStunDMGMultiplier(), 1.0f, GetStunDMGMultiplier()));
	}
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
