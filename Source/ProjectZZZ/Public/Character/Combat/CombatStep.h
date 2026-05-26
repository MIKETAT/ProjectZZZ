// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "CombatStep.generated.h"

class UGameplayEffect;
enum class EInputAction : uint8;

UENUM(BlueprintType)
enum class ECombatActionPriority : uint8
{
	None = 0					UMETA(DisplayName = "None"),
	Switch = 5					UMETA(DisplayName = "Switch"),
	
	BasicAttack = 10			UMETA(DisplayName = "Attack"),
	Dodge = 20					UMETA(DisplayName = "Dodge"),

	CounterAttack = 50			UMETA(DisplayName = "CounterAttack"),
	
	Ultimate = 80				UMETA(DisplayName = "Ultimate"),	
	
	HitReaction = 100			UMETA(DisplayName = "HitReaction"),
	Dead = 255					UMETA(DisplayName = "Dead"),
};

USTRUCT(BlueprintType)
struct FHitPayloadConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bEnableHitPayload{false};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	float DamageMultiplier{1.f};	// 伤害倍率

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	float DazeMultiplier{1.5f};		// 失衡倍率

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	bool bIsHeavyAttack{false};		// 重击效果
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	TSubclassOf<UGameplayEffect> ImpactEffectOnTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	TSubclassOf<UGameplayEffect> HitFeedbackEffectOnSelf;
};

USTRUCT(BlueprintType)
struct FMotionWarpConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bEnableMotionWarp{false};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bEnableMotionWarp"))
	float MotionWarpingEffectiveDistance{100.f};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bEnableMotionWarp"))
	FName WarpTargetName{FName("Default")};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bEnableMotionWarp"))
	float StandOffDistance{0.f};
};

USTRUCT(BlueprintType)
struct FAssistActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bIsAssistAction{false};

	/*
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsAssistAction"))
	float ParryEnterDistance{0.f};*/

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsAssistAction"))
	float ParrySocketOffset{0.f};		// parry socket to root relative offset along forward direction

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsAssistAction"))
	FName WarpTargetName{FName("ParryTarget")};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsAssistAction"))
	FName SuccessSectionName{FName("Follow")};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsAssistAction"))
	float HitStopDuration{0.05f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsAssistAction"))
	float HitStopTimeScale{0.1f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsAssistAction"))
	TSubclassOf<UGameplayEffect> ParryEffectOnEnemy{nullptr};
	
	// todo: 要不要这个参数 
	/*UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsAssistAction", ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnBlendAlpha{0.6f};*/
};

USTRUCT(BlueprintType)
struct FParriedActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bIsParriedAction{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsParriedAction"))
	float ParryOffset{0.f};
};

UCLASS(BlueprintType)
class PROJECTZZZ_API UCombatActionStep : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual UAnimMontage* GetAnimMontage(const FCharacterFrameDataBus& Data) const { return Montage; };

public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Montage{nullptr};		// AnimSequence?
	
	UPROPERTY(EditDefaultsOnly)
	EInputAction TriggerInput{EInputAction::EInputAction_Max};

	UPROPERTY(EditDefaultsOnly)
	ECombatActionPriority Priority{ECombatActionPriority::None};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bIsBasicAttack : 1 {false};
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ActionTag{FGameplayTag::EmptyTag};

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer RequiredTags{FGameplayTag::EmptyTag};

	UPROPERTY(EditDefaultsOnly)
	TMap<EInputAction, TObjectPtr<UCombatActionStep>> ComboLinks;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	TSubclassOf<UGameplayEffect> CostGameplayEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | MotionWarp")
	FMotionWarpConfig WarpConfig;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | HitPayload")
	FHitPayloadConfig HitPayloadConfig;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | AssistConfig")
	FAssistActionConfig AssistConfig;

	// for enemy
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | ParriedActionConfig")
	FParriedActionConfig ParriedActionConfig;
};

UCLASS(BlueprintType)
class PROJECTZZZ_API UAgentCombatSteps : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<UCombatActionStep>> CombatSteps;
};

USTRUCT()
struct FBufferedIntent
{
	GENERATED_BODY()
	
	UPROPERTY()
	const UCombatActionStep* ActionStep{nullptr};

	float ExpirationTime{0.0f};

	ECombatActionPriority Priority{ECombatActionPriority::None};
	
public:
	bool IsValid() const { return ActionStep != nullptr; }

	void SetIntent(const UCombatActionStep* InCombatStep, const float InExpirationTime, const ECombatActionPriority InPriority)
	{
		ActionStep = InCombatStep;
		ExpirationTime = InExpirationTime;
		Priority = InPriority;
	}
	
	void Reset()
	{
		ActionStep = nullptr;
		ExpirationTime = 0.0f;
		Priority = ECombatActionPriority::None;
	}
};

USTRUCT()
struct PROJECTZZZ_API FCombatExecutionState
{
	GENERATED_BODY()
	
public:
	void Reset()
	{
		CurrentStep = nullptr;
		MontageInstanceId = INDEX_NONE;

		bInputBufferWindowOpen = false;
		bProceedWindowOpen = false;
		bIsRecoveryWindowOpen = false;
		bParryWindowOpen = false;
		bHasConfirmedNextAction = false;
		bHasSuccessfullyStarted = false;
	}
	
	UPROPERTY()
	TObjectPtr<const UCombatActionStep> CurrentStep{nullptr};

	int32 MontageInstanceId{INDEX_NONE};
	
	// Todo: use Bitmask
	uint8 bInputBufferWindowOpen : 1 {false};
	uint8 bProceedWindowOpen : 1 {false};
	uint8 bMovementInterruptWindowOpen : 1 {false};
	uint8 bIsRecoveryWindowOpen : 1 {false};
	uint8 bParryWindowOpen  : 1 {false};	// only parry?
	uint8 bHasSuccessfullyStarted : 1 {false};
	uint8 bHasConfirmedNextAction : 1 {false};
};
