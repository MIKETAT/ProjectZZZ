// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "EnemyAttributeSet.generated.h"

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


UCLASS()
class PROJECTZZZ_API UEnemyAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void OnRep_Daze(const FGameplayAttributeData& OldDaze);

	UFUNCTION()
	void OnRep_MaxDaze(const FGameplayAttributeData& OldMaxDaze);

	UFUNCTION()
	void OnRep_StunDMGMultiplier(const FGameplayAttributeData& OldStunDMGMultiplier);
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Daze)
	FGameplayAttributeData Daze;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, Daze)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxDaze)
	FGameplayAttributeData MaxDaze;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, MaxDaze)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_StunDMGMultiplier)
	FGameplayAttributeData StunDMGMultiplier;			// 失衡易伤倍率
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, StunDMGMultiplier)
	
};
