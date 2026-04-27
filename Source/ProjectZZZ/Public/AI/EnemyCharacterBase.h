// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Character/Combat/CombatInterface.h"
#include "EnemyCharacterBase.generated.h"

class UEnemyAttributeSet;

UCLASS()
class PROJECTZZZ_API AEnemyCharacterBase : public ACharacterBase, public ICombatInterface
{
	GENERATED_BODY()

public:
	AEnemyCharacterBase();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
protected:
	virtual void BeginPlay() override;

	// Combat Interface
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComp() const override { return GetAbilitySystemComponent(); }
	virtual UCharacterCombatComponent* GetCombatComp() const override { return GetCombatComponent(); }
	// ~Combat Interface

	virtual TSubclassOf<UGameplayEffect> GetExclusiveInitGE() const override { return EnemyExclusiveInitGE; }

	// Test
	virtual void Die() override;
	
private:
	void PrintDebugInfo();
	void PrintAttributeSet(UAttributeSet* Attribute);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> EnemyExclusiveInitGE;

	UPROPERTY()
	TObjectPtr<UEnemyAttributeSet> EnemyAttributeSet;
};
