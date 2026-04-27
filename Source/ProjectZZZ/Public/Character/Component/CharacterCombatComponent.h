// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CombatComponentBase.h"
#include "Character/Combat/AttackDetection.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "CharacterCombatComponent.generated.h"

class UCombatAnimSchedulerComponent;
class UAgentAbilitySystemComponent;
class UAnimInstanceBase;
enum ECombatAnimRequestFinishReason : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API UCharacterCombatComponent : public UCombatComponentBase
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

	// CombatComponentBase Interface
	//virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return Cast<UAbilitySystemComponent>(AbilitySystemComponent.Get()); }
	virtual void ProcessHitEvent(AActor* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config) override;
	
	virtual void HandleIncomingDamage(const FAttackContext& Context, FAttackResult& OutResult ) override;
	
	virtual void ProcessHitFeedback(const FAttackResult& Result) override;
	
	virtual void EnableAttackDetection(UCombatActionStep* ActionStep, const FHitShapeConfig& Config) override;
	
	virtual void DisableAttackDetection() override;
	// !Interface

	void BindAttributeListeners();
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnEnergyChanged(const FOnAttributeChangeData& Data);
	void OnDecibelsChanged(const FOnAttributeChangeData& Data);

	void OnDazeChanged(const FOnAttributeChangeData& Data);

	void HandleDeath();
	
	UFUNCTION()
	void HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason);

	UFUNCTION()
	void HandleCombatWindowChange(const FGameplayTag Tag, bool bIsOpen, UAnimMontage* SourceMontage);
private:
	// Input
	void RefreshInputActionBitmask(const float DeltaTime);
	void ProcessInputAction(const float DeltaTime);
	void ProcessBufferedInput(const float DeltaTime);
	
	// Action
	void InitializeCombatStepList();
	UCombatActionStep* SelectTargetAction();
	UCombatActionStep* SelectComboActionIntent(const EInputAction Input);
	UCombatActionStep* SelectCombatActionIntent(const EInputAction Input);
	
	void BufferInputIntent(const UCombatActionStep* ActionToBuffer);

	bool CanAffordActionCost(const UCombatActionStep* Step) const;

	void PayActionCost(const UCombatActionStep* Step);
	
	bool IsAnyActionActive() const;

	int32 ExecuteAction(const UCombatActionStep* Step);
	
	// AttackDetection
	FTransform CalculateShapeWorldTransform() const;

	void RefreshAttackDetection(float DeltaTime);
	void RefreshWeaponSweepDirection(float DeltaTime);

public:
	// todo
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> DamageEffectClass;	// todo
private:
	float GlobalBufferLifespan{0.3f}; 
	
	UPROPERTY()
	TObjectPtr<ACharacterBase> Character{nullptr};

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh{nullptr};

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

	// Attack Detection
	FAttackDetectionConfig AttackDetectionConfig;
	
	// Double Buffering. Use CurrentInputActionBitmask.
public:
	FInputBitmask InputActionBitmask;
private:
	FInputBitmask CurrentInputActionBitmask;	
};

