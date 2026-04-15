// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "CharacterCombatComponent.generated.h"


class UAnimInstanceBase;
enum ECombatAnimRequestFinishReason : uint8;
class UCombatAnimSchedulerComponent;
class UAgentAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API UCharacterCombatComponent : public UActorComponent
{
	GENERATED_BODY()
// Interface
public:
	UCharacterCombatComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
// ~Interface
public:
	void TryInitComponents();

	bool IsAllowMovementInterruptAction() const;

	void CancelCurrentAction();
	
	bool CanInterruptCurrentAction(const UCombatActionStep* Step) const;
	

	UFUNCTION()
	void HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason);

	UFUNCTION()
	void HandleCombatWindowChange(const FGameplayTag Tag, bool bIsOpen, UAnimMontage* SourceMontage);
private:
	void InitializeCombatStepList();
	void RefreshInputActionBitmask(const float DeltaTime);

	void ProcessInputAction(const float DeltaTime);
	void ProcessBufferedInput(const float DeltaTime);
	
	UCombatActionStep* SelectTargetAction();
	UCombatActionStep* SelectComboActionIntent(const EInputAction Input);
	UCombatActionStep* SelectCombatActionIntent(const EInputAction Input);
	
	void BufferInputIntent(const UCombatActionStep* ActionToBuffer);

	// Check if meet Step's cost
	bool CanAffordActionCost(const UCombatActionStep* Step) const;

	void PayActionCost(const UCombatActionStep* Step);
	
	bool IsAnyActionActive() const;

	int32 ExecuteAction(const UCombatActionStep* Step);

private:
	float GlobalBufferLifespan{0.3f}; 
	
	UPROPERTY()
	TObjectPtr<ACharacterBase> Character{nullptr};

	UPROPERTY()
	TObjectPtr<UAnimInstanceBase> AnimInstance{nullptr};
	
	UPROPERTY()
	TObjectPtr<UAgentAbilitySystemComponent> AbilitySystemComponent{nullptr};

	UPROPERTY()
	TObjectPtr<UCombatAnimSchedulerComponent> CombatAnimSchedulerComponent{nullptr};

	UPROPERTY()
	TObjectPtr<UGameplayEffect> ActionCostGE{nullptr};

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAgentCombatSteps> AgentCombatSteps{nullptr};

	UPROPERTY()
	FCombatExecutionState CurrentExecutionState;

	UPROPERTY()
	FBufferedIntent PendingIntent;

	UPROPERTY(Transient)
	TArray<UCombatActionStep*> CombatActionList;
	
	// Double Buffering. Use CurrentInputActionBitmask.
public:
	FInputBitmask InputActionBitmask;
private:
	FInputBitmask CurrentInputActionBitmask;	
};