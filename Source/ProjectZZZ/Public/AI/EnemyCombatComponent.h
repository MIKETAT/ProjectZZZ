// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Component/CombatComponentBase.h"
#include "EnemyCombatComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API UEnemyCombatComponent : public UCombatComponentBase
{
	GENERATED_BODY()

public:
	UEnemyCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	virtual int32 ExecuteAction(const UCombatActionStep* ActionStep) override;

	float GetCurrentActionParryOffset() const;
	
	virtual void HandleIncomingDamage(const FAttackContext& Context, FAttackResult& Result) override;
	
	virtual void ProcessHitFeedback(const FAttackResult& Result) override;

	virtual void InjectAndBindASC(UAgentAbilitySystemComponent* InASC) override;

	bool IsDazeValueFull() const;
	
	void OnDazeChanged(const FOnAttributeChangeData& Data);
	
	void HandleStun();
};
