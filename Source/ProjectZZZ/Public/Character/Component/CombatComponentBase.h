#pragma once

#include "CoreMinimal.h"
#include "Character/Combat/AttackDetection.h"
#include "Character/Combat/CombatHitReactionAction.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "CombatComponentBase.generated.h"

struct FAttackShapeQueryGeometry;
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

USTRUCT()
struct FAttackDetectedTarget
{
	GENERATED_BODY()
	
	TWeakObjectPtr<AActor> Actor{nullptr};

	TWeakObjectPtr<UPrimitiveComponent> Component{nullptr};

	FHitResult HitResult;

	bool bIsHitResult{false};

	FVector QueryLocation{FVector::ZeroVector};
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatActionFinished, APlayerCharacter*, ECombatAnimRequestFinishReason)

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float CurrentHealth, float MaxHealth);

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

	virtual bool ResolveAttackDetectionSegment(const FName& SegmentName, FResolvedAttackDetectionSegment& OutSegment);
	
	virtual void EnableAttackDetection(const UAnimSequenceBase* SourceAnimation, const FName& SegmentName);// PURE_VIRTUAL(UCombatComponentBase::EnableAttackDetection, )

	virtual void DisableAttackDetection(const UAnimSequenceBase* SourceAnimation, const FName& SegmentName);// PURE_VIRTUAL(UCombatComponentBase::DisableAttackDetection, )

	bool IsAnyActionActive() const;
	
	void CancelCurrentAction();
	
	bool CanInterruptCurrentAction(const UCombatActionStep* Step) const;

	void RefreshAttackDetection(float DeltaTime);

	void RefreshWeaponSweep(const float DeltaTime);

	void RefreshActorPathSweep();
	
	void TriggerAttackDetectionQuery(const UAnimSequenceBase* SourceAnimation, const FName& SegmentName);
	
	void RefreshShapeQueryContinuous();

	FAttackDetectedTarget MakeDetectedTargetFromHit(const FHitResult& HitResult);

	FAttackDetectedTarget MakeDetectedTargetFromOverlap(const FOverlapResult& OverlapResult, const FTransform& QueryTransform);

	void ProcessDetectionResults(const FAttackDetectedTarget& DetectedTarget, const FResolvedAttackDetectionSegment& Segment);

	bool PassHitDedupe(AActor* HitActor, const FResolvedAttackDetectionSegment& Segment);
	
	virtual void InjectAndBindASC(UAgentAbilitySystemComponent* InASC);
	
	virtual void BindExclusiveAttributes() PURE_VIRTUAL(UCombatComponentBase::BindExclusiveAttributes, );

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	void ApplyGameplayEffectOnTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const TSubclassOf<UGameplayEffect>& GE);

	void ApplyGameplayEffectOnSelf(UAbilitySystemComponent* ASC, const TSubclassOf<UGameplayEffect>& GE);

	void ApplyImpactEffectOnTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FAttackContext& Context);

	EHitReactionDirection CalculateHitReactionDirection(const FVector& AttackerLocation) const;

	void ExecuteHitReaction(const AActor* Instigator, const EAttackStrength Strength);
	
protected:
	virtual void CachePointers();
	
	virtual int32 ExecuteAction(const UCombatActionStep* ActionStep, const FCombatActionContext& Context);
	
	UFUNCTION()
	virtual void HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason);
	
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	void HandleDeath();

	bool IsSourceAnimationFromCurrentActionMontage(const UAnimSequenceBase* SourceAnimation) const;

private:
	void DebugPrintCurrentActionState();

	void DrawDebugAttackDetectionShape(const FAttackShapeQueryGeometry& Geometry);

	static void DrawDebugSweepShape(
		const UWorld* World,
		const FVector& Start,
		const FVector& End,
		const FQuat& TraceRotation,
		const FCollisionShape& Shape,
		const bool bHit,
		const TArray<FHitResult>& HitResults,
		const float LifeTime = 1.0f,
		const float Thickness = 1.5f);
	
public:
	FOnCombatActionFinished OnCombatActionFinished;

	FOnHealthChanged OnHealthChanged;
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	FDetectionDebugConfig DebugConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowActionDebugInfo{false};
};
