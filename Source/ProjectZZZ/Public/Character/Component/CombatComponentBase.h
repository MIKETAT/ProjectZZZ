#pragma once

#include "CoreMinimal.h"
#include "Character/Combat/AttackDetection.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "CombatComponentBase.generated.h"


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
		return IsValid(Instigator);	// todo: 根据最终结构体成员进行修改 
	}

public:
	UPROPERTY()
	TObjectPtr<AActor> Instigator{nullptr};

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC{nullptr};
	
	UPROPERTY()
	FHitResult HitResult;

	UPROPERTY()
	FVector HitDirection{FVector::ZeroVector};

	UPROPERTY()
	FHitPayloadConfig PayloadConfig;
	
	/*UPROPERTY()
	UCombatActionStep* SourceAction{nullptr};*/
};

UENUM(BlueprintType)
enum class EDamageResolveType : uint8
{
	Invalid,
	Dodged,
	Parried,
	Blocked,
	Immune,
	Hit,
	Kill
};

USTRUCT(BlueprintType)
struct FAttackResult
{
	GENERATED_BODY()

	bool bWasDodged{false};

	bool bWasParried{false};

	bool bWasDead{false};
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatActionFinished, class APlayerCharacter*);

UCLASS(Abstract)
class PROJECTZZZ_API UCombatComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponentBase();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
public:
	virtual void ProcessHitFeedback(const FAttackResult& Result) PURE_VIRTUAL(UCombatComponentBase::ProcessHitFeedback,);
	
	virtual void ProcessHitEvent(ACharacterBase* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config/*, UCombatActionStep* SourceAction*/);// PURE_VIRTUAL(UCombatComponentBase::ProcessHitEvent,);

	virtual void HandleIncomingDamage(const FAttackContext& Context, FAttackResult& OutResult);// PURE_VIRTUAL(UCombatComponentBase::HandleIncomingDamage,);
	
	virtual void EnableAttackDetection(const FHitShapeConfig& ShapeConfig);// PURE_VIRTUAL(UCombatComponentBase::EnableAttackDetection, )

	virtual void DisableAttackDetection();// PURE_VIRTUAL(UCombatComponentBase::DisableAttackDetection, )

	bool IsAnyActionActive() const;
	
	void CancelCurrentAction();
	
	bool CanInterruptCurrentAction(const UCombatActionStep* Step) const;

	// AttackDetection
	FTransform CalculateShapeWorldTransform() const;

	void RefreshAttackDetection(float DeltaTime);
	
	void RefreshWeaponSweepDirection(float DeltaTime);
	
	virtual void InjectAndBindASC(UAgentAbilitySystemComponent* InASC);
	
	virtual void BindExclusiveAttributes() PURE_VIRTUAL(UCombatComponentBase::BindExclusiveAttributes, ); 
protected:
	void CachePointers();
	
	virtual int32 ExecuteAction(const UCombatActionStep* ActionStep);
	
	UFUNCTION()
	void HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason);
	
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	void HandleDeath();
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> DamageEffectClass;	// todo
	
	FOnCombatActionFinished OnCombatActionFinished;
protected:
	UPROPERTY()
	TObjectPtr<UCombatAnimSchedulerComponent> CombatAnimSchedulerComponent{nullptr};

	
	
protected:
	UPROPERTY()
	TObjectPtr<APlayerCharacter> Character{nullptr};

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh{nullptr};

	UPROPERTY()
	TObjectPtr<UAnimInstanceBase> AnimInstance{nullptr};
	
	UPROPERTY()
	TObjectPtr<UAgentAbilitySystemComponent> AbilitySystemComponent{nullptr};

	
	UPROPERTY()
	FCombatExecutionState CurrentExecutionState;
	
	// Attack Detection
	FAttackDetectionConfig AttackDetectionConfig;
};
