#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Character/CharacterFrameDataBus.h"
#include "Combat/CombatInterface.h"
#include "GameFramework/Character.h"
#include "State/LocomotionState.h"
#include "CharacterBase.generated.h"

class UHitDetectionComponent;
class UHitStopComponent;
class UMotionWarpingComponent;
class UCombatComponentBase;
class UBaseCombatAttributeSet;
class UCombatAnimSchedulerComponent;
class UGameplayEffect;
class UAgentAbilitySystemComponent;
class UAgentAttributeSet;
class UCharacterCombatComponent;
struct FCharacterFrameDataBus;
class UPlayerInputHandlerComponent;
struct FInputActionValue;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;

UENUM(BlueprintType)
enum class ECharacterPresentationState : uint8
{
	ActiveVisible,
	ActiveInvisible,
	InactiveHidden
};

UCLASS()
class PROJECTZZZ_API ACharacterBase : public ACharacter, public ICombatInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void UnPossessed() override;
	
	virtual void PostInitializeComponents() override;
	// ~Interface
	
	// GAS Interface
public:
	UAbilitySystemComponent* GetAbilitySystemComponent() const { return AgentAbilitySystemComponent.Get(); }
	// !GAS Interface

	// ICombatInterface
	UFUNCTION(BlueprintCallable)
	virtual UAbilitySystemComponent* GetAbilitySystemComp() const override { return AgentAbilitySystemComponent.Get(); }
	
	virtual UCombatComponentBase* GetCombatComp() const override { return CombatBase.Get(); }
	// ~ICombatInterface

public:
	const FLocomotionState& GetLocomotionState() const { return LocomotionState; }
	
	UBaseCombatAttributeSet* GetBaseCombatAttribute() const { return BaseCombatAttribute; }
	
	UCombatAnimSchedulerComponent* GetCombatAnimSchedulerComponent() const { return CombatAnimSchedulerComponent; }
	
	virtual UHitStopComponent* GetHitStopComponent() const { return HitStopComponent.Get(); }

	UHitDetectionComponent* GetHitDetectionComponent() const { return HitDetectionComponent.Get(); }

	UFUNCTION(BlueprintCallable)
	void AddTagToASC(FGameplayTag Tag);
	
public:
	virtual void Die() {};

	void SetCharacterState(const ECharacterPresentationState State);

	ECharacterPresentationState GetCharacterState() const { return CharacterState; }
	
protected:
	void RefreshInput(const float DeltaTime);
	
	void RefreshLocomotionState(const float DeltaTime);
	
	virtual void InitializeAttributes() PURE_VIRTUAL(ACharacterBase::InitializeAttributes, );

	void ApplyGameplayEffectToSelf(const TSubclassOf<UGameplayEffect>& Effect);
private:
	//virtual TSubclassOf<UGameplayEffect> GetExclusiveInitGE() const { return nullptr; };
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> BaseInitGE;
	
	// todo: 规范替代下面的Loco State 变量
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	uint8 bHasMovementInput : 1 {false};
	
protected:
	UPROPERTY()
	TObjectPtr<UCombatComponentBase> CombatBase;

	UPROPERTY()
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
	
	UPROPERTY()
	TObjectPtr<UAgentAbilitySystemComponent> AgentAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCombatAnimSchedulerComponent> CombatAnimSchedulerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHitDetectionComponent> HitDetectionComponent;
	
	UPROPERTY()
	TObjectPtr<UHitStopComponent> HitStopComponent;
	
	UPROPERTY()
	TObjectPtr<UBaseCombatAttributeSet> BaseCombatAttribute;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	FLocomotionState LocomotionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	ECharacterPresentationState CharacterState{ECharacterPresentationState::ActiveVisible};
	
};
