// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "CombatStep.generated.h"

class UGameplayEffect;
enum class EInputAction : uint8;

UCLASS(BlueprintType)
class PROJECTZZZ_API UCombatActionStep : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual UAnimMontage* GetAnimMontage(const FCharacterFrameDataBus& Data) const { return Montage; };

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> Montage;		// AnimSequence?

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EInputAction TriggerInput{EInputAction::EInputAction_Max};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActionName{TEXT("ActionNameDefault")};
	
	UPROPERTY()
	FGameplayTag ActionTag;			// Identify CombatActionStep
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat | Cost")
	TSubclassOf<UGameplayEffect> CostGameplayEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority{-1};		// Todo: Use Enum?

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<EInputAction, const TObjectPtr<UCombatActionStep>> ComboLinks;
	
	// Todo: Cost for Special_Attack or Ultimate or Dodge cooldown
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bIsHeavyAttack : 1 {false};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bIsBasicAttack : 1 {false};
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

	int32 Priority{INDEX_NONE};
	
public:
	bool IsValid() const { return ActionStep != nullptr; }

	void SetIntent(const UCombatActionStep* InCombatStep, const float InExpirationTime, const float InPriority)
	{
		ActionStep = InCombatStep;
		ExpirationTime = InExpirationTime;
		Priority = InPriority;
	}
	
	void Reset()
	{
		ActionStep = nullptr;
		ExpirationTime = 0.0f;
		Priority = INDEX_NONE;
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
		UE_LOG(LogTemp, Warning, TEXT("Combat Status Reset."));
	}
	
	UPROPERTY()
	const UCombatActionStep* CurrentStep{nullptr};

	int32 MontageInstanceId{INDEX_NONE};
	
	// Todo: use Bitmask
	uint8 bInputBufferWindowOpen : 1 {false};
	uint8 bProceedWindowOpen : 1 {false};
	uint8 bIsRecoveryWindowOpen : 1 {false};
	uint8 bParryWindowOpen  : 1 {false};	// only parry?
	uint8 bHasSuccessfullyStarted : 1 {false};
	uint8 bHasConfirmedNextAction : 1 {false};
};
