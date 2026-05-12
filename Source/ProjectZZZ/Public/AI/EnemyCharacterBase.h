// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Character/Combat/CombatInterface.h"
#include "EnemyCharacterBase.generated.h"

class UEnemyCombatComponent;
class UEnemyAttributeSet;

UCLASS()
class PROJECTZZZ_API AEnemyCharacterBase : public ACharacterBase
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
	
	// ~Combat Interface

	//virtual TSubclassOf<UGameplayEffect> GetExclusiveInitGE() const override { return EnemyExclusiveInitGE; }

	UEnemyCombatComponent* GetEnemyCombatComponent() const { return EnemyCombatComponent; }

	virtual void InitializeAttributes() override;
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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bPrintDebugInfo{false};

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent{nullptr};
};
