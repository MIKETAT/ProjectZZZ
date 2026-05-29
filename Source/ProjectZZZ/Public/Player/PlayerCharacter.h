// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "PlayerCharacter.generated.h"

class UImage;
class AZZZPlayerController;
struct FCombatEventMessage;
enum class ECombatEventHandleResult : uint8;

UENUM(BlueprintType)
enum class EAgentPresenceState : uint8
{
	Active,
	Lingering,
	OffField
};

UCLASS()
class PROJECTZZZ_API APlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* NewController) override;
	
	virtual void UnPossessed() override;

	virtual void InitializeAttributes() override;
public:
	bool IsAnyActionActive() const { return AgentCombatComponent && AgentCombatComponent->IsAnyActionActive(); };

	bool IsMoving() const;

	bool IsActive() const { return bIsActive; }

	void SetAgentActive(bool bActive) { bIsActive = bActive; }
	
	bool HasMovementInput() const { return CharacterFrameDataBus.HasMovementInput(); }
	
	UTexture2D* GetAgentHead() const { return AgentHead; }

	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }
	
	UCharacterCombatComponent* GetAgentCombatComponent() const { return AgentCombatComponent; }
	
	EAgentPresenceState GetAgentPresence() const { return AgentPresenceState; };

	void SetAgentPresence(const EAgentPresenceState NewPresence) { AgentPresenceState = NewPresence; }
	
	UAgentAttributeSet* GetAgentAttributeSet() const { return AgentAttributeSet; }
	
	//virtual TSubclassOf<UGameplayEffect> GetExclusiveInitGE() const override { return AgentExclusiveInitGE; }

	const FCharacterFrameDataBus& GetCharacterFrameDataBus() const { return CharacterFrameDataBus; }

	void RefreshCharacterFrameInputData(const FCharacterFrameDataBus& DataBus) { CharacterFrameDataBus.PlayerInputs = DataBus.PlayerInputs; }

	ECombatEventHandleResult HandleEnemyDeath(const FCombatEventMessage& Msg);

	void SwitchToOnField();
	
	void SwitchToOffField();
	
	UCombatActionStep* GetSpecialAction(const FGameplayTag& Tag) const { return AgentCombatComponent ? AgentCombatComponent->GetSpecialAction(Tag) : nullptr;};
	
private:
	void ProcessMovementInput(float DeltaTime);
	
	void ProcessLookInput(float DeltaTime);
	
	void ProcessCombatActionInput(float DeltaTime);

public:
// Components
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterCombatComponent> AgentCombatComponent{nullptr};

	UPROPERTY()
	TWeakObjectPtr<AZZZPlayerController> OwnerController;
	
	FDelegateHandle DeathListenerHandle;		//

	EAgentPresenceState AgentPresenceState{EAgentPresenceState::OffField};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FCharacterFrameDataBus CharacterFrameDataBus;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> AgentHead;

	bool bIsActive{false};
};
