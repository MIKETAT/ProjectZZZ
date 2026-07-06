#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/PlayerCharacter.h"
#include "SquadManagerComponent.generated.h"

class UCameraRigAsset;
class AEnemyCharacterBase;
class UCombatComponentBase;
class UCharacterCombatComponent;
enum class EAgentPresenceState : uint8;
class UCombatActionStep;
struct FCombatEventMessage;
enum class ECombatEventHandleResult : uint8;
struct FCharacterFrameDataBus;
class AZZZPlayerController;
class APlayerCharacter;
class UImage;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTriggerChainAttackWindow, UTexture2D*, UTexture2D*);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTriggerQuickAssistWindow, UTexture2D*);

DECLARE_MULTICAST_DELEGATE(FOnFinishChainAttack);

DECLARE_MULTICAST_DELEGATE(FOnFinishQuickAssist);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnActiveAgentChanged, APlayerCharacter*/** OldAgent */,  APlayerCharacter*/** NewAgent */)

USTRUCT(BlueprintType)
struct FChainAttackWindowStatus
{
	GENERATED_BODY()
	
public:
	void ResetChainAttackWindow();
	
public:
	UPROPERTY()
	TObjectPtr<AEnemyCharacterBase> Enemy{nullptr};
	
	UPROPERTY(EditDefaultsOnly)
	float QTEDuration{3.f};
	
	UPROPERTY()
	float QTERemainingTime{3.f};

	UPROPERTY()
	bool bActive{false};
};

USTRUCT()
struct FPerfectAssistWindowStatus
{
	GENERATED_BODY()
	
	void ResetPerfectAssistWindow();
	
	UPROPERTY()
	bool bPerfectAssistWindowOpen{false};
	
	UPROPERTY()
	TObjectPtr<AEnemyCharacterBase> TargetEnemy{nullptr};

	UPROPERTY()
	float ParryReferenceOffset{0.f};
};

USTRUCT()
struct FQuickAssistWindowStatus
{
	GENERATED_BODY()
	
	void ResetQuickAssistWindow();
	
	bool bQuickAssistWindowOpen{false};
	
	UPROPERTY()
	TObjectPtr<AEnemyCharacterBase> TargetEnemy{nullptr};
	
	UPROPERTY()
	float QuickAssistRemainingTime{2.f};

	UPROPERTY()
	float QuickAssistCountDownDuration{2.f};
};

UENUM(BlueprintType)
enum class EAgentSwitchOutMode : uint8
{
	ExitImmediately,
	ExitWithSwitchOutAnim,
	FinishActionThenExit,
	None		// for initial spawn
};

UENUM(BlueprintType)
enum class EAgentSwitchInMode : uint8
{
	InitialIdle,
	InheritLocomotion,
	EnterWithSwitchInAnim,
	ExecuteDefensiveAssist,
	ExecuteChainAttack,
	ExecuteQuickAssist,
};

UENUM(BlueprintType)
enum class EAgentSpawnPolicy : uint8
{
	InitialSpawn,
	AgentRelativeLeft,
	AgentRelativeRight,
	ChainAttackLeft,
	ChainAttackRight,
	ParryAssistFacingTarget,
	QuickAssistFacingTarget
};

USTRUCT(BlueprintType)
struct FAgentTransitionRequest
{
	GENERATED_BODY()

	UPROPERTY()
	int32 TargetAgentIndex{INDEX_NONE};

	UPROPERTY()
	EAgentSwitchInMode SwitchInMode{EAgentSwitchInMode::EnterWithSwitchInAnim};

	UPROPERTY()
	EAgentSwitchOutMode SwitchOutMode{EAgentSwitchOutMode::None};

	UPROPERTY()
	EAgentSpawnPolicy SpawnPolicy{EAgentSpawnPolicy::InitialSpawn};
	
	UPROPERTY()
	TObjectPtr<UCombatActionStep> SpecialActionToExecute;
	
	UPROPERTY()
	TObjectPtr<AEnemyCharacterBase> Enemy{nullptr};

	UPROPERTY()
	TObjectPtr<APlayerCharacter> CurrentAgent{nullptr};
};

USTRUCT()
struct FAgentTransitionSnapshot
{
	GENERATED_BODY()

	FVector Velocity{FVector::ZeroVector};

	EMovementMode MovementMode{MOVE_Walking};

	bool bWasMoving{false};

	bool bHasActiveAction{false};
};

