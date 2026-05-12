// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SquadManagerComponent.generated.h"


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

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTriggerChainAttack, UTexture2D*, UTexture2D*);
DECLARE_MULTICAST_DELEGATE(FOnFinishChainAttack);

USTRUCT(BlueprintType)
struct FChainAttackWindowStatus
{
	GENERATED_BODY()
	
public:
	void ResetChainAttackWindow();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float QTEDuration{3.f};
	
	UPROPERTY()
	float QTERemainingTime{3.f};

	UPROPERTY()
	bool bActive{false};
};

UENUM(BlueprintType)
enum class EAgentSwitchOutMode : uint8
{
	ExitImmediately,
	ExitWithSwitchOutAnim,
	FinishActionThenExit
};

UENUM(BlueprintType)
enum class EAgentSwitchInMode : uint8
{
	InitialIdle,
	InheritLocomotion,
	EnterWithSwitchInAnim,
	ExecuteSpecialAction
};

UENUM(BlueprintType)
enum class EAgentSpawnPolicy : uint8
{
	AbsoluteTransform,
	RelativeRight,
	FaceTarget			// Chain Attack/ Parry Assist/ Quick Assist
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
	EAgentSwitchOutMode SwitchOutMode{EAgentSwitchOutMode::ExitWithSwitchOutAnim};

	UPROPERTY()
	EAgentSpawnPolicy SpawnPolicy{EAgentSpawnPolicy::AbsoluteTransform};

	UPROPERTY()
	FTransform SpawnTransform{FTransform::Identity};
	
	UPROPERTY()
	TObjectPtr<UCombatActionStep> SpecialActionToExecute;
	
	// Motion Warping
	UPROPERTY()
	TObjectPtr<AActor> MotionWarpTarget{nullptr};
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
	
	// Getter and Setter
	APlayerCharacter* GetActivePlayerCharacter() const;
	
	FChainAttackWindowStatus GetChainAttackWindowStatus() const { return ChainAttackStatus; }
private:
	// Switch Agent
	// Todo: 切换到后台的代理人动作只执行到逻辑结算结束就退到后台，不播放收刀之类的后摇动画
	// Todo: 运动状态下切换代理人, 应继承原运动状态(动画表现)
	void ExecuteAgentTransition(const FAgentTransitionRequest& Request);

	bool CanExecuteAgentTransition(const FAgentTransitionRequest& Request);

	void ApplyAgentState(APlayerCharacter* Agent, EAgentPresenceState State);

	void ApplyAgentActiveState(APlayerCharacter* Agent);

	void ApplyAgentLingeringState(APlayerCharacter* Agent);

	void ApplyAgentOffFieldState(APlayerCharacter* Agent);
		
	FAgentTransitionSnapshot CacheAgentSnapshot(APlayerCharacter* OldAgent);

	FAgentTransitionSnapshot GetInitialSnapshot();

	FTransform CalculateSwitchInTransform(const EAgentSpawnPolicy Policy, APlayerCharacter* OldAgent) const;

	void HandleAgentSwitchIn(APlayerCharacter* NewAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot);

	void HandleAgentSwitchOut(APlayerCharacter* OldAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot);

	void OnLingeringAgentActionFinished(APlayerCharacter* LingeringAgent);
	
	void SwitchToPreviousAgent();
	
	void SwitchToNextAgent();
	
	void SwitchToAgent(const int32 TargetIndex);

	void AgentChainAttack(const int32 TargetIndex);

private:
	
	// Squad 
	void InitializeAgentSquad();
		
	void BindAgentLingeringDelegate(APlayerCharacter* Agent);

	void UnBindAgentLingeringDelegate(APlayerCharacter* Agent);

	//void AgentSwapImplementation(const int32 AgentIndex = 0);

	APlayerCharacter* GetPreviousAgent() const;

	APlayerCharacter* GetNextAgent() const;
	
	int32 GetPreviousAgentIndex() const;
	
	int32 GetNextAgentIndex() const;

	// Input
	void RouteInput();

	bool SquadConsumeInput(FCharacterFrameDataBus& DataBus);

	void ConsumeChainAttackInput(FCharacterFrameDataBus& DataBus);
	
	void AgentConsumeInput(FCharacterFrameDataBus& DataBus);

	// Chain Attack QTE
	ECombatEventHandleResult TriggerChainAttackWindow(const FCombatEventMessage& CombatEventMessage);
	
	void CloseChainAttackWindow();
	
	void QTEAdvanceCountDown(float DeltaTime);

	//void OpenChainAttackWindow();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<APlayerCharacter>> SquadPreset;

	FOnTriggerChainAttack OnTriggerChainAttack;

	FOnFinishChainAttack OnFinishChainAttack;
private:
	UPROPERTY()
	TArray<APlayerCharacter*> Squad;

	UPROPERTY()
	TWeakObjectPtr<AZZZPlayerController> OwnerController{nullptr};

	int32 ActiveAgentIndex{INDEX_NONE};

	FDelegateHandle Handle;

	bool bChainAttackWindowActive{false};

	FChainAttackWindowStatus ChainAttackStatus;
};
