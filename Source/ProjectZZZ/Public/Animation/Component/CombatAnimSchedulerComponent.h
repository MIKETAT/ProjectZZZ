#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "CombatAnimSchedulerComponent.generated.h"

class UHitDetectionComponent;
struct FMontageBlendSettings;
enum class ECombatActionPriority : uint8;
class ACharacterBase;
class UAnimInstanceBase;

UENUM(BlueprintType)
enum class EMontageStatusFlag : uint8
{
	EMontageStatus_None				= 0					UMETA(DisplayName = "None"),
	EMontageStatus_Started			= 1 << 1			UMETA(DisplayName = "Started"),
	EMontageStatus_BlendingOut		= 1 << 2			UMETA(DisplayName = "BlendingOut"),
	//EMontageStatus_Finished			= 1 << 3			UMETA(DisplayName = "Finished"),
	//EMontageStatus_Interrupted		= 1 << 4			UMETA(DisplayName = "Interrupted"),
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
	
public:
	bool IsValid() const { return Montage.IsValid() && PlayRate > 0.f; }

	TWeakObjectPtr<UAnimMontage> Montage{nullptr};
	
	float PlayRate{1.f};

	int32 RequestID{INDEX_NONE};

	uint8 bIsFinished : 1 {false};

	uint8 MontageEventFlags{0U};	// 记录这个动画底层状态

	uint8 bUseWeaponSweepDetection : 1 {false};
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
	virtual void InitializeComponent() override;

public:
	int32 ExecuteAnimRequest(const FCombatAnimExecutionRequest& Request);

	void CancelAnimRequest(const int32 RequestID);

	void CancelAnimRequestWithBlendOutSetting(const int32 RequestID, const FMontageBlendSettings& BlendOutSetting);

	bool RequestMontageSetNextSection(const int32 RequestID, const FName& LoopSectionName, const FName& NextSectionName);
	
private:
	void CachePointers();
	
	bool IsRequestMontageBlendingOut(const FCombatAnimExecutionRequest* Request) const;

	bool CheckIfRequestMontageAlreadyPlaying(const FCombatAnimExecutionRequest& Request) const;

	void CollectConflictingAnimRequest(const FCombatAnimExecutionRequest& Request, TArray<int32>& OutConflictingRequestIDs) const;

	void FinishRequest(const int32 RequestID, const ECombatAnimRequestFinishReason Reason);

	FName GetMontageGroupName(const UAnimMontage* Montage) const;

	FMontageBlendSettings GetMontageBlendInSetting(const UAnimMontage* Montage) const;

	FMontageBlendSettings GetMontageBlendOutSetting(const UAnimMontage* Montage) const;

	void BindMontageNativeDelegates(UAnimMontage* Montage, const int32 Id);
	
	UFUNCTION()
	void HandleMontageStarted(UAnimMontage* Montage, int32 RequestID);
	
	UFUNCTION()
	void HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 RequestID);

	UFUNCTION()
	void HandleMontageEnd(UAnimMontage* Montage, bool bInterrupted, int32 RequestID);

public:
	UPROPERTY(BlueprintAssignable)
	FOnCombatAnimFinished OnAnimRequestFinished;
	
private:
	UPROPERTY(Transient)
	TMap<int32, FCombatAnimExecutionRequest> ProceedingRequests;

	int32 NextIDGenerator{-1};
	
	UPROPERTY()
	TObjectPtr<UAnimInstanceBase> AnimInstance{nullptr};

	UPROPERTY()
	TObjectPtr<ACharacterBase> Character{nullptr};

	UPROPERTY()
	TWeakObjectPtr<UHitDetectionComponent> HitDetectionComponent{nullptr};
};
