#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "EnemyCharacterBase.generated.h"

class UWidgetComponent;
class UEnemyCombatComponent;
class UEnemyAttributeSet;

UCLASS()
class PROJECTZZZ_API AEnemyCharacterBase : public ACharacterBase
{
	GENERATED_BODY()

public:
	AEnemyCharacterBase();
	
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
protected:
	virtual void BeginPlay() override;

	// Combat Interface
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComp() const override { return GetAbilitySystemComponent(); }
	
	// ~Combat Interface

	//virtual TSubclassOf<UGameplayEffect> GetExclusiveInitGE() const override { return EnemyExclusiveInitGE; }

	UEnemyCombatComponent* GetEnemyCombatComponent() const { return EnemyCombatComponent; }

	virtual void InitializeAttributes() override;
	
	// Test
	virtual void Die() override;

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnParryWindowOpened();
	
private:
	void PrintDebugInfo();
	
	void PrintAttributeSet(UAttributeSet* Attribute);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> EnemyExclusiveInitGE;

	UPROPERTY()
	TObjectPtr<UEnemyAttributeSet> EnemyAttributeSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bPrintDebugInfo{false};

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyCombatComponent> EnemyCombatComponent{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> ParryFlashWidget{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	FName ParrySocketName{FName("Bip001Head")};
};
