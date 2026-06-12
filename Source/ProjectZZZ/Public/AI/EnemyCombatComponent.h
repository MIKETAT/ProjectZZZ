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
	virtual int32 ExecuteAction(const UCombatActionStep* ActionStep, const FCombatActionContext& Context) override;
	
	float GetCurrentActionParryOffset() const;
	
	virtual void HandleIncomingDamage(const FAttackContext& Context, FAttackResult& Result) override;
	
	virtual void ProcessHitFeedback(const FAttackResult& Result) override;

	virtual void InjectAndBindASC(UAgentAbilitySystemComponent* InASC) override;

	bool IsDazeValueFull() const;
	
	void OnDazeChanged(const FOnAttributeChangeData& Data);

	UFUNCTION(BlueprintCallable)
	bool IsStunned() const { return bIsStunned; };
	
	void EnterStunState();

	void ExitStunState();

	void UpdateBlackBoardStunState();

private:
	bool bIsStunned{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float StunDuration{3.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> ResetDazeGE;
	
	FTimerHandle StunRecoveryTimerHandle;
};
