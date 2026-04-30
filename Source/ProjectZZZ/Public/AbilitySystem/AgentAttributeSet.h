// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AgentAttributeSet.generated.h"

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTZZZ_API UAgentAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Impact(const FGameplayAttributeData& OldImpact);
	
	UFUNCTION()
	void OnRep_Energy(const FGameplayAttributeData& OldEnergy);

	UFUNCTION()
	void OnRep_MaxEnergy(const FGameplayAttributeData& OldMaxEnergy);

	UFUNCTION()
	void OnRep_Decibels(const FGameplayAttributeData& OldDecibels);

	UFUNCTION()
	void OnRep_MaxDecibels(const FGameplayAttributeData& OldMaxDecibels);
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Impact)
	FGameplayAttributeData Impact;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, Impact)
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Energy)
	FGameplayAttributeData Energy;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, Energy)
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxEnergy)
	FGameplayAttributeData MaxEnergy;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, MaxEnergy)
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Decibels)
	FGameplayAttributeData Decibels;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, Decibels)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxDecibels)
	FGameplayAttributeData MaxDecibels;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, MaxDecibels)

	// Custom Attributes
	FGameplayTagContainer AgentCustomTags;

	FGameplayTag Status;		// Normal / light Knockback/ significant knockback / Stagger
};