UENUM(BlueprintType)
enum class EHUDSquadDisplaySlot : uint8
{
	Active			UMETA(DisplayName = "Active"),
	Second		UMETA(DisplayName = "Second"),
	Third			UMETA(DisplayName = "Third"),
};

USTRUCT(BlueprintType)
struct FHUDSquadAgentSource
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EHUDSquadDisplaySlot DisplaySlot{EHUDSquadDisplaySlot::Active};

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<APlayerCharacter> Agent{nullptr};

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UCharacterCombatComponent> CombatComponent{nullptr};
};

USTRUCT(BlueprintType)
struct FHUDSquadSource
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FHUDSquadAgentSource ActiveAgent;

	UPROPERTY(BlueprintReadOnly)
	FHUDSquadAgentSource SecondAgent;

	UPROPERTY(BlueprintReadOnly)
	FHUDSquadAgentSource ThirdAgent;

	UPROPERTY(Transient)
	TWeakObjectPtr<USquadManagerComponent> SquadManager{nullptr};
};

USTRUCT()
struct FAgentStatusSnapShot
{
	GENERATED_BODY()

public:
	float CurrentHealth{0.0f};

	float MaxHealth{0.0f};

	float CurrentEnergy{0.0f};

	float MaxEnergy{0.0f};

	float CurrentDecibels{0.0f};

	float MaxDecibels{0.0f};

	bool bCanExecuteSpecialAttackEX{false};

	bool bCanExecuteUltimate{false};
	
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerCharacter> Agent{nullptr};
	
	UPROPERTY(Transient)
	TWeakObjectPtr<UTexture2D> AgentHead{nullptr};
};

USTRUCT()
struct FSquadStatusSnapshot
{
	GENERATED_BODY()
	
	/*UPROPERTY(Transient)
	TWeakObjectPtr<APlayerCharacter> SecondAgent{nullptr};

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerCharacter> ThirdAgent{nullptr};


	UPROPERTY(Transient)
	TWeakObjectPtr<UTexture2D> SecondAgentHead{nullptr};

	UPROPERTY(Transient)
	TWeakObjectPtr<UTexture2D> ThirdAgentHead{nullptr};*/

	FAgentStatusSnapShot ActiveAgentStatus;

	FAgentStatusSnapShot SecondAgentStatus;

	FAgentStatusSnapShot ThirdAgentStatus;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API USquadManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USquadManagerComponent();
	
protected:
	virtual void BeginPlay() override;
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void InitializeComponent() override;
	
	// Squad 
	void InitializeAgentSquad();
	
	// Getter and Setter
	UFUNCTION(BlueprintCallable)
	APlayerCharacter* GetActiveAgent() const;

	APlayerCharacter* GetSecondAgent() const;

	APlayerCharacter* GetThirdAgent() const;

	APlayerCharacter* GetTargetAgent(const int32 TargetIndex) const;

	bool GetLockAgentSwitch() const { return bLockAgentSwitch; }

	void SetLockAgentSwitch(const bool bLock) { bLockAgentSwitch = bLock; }
	
	bool IsActiveAgentExecutingAction() const { return GetActiveAgent() && GetActiveAgent()->IsAnyActionActive(); }
	
	FChainAttackWindowStatus GetChainAttackWindowStatus() const { return ChainAttackStatus; }
	
	FTransform CalculateUltimateCameraPosition(UCombatActionStep* Ultimate, const FTransform& AgentTransform);

	void BuildHUDSquadSource(FHUDSquadSource& Source);

	FSquadStatusSnapshot BuildSquadStatusSnapshot();
	
private:
	// Switch Agent
	// Todo: 运动状态下切换代理人, 应继承原运动状态(动画表现)
	void ExecuteAgentTransition(const FAgentTransitionRequest& Request);

	bool CanExecuteAgentTransition(const FAgentTransitionRequest& Request);

	void ApplyAgentActiveState(APlayerCharacter* Agent);

	void ApplyAgentLingeringState(APlayerCharacter* Agent);

	void ApplyAgentOffFieldState(APlayerCharacter* Agent);
		
	FAgentTransitionSnapshot CacheAgentSnapshot(APlayerCharacter* OldAgent);

	FAgentTransitionSnapshot GetInitialSnapshot();

	void HandleAgentSwitchIn(APlayerCharacter* NewAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot);

	void HandleAgentSwitchOut(APlayerCharacter* OldAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot);

