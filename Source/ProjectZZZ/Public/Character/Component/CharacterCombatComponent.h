// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CombatComponentBase.h"
#include "Animation/CharacterAnimInstance.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "CharacterCombatComponent.generated.h"

class UCharacterAnimInstance;
class AEnemyCharacterBase;
class UCombatAnimSchedulerComponent;
class UAnimInstanceBase;
enum ECombatAnimRequestFinishReason : uint8;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnActionLogicFinished, APlayerCharacter*, int32);

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
	FORCEINLINE UCharacterAnimInstance* GetAgentAnimInstance() const { return Cast<UCharacterAnimInstance>(AnimInstance); }
	
	bool IsAllowMovementInterruptAction() const;

	UCombatActionStep* GetSpecialAction(const FGameplayTag& Tag) const;
	
	// CombatComponentBase Interface
	
	virtual void HandleIncomingDamage(const FAttackContext& Context, FAttackResult& Result) override;

	// Only process the effects of this attack on self
	virtual void ProcessHitFeedback(const FAttackResult& Result) override;
	/*
	virtual void ProcessHitEvent(AActor* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config) override;
	
	virtual void EnableAttackDetection(UCombatActionStep* ActionStep, const FHitShapeConfig& ShapeConfig) override;
	
	virtual void DisableAttackDetection() override;*/
	// !Interface
	
	virtual void InjectAndBindASC(UAgentAbilitySystemComponent* InASC) override;

	bool CanExecuteSwitchInAction() const { return CanExecuteSwitchAction(SwitchInAction); }
	
	void ExecuteSwitchInAction();
	
	int32 ExecuteSwitchOutAction();

	int32 ExecuteSwitchAction(UCombatActionStep* Action, const FCombatActionContext& Context);
	
	void OnEnergyChanged(const FOnAttributeChangeData& Data);

	void OnDecibelsChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void HandleCombatWindowChange(const FGameplayTag Tag, bool bIsOpen, UAnimMontage* SourceMontage);
	
	bool CanExecuteSwitchAction(const UCombatActionStep* Step) const;

	// Meets cost and required tags
	bool MeetsActionRequirements(const UCombatActionStep* Step) const;

	int32 ExecuteUltimateAction(const FCombatActionContext& Context);
	
	AEnemyCharacterBase* FindClosestEnemy(const float MaxDistance);

	void NotifyActionLogicFinished(const FGameplayTag& Tag);

	bool IsCurrentActionLogicFinished() const;
	
	void ProcessFrameInput(const FPlayerInputs& FrameInputs);
private:
	// Input
	void ProcessInputAction(const FPlayerInputs& FrameInputs);
	
	void ProcessBufferedInput(const FPlayerInputs& FrameInputs);
	
	// Action
	void InitializeCombatStepList();
	
	UCombatActionStep* SelectTargetAction(const FPlayerInputs& FrameInputs);
	
	UCombatActionStep* SelectComboActionIntent(const EInputAction Input);
	
	UCombatActionStep* SelectCombatActionIntent(const EInputAction Input);
	
	void BufferInputIntent(const UCombatActionStep* ActionToBuffer);

	virtual int32 ExecuteAction(const UCombatActionStep* ActionStep, const FCombatActionContext& Context) override;

	void TryApplyMotionWarpingIfNeeded(const UCombatActionStep* ActionStep, const AEnemyCharacterBase* Enemy);

	void ApplyStaticPointMotionWarping(const FMotionWarpConfig& Config, const APlayerCharacter* Agent, const AEnemyCharacterBase* Enemy);
	
	void PayActionCost(const UCombatActionStep* Step);
	
public:
	FOnActionLogicFinished OnActionLogicFinished;
	
private:
	float GlobalBufferLifespan{0.3f}; 
	
	UPROPERTY()
	TObjectPtr<UGameplayEffect> ActionCostGE{nullptr};

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAgentCombatSteps> AgentCombatSteps{nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "Combat | Special Action")
	TObjectPtr<UCombatActionStep> ChainAttackAction{nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "Combat | Special Action")
	TObjectPtr<UCombatActionStep> DefensiveAssistAction{nullptr};
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat | Special Action")
	TObjectPtr<UCombatActionStep> QuickAssistAction{nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "Combat | Special Action")
	TObjectPtr<UCombatActionStep> UltimateAction{nullptr};
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> SwitchInMontage;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> SwitchOutMontage;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCombatActionStep> SwitchInAction{nullptr};

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCombatActionStep> SwitchOutAction{nullptr};
	
	UPROPERTY()
	FBufferedIntent PendingIntent;

	UPROPERTY(Transient)
	TArray<UCombatActionStep*> CombatActionList;
};

