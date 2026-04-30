// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Character/CharacterFrameDataBus.h"
#include "Combat/CombatInterface.h"
#include "GameFramework/Character.h"
#include "State/LocomotionState.h"
#include "CharacterBase.generated.h"

class UCombatComponentBase;
class UBaseCombatAttributeSet;
class UCombatAnimSchedulerComponent;
class UGameplayEffect;
class UAgentAbilitySystemComponent;
class UAgentAttributeSet;
class UCharacterCombatComponent;
struct FCharacterFrameDataBus;
class UPlayerInputHandlerComponent;
struct FInputActionValue;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;

UCLASS()
class PROJECTZZZ_API ACharacterBase : public ACharacter, public ICombatInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void UnPossessed() override;
	
	virtual void PostInitializeComponents() override;
	// ~Interface

	// GAS Interface
public:
	UAbilitySystemComponent* GetAbilitySystemComponent() const { return AgentAbilitySystemComponent.Get(); }
	// !GAS Interface

	// ICombatInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComp() const override { return AgentAbilitySystemComponent.Get(); }
	
	//virtual UCombatComponentBase* GetCombatComp() const override { return CombatComponent.Get(); };
	// ~ICombatInterface
public:
	const FLocomotionState& GetLocomotionState() const { return LocomotionState; }
	
	UBaseCombatAttributeSet* GetBaseCombatAttribute() const { return BaseCombatAttribute; }
	
	UCombatAnimSchedulerComponent* GetCombatAnimSchedulerComponent() const { return CombatAnimSchedulerComponent; }
	
	UCharacterCombatComponent* GetCombatComponent() const { return CombatComponent; }

public:
	virtual void Die() {};
	
protected:
	void RefreshInput(const float DeltaTime);
	
	void RefreshLocomotionState(const float DeltaTime);
	
	void InitializeAttributes();

private:
	void ApplyGameplayEffectToSelf(const TSubclassOf<UGameplayEffect>& Effect);
	
	virtual TSubclassOf<UGameplayEffect> GetExclusiveInitGE() const { return nullptr; };
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> BaseInitGE;
	
	// todo: 规范替代下面的Loco State 变量
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	uint8 bHasMovementInput : 1 {false};
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterCombatComponent> CombatComponent{nullptr};
	
	UPROPERTY()
	TObjectPtr<UAgentAbilitySystemComponent> AgentAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCombatAnimSchedulerComponent> CombatAnimSchedulerComponent;
	
	UPROPERTY()
	TObjectPtr<UBaseCombatAttributeSet> BaseCombatAttribute;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	FLocomotionState LocomotionState;
};