	void OnLingeringAgentActionFinished(APlayerCharacter* LingeringAgent, ECombatAnimRequestFinishReason Reason);

	void SwitchToAgent(const int32 TargetIndex, bool bIsPrevious, const FCharacterFrameDataBus& DataBus);

	void AgentChainAttack(const int32 TargetIndex, bool bIsPrevious);

	void AgentDefensiveAssist(const int32 TargetIndex, bool bIsPrevious);

	void AgentQuickAssist(const int32 TargetIndex);

	void AgentUltimateAttack();
		
	void BindAgentLingeringDelegate(APlayerCharacter* Agent);

	void UnBindAgentLingeringDelegate(APlayerCharacter* Agent);

	void OnPendingExitAgentActionLogicFinished(APlayerCharacter* Agent, int32 RequestId);

	APlayerCharacter* GetPreviousAgent() const;

	APlayerCharacter* GetNextAgent() const;
	
	int32 GetPreviousAgentIndex() const;
	
	int32 GetNextAgentIndex() const;

	// Input
	void RouteInput();

	bool SquadConsumeInput(FCharacterFrameDataBus& DataBus);

	void ConsumeChainAttackInput(FCharacterFrameDataBus& DataBus);

	bool ConsumePerfectAssistInput(FCharacterFrameDataBus& DataBus);

	bool ConsumeQuickAssistInput(FCharacterFrameDataBus& DataBus);
	
	void AgentConsumeInput(FCharacterFrameDataBus& DataBus);

	// Chain Attack QTE
	ECombatEventHandleResult TriggerChainAttackWindow(const FCombatEventMessage& CombatEventMessage);
	
	void CloseChainAttackWindow();
	
	void QTEAdvanceCountDown(float DeltaTime);

	void EnterChainAttackSlowMotion(UWorld* World);

	void ExitChainAttackSlowMotion(UWorld* World);

	// Perfect Assist
	ECombatEventHandleResult TriggerPerfectAssistWindow(const FCombatEventMessage& CombatEventMessage);

	// Quick Assist
	ECombatEventHandleResult TriggerQuickAssistWindow(const FCombatEventMessage& CombatEventMessage);

	void CloseQuickAssistWindow();
	
	void QuickAssistAdvanceCountDown(float DeltaTime);

	FTransform CalculateAgentSpawnTransform(const FAgentTransitionRequest& Request);

	void CalculateChainAttackSpawnTransform(const FAgentTransitionRequest& Request, FTransform& SpawnTransform);
	
	void CalculateParrySpawnTransform(const FAgentTransitionRequest& Request, FTransform& SpawnTransform);

	void CalculateQuickAssistSpawnTransform(const FAgentTransitionRequest& Request, FTransform& SpawnTransform);

	FTransform GetInitialSpawnTransform() const;

	// Camera
	FTransform CalculateActionCameraPosition(const FAgentTransitionRequest& Request);
	
	void PrepareParryAssistCameraContext(const FAgentTransitionRequest& Request, bool bIsPrevious);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<APlayerCharacter>> SquadPreset;

	FOnTriggerChainAttackWindow OnTriggerChainAttackWindow;
	
	FOnFinishChainAttack OnFinishChainAttack;

	FOnTriggerQuickAssistWindow OnTriggerQuickAssistWindow;

	FOnFinishQuickAssist OnFinishQuickAssist;

	FOnActiveAgentChanged OnActiveAgentChanged;
	
private:
	UPROPERTY()
	TArray<APlayerCharacter*> Squad;

	UPROPERTY()
	TWeakObjectPtr<AZZZPlayerController> OwnerController{nullptr};

	int32 ActiveAgentIndex{INDEX_NONE};

	FDelegateHandle ChainAttackHandle;

	FDelegateHandle PerfectAssistActiveHandle;

	FDelegateHandle PerfectAssistCloseHandle;

	FDelegateHandle QuickAssistHandle;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	FChainAttackWindowStatus ChainAttackStatus;

	FPerfectAssistWindowStatus PerfectAssistStatus;
	
	FQuickAssistWindowStatus QuickAssistStatus;
	
	TMap<TWeakObjectPtr<APlayerCharacter>, int32> PendingLingeringExitRequestIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bLockAgentSwitch{false};

	// Y 只取正值, 方向通过SpawnPolicy确定
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FVector ChainAttackCameraOffset{-150.f, 75.f, -15.f};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float ChainAttackSlowMotionScale{0.05f};
};
