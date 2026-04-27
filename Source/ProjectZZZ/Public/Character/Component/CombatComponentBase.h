// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "CombatComponentBase.generated.h"


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


UCLASS(Abstract)
class PROJECTZZZ_API UCombatComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponentBase();
public:
	//virtual UAbilitySystemComponent* GetAbilitySystemComponent() const PURE_VIRTUAL(UCombatComponentBase::GetAbilitySystemComponent, return nullptr;);
	
	virtual void ProcessHitEvent(AActor* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config/*, UCombatActionStep* SourceAction*/) PURE_VIRTUAL(UCombatComponentBase::ProcessHitEvent,);

	virtual void HandleIncomingDamage(const FAttackContext& Context, FAttackResult& OutResult) PURE_VIRTUAL(UCombatComponentBase::HandleIncomingDamage,);
	
	virtual void ProcessHitFeedback(const FAttackResult& Result) PURE_VIRTUAL(UCombatComponentBase::ProcessHitFeedback,);

	virtual void EnableAttackDetection(UCombatActionStep* ActionStep, const FHitShapeConfig& ShapeConfig) PURE_VIRTUAL(UCombatComponentBase::EnableAttackDetection, )

	virtual void DisableAttackDetection() PURE_VIRTUAL(UCombatComponentBase::DisableAttackDetection, )
};
