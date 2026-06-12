// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "CombatAnimSchedulerComponent.generated.h"

enum class ECombatActionPriority : uint8;
class ACharacterBase;
class UAnimInstanceBase;

UENUM(BlueprintType)
enum class EMontageStatusFlag : uint8
{
	EMontageStatus_None				= 0					UMETA(DisplayName = "None"),
	EMontageStatus_Started			= 1 << 1			UMETA(DisplayName = "Started"),
	EMontageStatus_BlendingOut		= 1 << 2			UMETA(DisplayName = "BlendingOut"),
	EMontageStatus_Finished			= 1 << 3			UMETA(DisplayName = "Finished"),
	EMontageStatus_Interrupted		= 1 << 4			UMETA(DisplayName = "Interrupted"),
	EMontageStatus_Ended			= 1 << 5			UMETA(DisplayName = "Ended"),
};

UENUM(BlueprintType)
enum ECombatAnimRequestFinishReason : uint8
{
	ERequestFinishReason_CompleteNormally,
	ERequestFinishReason_Interrupted,
	ERequestFinishReason_Cancelled
};

USTRUCT(BlueprintType)
struct FCombatAnimExecutionRequest
{
	GENERATED_BODY()

	UPROPERTY()
	UAnimMontage* Montage{nullptr};

	UPROPERTY()
	ECombatActionPriority Priority{ECombatActionPriority::None};

	UPROPERTY()
	float PlayRate{1.f};

	int32 RequestID{INDEX_NONE};

	uint8 bIsFinished : 1 {false};

	uint8 MontageEventFlags{0U};	// 记录这个动画底层状态

public:
	bool IsValid() const { return Montage != nullptr; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatAnimFinished, int32, RequestID, ECombatAnimRequestFinishReason, Reason);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API UCombatAnimSchedulerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatAnimSchedulerComponent();

protected:
	virtual void BeginPlay() override;
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;

public:
	int32 ExecuteAnimRequest(const FCombatAnimExecutionRequest& Request);

	void CancelAnimRequest(const int32 RequestID);

	bool RequestMontageSetNextSection(const int32 RequestID, const FName& LoopSectionName, const FName& NextSectionName);

	//bool CanInterruptCurrentAction() const;
private:
	bool IsRequestMontageBlendingOut(const FCombatAnimExecutionRequest* Request) const;

	int32 CheckIfRequestMontageAlreadyPlaying(const FCombatAnimExecutionRequest& Request) const;

	bool CanExecuteCombatAnimRequest(const FCombatAnimExecutionRequest& Request, TArray<int32>& PendingStopRequestIDs) const;

	// Reason?
	void FinishRequest(const int32 RequestID, const ECombatAnimRequestFinishReason Reason);

	FName GetMontageSlotName(const UAnimMontage* Montage) const;

	FMontageBlendSettings GetMontageBlendInSetting(const UAnimMontage* Montage) const;

	void BindMontageNativeDelegates(UAnimMontage* Montage, const int32 Id);
	
	UFUNCTION()
	void HandleMontageStarted(UAnimMontage* Montage, int32 RequestID);
	
	UFUNCTION()
	void HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 RequestID);

	UFUNCTION()
	void HandleMontageEnd(UAnimMontage* Montage, bool bInterrupted, int32 RequestID);

	void RefreshProceedingRequest();

public:
	UPROPERTY(BlueprintAssignable)
	FOnCombatAnimFinished OnAnimRequestFinished;
private:
	UPROPERTY()
	TMap<int32, FCombatAnimExecutionRequest> ProceedingRequests;

	int32 NextIDGenerator{-1};
	
	UPROPERTY()
	TObjectPtr<UAnimInstanceBase> AnimInstance;

	UPROPERTY()
	TObjectPtr<ACharacterBase> Character;
};
