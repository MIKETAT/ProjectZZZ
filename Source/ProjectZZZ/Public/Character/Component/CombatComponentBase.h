#pragma once

#include "CoreMinimal.h"
#include "Character/Combat/AttackDetection.h"
#include "Character/Combat/CombatHitReactionAction.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "CombatComponentBase.generated.h"

class USquadManagerComponent;
class APlayerCharacter;
class UCharacterCombatComponent;
class UCombatAnimSchedulerComponent;
enum ECombatAnimRequestFinishReason : uint8;
struct FOnAttributeChangeData;
class UAnimInstanceBase;
class UAgentAbilitySystemComponent;
struct FHitShapeConfig;
class UAbilitySystemComponent;
class UCombatActionStep;
struct FHitPayloadConfig;

USTRUCT(BlueprintType)
struct FAttackContext
{
	GENERATED_BODY()

public:
	bool IsContextValid() const
	{
		return IsValid(Instigator) && IsValid(Target);	// todo: 根据最终结构体成员进行修改 
	}

public:
	UPROPERTY()
	TObjectPtr<AActor> Instigator{nullptr};

	UPROPERTY()
	TObjectPtr<AActor> Target{nullptr};

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC{nullptr};
	
	UPROPERTY()
	FHitResult HitResult;

	UPROPERTY()
	FVector HitDirection{FVector::ZeroVector};

	UPROPERTY()
	FHitPayloadConfig PayloadConfig;
};

UENUM(BlueprintType)
enum class EAttackResultType : uint8
{
	Invalid,
	Hit,
	Killed,
	Dodged,
	Parried,
};

USTRUCT(BlueprintType)
struct FAttackResult
{
	GENERATED_BODY()

	EAttackResultType ResultType{EAttackResultType::Invalid};

	UPROPERTY()
	TSubclassOf<UGameplayEffect> HitFeedbackEffectOnSelf{nullptr};

	UPROPERTY()
	TWeakObjectPtr<ACharacterBase> ParryInstigator{nullptr};
	
	UPROPERTY()
	float HitStopDuration{0.f};

	UPROPERTY()
	float HitStopTimeScale{1.f};
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatActionFinished, APlayerCharacter*, ECombatAnimRequestFinishReason)

UCLASS(Abstract)
class PROJECTZZZ_API UCombatComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponentBase();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	USquadManagerComponent* GetSquadManagerComponent() const;
	
public:
	virtual void ProcessHitFeedback(const FAttackResult& Result) PURE_VIRTUAL(UCombatComponentBase::ProcessHitFeedback,);
	
	virtual void ProcessHitEvent(ACharacterBase* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config/*, UCombatActionStep* SourceAction*/);// PURE_VIRTUAL(UCombatComponentBase::ProcessHitEvent,);

	virtual void HandleIncomingDamage(const FAttackContext& Context, FAttackResult& Result)  PURE_VIRTUAL(UCombatComponentBase::HandleIncomingDamage,);
	
	virtual void EnableAttackDetection(const FGameplayTag& Tag, UAttackDetectionConfig* DetectionConfig);// PURE_VIRTUAL(UCombatComponentBase::EnableAttackDetection, )

	virtual void DisableAttackDetection();// PURE_VIRTUAL(UCombatComponentBase::DisableAttackDetection, )

	bool IsAnyActionActive() const;
	
	void CancelCurrentAction();
	
	bool CanInterruptCurrentAction(const UCombatActionStep* Step) const;

	void RefreshAttackDetection(float DeltaTime);

	void SubStepAttackDetection(const FTransform& LastWeaponRootTransform, const FTransform& LastWeaponTipTransform,
								const FTransform& CurrentWeaponRootTransform, const FTransform& CurrentWeaponTipTransform);
	
	virtual void InjectAndBindASC(UAgentAbilitySystemComponent* InASC);
	
	virtual void BindExclusiveAttributes() PURE_VIRTUAL(UCombatComponentBase::BindExclusiveAttributes, );

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	void ApplyGameplayEffectOnTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const TSubclassOf<UGameplayEffect>& GE);

	void ApplyGameplayEffectOnSelf(UAbilitySystemComponent* ASC, const TSubclassOf<UGameplayEffect>& GE);

	void ApplyImpactEffectOnTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FAttackContext& Context);

	EHitReactionDirection CalculateHitReactionDirection(const FVector& AttackerLocation) const;

	void ExecuteHitReaction(const AActor* Instigator, const EAttackStrength Strength);
protected:
	void CachePointers();
	
	virtual int32 ExecuteAction(const UCombatActionStep* ActionStep, const FCombatActionContext& Context);
	
	UFUNCTION()
	void HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason);
	
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	void HandleDeath();

private:
	void DebugPrintCurrentActionState();
	
public:
	FOnCombatActionFinished OnCombatActionFinished;
	
	UPROPERTY(EditDefaultsOnly, Category = "HitReaction")
	TMap<EAttackStrength, FDirectionalHitReactionActions> HitReactionMap;
	
protected:
	UPROPERTY()
	TObjectPtr<UCombatAnimSchedulerComponent> CombatAnimSchedulerComponent{nullptr};
	
	UPROPERTY()
	TObjectPtr<ACharacterBase> Character{nullptr};

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh{nullptr};

	UPROPERTY()
	TObjectPtr<UAnimInstanceBase> AnimInstance{nullptr};
	
	UPROPERTY()
	TObjectPtr<UAgentAbilitySystemComponent> AbilitySystemComponent{nullptr};
	
	UPROPERTY()
	FCombatExecutionState CurrentExecutionState;
	
	// Attack Detection
	FAttackDetectionStatus DetectionStatus;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	FDetectionDebugConfig DebugConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowActionDebugInfo{false};
};
