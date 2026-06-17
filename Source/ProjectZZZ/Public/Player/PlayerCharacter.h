#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "LevelSequence.h"
#include "PlayerCharacter.generated.h"

class ALevelSequenceActor;
class ULevelSequencePlayer;
struct FPendingUltimateCutInRequest;
class UGameplayCameraComponent;
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

USTRUCT(BlueprintType)
struct FPendingUltimateCutInRequest
{
	GENERATED_BODY()

	void Reset();
	
	TWeakObjectPtr<APlayerCharacter> Agent{nullptr};
	
	TWeakObjectPtr<UCombatActionStep> UltimateAction{nullptr};
	
	TWeakObjectPtr<ULevelSequence> CutInSequence{nullptr};
	
	FGameplayTag CameraStateTag{FGameplayTag::EmptyTag};
	
	FLinearColor BackgroundColor{FLinearColor::White};
	
	int32 StencilValue{42};
	
	bool bIsValid{false};
};

USTRUCT(BlueprintType)
struct FActiveUltimateExecutionState
{
	GENERATED_BODY()
	
public:
	void Reset();
	
	TWeakObjectPtr<APlayerCharacter> Agent{nullptr};
	
	TWeakObjectPtr<ULevelSequencePlayer> SequencePlayer{nullptr};
	
	TWeakObjectPtr<ALevelSequenceActor> SequenceActor{nullptr};
	
	FGameplayTag CameraStateTag{FGameplayTag::EmptyTag};

	FDelegateHandle OnUltimateActionFinishedHandle;
	
	FDelegateHandle OnSequenceFinishedHandle;
	
	bool bSequenceFinished{false};
	
	bool bIsValid{false};
	
	bool bActionFinished{false};
	
	bool bAborting{false};
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
	
	UTexture2D* GetAgentHead() const { return AgentHead; }

	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }
	
	UCharacterCombatComponent* GetAgentCombatComponent() const { return AgentCombatComponent; }
	
	EAgentPresenceState GetAgentPresence() const { return AgentPresenceState; };

	void SetAgentPresence(const EAgentPresenceState NewPresence) { AgentPresenceState = NewPresence; }
	
	UAgentAttributeSet* GetAgentAttributeSet() const { return AgentAttributeSet; }

	//const FCharacterFrameDataBus& GetCharacterFrameDataBus() const { return CharacterFrameDataBus; }

	//void RefreshCharacterFrameInputData(const FCharacterFrameDataBus& DataBus) { CharacterFrameDataBus.PlayerInputs = DataBus.PlayerInputs; }

	ECombatEventHandleResult HandleEnemyDeath(const FCombatEventMessage& Msg);

	void SwitchToOnField();
	
	void SwitchToOffField();
	
	UCombatActionStep* GetSpecialAction(const FGameplayTag& Tag) const { return AgentCombatComponent ? AgentCombatComponent->GetSpecialAction(Tag) : nullptr;};

	AEnemyCharacterBase* FindClosestEnemy(const float MaxDistance) const;
	
	void ProcessFrameInput(const FCharacterFrameDataBus& DataBus);
	
private:
	void ProcessMovementInput(const FCharacterFrameDataBus& DataBus);
	
	void ProcessLookInput(const FCharacterFrameDataBus& DataBus);
	
	void ProcessCombatActionInput(const FCharacterFrameDataBus& DataBus);

public:
// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGameplayCameraComponent> GameplayCamera;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> AgentExclusiveInitGE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPrintDebugInfo{false};
	
private:
	UPROPERTY()
	TObjectPtr<UAgentAttributeSet> AgentAttributeSet;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterCombatComponent> AgentCombatComponent{nullptr};

	UPROPERTY()
	TWeakObjectPtr<AZZZPlayerController> OwnerController;
	
	FDelegateHandle DeathListenerHandle;

	EAgentPresenceState AgentPresenceState{EAgentPresenceState::OffField};
	

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> AgentHead;

	bool bIsActive{false};

	bool bIsLocalPlayer{false};
};
