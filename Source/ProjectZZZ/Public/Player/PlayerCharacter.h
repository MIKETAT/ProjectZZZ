// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Character/Combat/CombatInterface.h"
#include "PlayerCharacter.generated.h"

struct FCombatEventMessage;
enum class ECombatEventHandleResult : uint8;

UCLASS()
class PROJECTZZZ_API APlayerCharacter : public ACharacterBase, public ICombatInterface
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
public:
	// Combat Interface
	virtual UAbilitySystemComponent* GetAbilitySystemComp() const override { return GetAbilitySystemComponent(); }
	// !Combat Interface
	
	UAgentAttributeSet* GetAgentAttributeSet() const { return AgentAttributeSet; }
	virtual TSubclassOf<UGameplayEffect> GetExclusiveInitGE() const override { return AgentExclusiveInitGE; }


	ECombatEventHandleResult HandleEnemyDeath(const FCombatEventMessage& Msg);

	
private:
	void ProcessMovementInput(float DeltaTime);
	void ProcessLookInput(float DeltaTime);
	void ProcessCombatActionInput(float DeltaTime);

public:
// Components
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPlayerInputHandlerComponent> PlayerInputHandlerComponent{nullptr};
	
	// 暂时使用默认的相机
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> AgentExclusiveInitGE;
private:
	UPROPERTY()
	TObjectPtr<UAgentAttributeSet> AgentAttributeSet;

	FDelegateHandle DeathListenerHandle;
};
