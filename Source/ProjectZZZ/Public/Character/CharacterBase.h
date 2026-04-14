// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Character/CharacterFrameDataBus.h"
#include "GameFramework/Character.h"
#include "State/LocomotionState.h"
#include "CharacterBase.generated.h"

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
class PROJECTZZZ_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

	// Interface
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
	
public:
	const FLocomotionState& GetLocomotionState() const { return LocomotionState; }

	UAgentAttributeSet* GetAgentAttributeSet() { return AgentAttributeSet; }
	UCombatAnimSchedulerComponent* GetCombatAnimSchedulerComponent() const { return CombatAnimSchedulerComponent; }
protected:
	void RefreshInput(const float DeltaTime);
	void RefreshLocomotionState(const float DeltaTime);
	
	void InitializeAttributes();
	
public:
// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCharacterCombatComponent> CombatComponent{nullptr};
	
// todo: 规范替代下面的Loco State 变量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	uint8 bHasMovementInput : 1 {false};

	UPROPERTY()
	TObjectPtr<UAgentAbilitySystemComponent> AgentAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCombatAnimSchedulerComponent> CombatAnimSchedulerComponent;
	
	UPROPERTY()
	TObjectPtr<UAgentAttributeSet> AgentAttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> InitAttributes;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FLocomotionState LocomotionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FCharacterFrameDataBus CharacterFrameDataBus;
};
