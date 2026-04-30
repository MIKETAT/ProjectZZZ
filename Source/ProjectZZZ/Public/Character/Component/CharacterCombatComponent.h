// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CombatComponentBase.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "CharacterCombatComponent.generated.h"

class UCombatAnimSchedulerComponent;
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
	bool IsAllowMovementInterruptAction() const;
	
	// CombatComponentBase Interface
	virtual void ProcessHitFeedback(const FAttackResult& Result) override;
	
	/*
	virtual void ProcessHitEvent(AActor* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config) override;

	virtual void HandleIncomingDamage(const FAttackContext& Context, FAttackResult& OutResult ) override;
	
	virtual void EnableAttackDetection(UCombatActionStep* ActionStep, const FHitShapeConfig& ShapeConfig) override;
	
	virtual void DisableAttackDetection() override;*/
	// !Interface


	virtual void InjectAndBindASC(UAgentAbilitySystemComponent* InASC) override;

	void ExecuteSwitchInAction();
	
	void ExecuteSwitchOutAction();

	void ExecuteSwitchAction(UAnimMontage* Montage);
	
	void OnEnergyChanged(const FOnAttributeChangeData& Data);

	void OnDecibelsChanged(const FOnAttributeChangeData& Data);

	void OnDazeChanged(const FOnAttributeChangeData& Data);
	
	void HandleStun();

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
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> SwitchInMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> SwitchOutMontage;
private:
	float GlobalBufferLifespan{0.3f}; 
	
	UPROPERTY()
	TObjectPtr<UGameplayEffect> ActionCostGE{nullptr};

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAgentCombatSteps> AgentCombatSteps{nullptr};
	
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

