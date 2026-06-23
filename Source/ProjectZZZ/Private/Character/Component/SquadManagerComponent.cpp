#include "Character/Component/SquadManagerComponent.h"

#include "AI/EnemyCharacterBase.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Combat/ZZZCombatEventTypes.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "Character/Component/CombatCameraDirectorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/PlayerCharacter.h"
#include "Utility/ZZZGameplayTag.h"

void FChainAttackWindowStatus::ResetChainAttackWindow()
{
	QTEDuration = QTEDuration > 0.f ? QTEDuration : 3.f;
	QTERemainingTime = QTEDuration;
	Enemy = nullptr;
}

void FPerfectAssistWindowStatus::ResetPerfectAssistWindow()
{
	bPerfectAssistWindowOpen = false;
	ParryReferenceOffset = 0.f;
	TargetEnemy = nullptr;
}

void FQuickAssistWindowStatus::ResetQuickAssistWindow()
{
	bQuickAssistWindowOpen = false;
	QuickAssistCountDownDuration = QuickAssistCountDownDuration > 0.f ? QuickAssistCountDownDuration : 2.f;
	QuickAssistRemainingTime = QuickAssistCountDownDuration;
	TargetEnemy = nullptr;
}

USquadManagerComponent::USquadManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void USquadManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeAgentSquad();
	
	if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>())
	{
		FCombatEventDelegate ChainAttackCallback;
		ChainAttackCallback.BindUObject(this, &USquadManagerComponent::TriggerChainAttackWindow);
		ChainAttackHandle = EventBus->Subscribe(Combat::Event::ChainAttack, this, 1, ChainAttackCallback);

		FCombatEventDelegate PerfectAssistCallback;
		PerfectAssistCallback.BindUObject(this, &USquadManagerComponent::TriggerPerfectAssistWindow);
		PerfectAssistActiveHandle = EventBus->Subscribe(Combat::Event::PerfectAssist, this, 1, PerfectAssistCallback);

		FCombatEventDelegate QuickAssistCallback;
		QuickAssistCallback.BindUObject(this, &USquadManagerComponent::TriggerQuickAssistWindow);
		QuickAssistHandle = EventBus->Subscribe(Combat::Event::QuickAssist, this, 1, QuickAssistCallback);
	}
}

void USquadManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RouteInput();
	QTEAdvanceCountDown(DeltaTime);
	QuickAssistAdvanceCountDown(DeltaTime);
}

void USquadManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
	OwnerController = Cast<AZZZPlayerController>(GetOwner());
}

APlayerCharacter* USquadManagerComponent::GetActiveAgent() const
{
	if (Squad.IsValidIndex(ActiveAgentIndex))
	{
		return Squad[ActiveAgentIndex];
	}
	// not possible
	return nullptr;
}

APlayerCharacter* USquadManagerComponent::GetTargetAgent(const int32 TargetIndex) const
{
	if (Squad.IsValidIndex(TargetIndex))
	{
		return Squad[TargetIndex];
	}
	return nullptr;
}

void USquadManagerComponent::ExecuteAgentTransition(const FAgentTransitionRequest& Request)
{
	if (!CanExecuteAgentTransition(Request))
	{
		return;
	}
	
	APlayerCharacter* OldAgent{GetActiveAgent()};
	APlayerCharacter* TargetAgent{GetTargetAgent(Request.TargetAgentIndex)};
	
	const FAgentTransitionSnapshot Snapshot{CacheAgentSnapshot(OldAgent)};

	// 初始时无OldAgent
	if (OldAgent)
	{
		HandleAgentSwitchOut(OldAgent, Request, Snapshot);
	}
	
	checkf(TargetAgent, TEXT("Switch In Invalid Agent"));
	HandleAgentSwitchIn(TargetAgent, Request, Snapshot);
	ActiveAgentIndex = Request.TargetAgentIndex;
	OnActiveAgentChanged.Broadcast(OldAgent, TargetAgent);
	
	if (OldAgent)
	{
		OldAgent->SetAgentActive(false);
	}

	if (TargetAgent)
	{
		TargetAgent->SetAgentActive(true);
	}
}

bool USquadManagerComponent::CanExecuteAgentTransition(const FAgentTransitionRequest& Request)
{
	if (bLockAgentSwitch || !OwnerController.IsValid()
		|| !Squad.IsValidIndex(Request.TargetAgentIndex)
		|| Request.TargetAgentIndex == ActiveAgentIndex)
	{
		return false;
	}
	
	APlayerCharacter* TargetAgent{Squad[Request.TargetAgentIndex]};
	if (!IsValid(TargetAgent) || TargetAgent->GetAgentPresence() != EAgentPresenceState::OffField)
	{
		return false;
	}

	UCharacterCombatComponent* TargetCombatComponent{TargetAgent->GetAgentCombatComponent()};
	if (!IsValid(TargetCombatComponent))
	{
		return false;
	}
	if (Request.SpecialActionToExecute)
	{
		return TargetCombatComponent->MeetsActionRequirements(Request.SpecialActionToExecute);
	}
	return true;
}

void USquadManagerComponent::ApplyAgentActiveState(APlayerCharacter* Agent)
{
	if (!IsValid(Agent))
	{
		return;
	}
	
	Agent->SetAgentPresence(EAgentPresenceState::Active);
	Agent->SetActorHiddenInGame(false);
	Agent->SetActorEnableCollision(true);
}

void USquadManagerComponent::ApplyAgentLingeringState(APlayerCharacter* Agent)
{
	if (!IsValid(Agent))
	{
		return;
	}

	Agent->SetAgentPresence(EAgentPresenceState::Lingering);
	Agent->SetActorEnableCollision(false);
}

void USquadManagerComponent::ApplyAgentOffFieldState(APlayerCharacter* Agent)
{
	if (!IsValid(Agent))
	{
		return;
	}
	
	Agent->SetAgentPresence(EAgentPresenceState::OffField);
	Agent->SetActorHiddenInGame(true);
	Agent->SetActorEnableCollision(false);

	if (UCharacterMovementComponent* MovementComponent = Agent->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (UCharacterCombatComponent* CombatComponent = Agent->GetAgentCombatComponent())
	{
		CombatComponent->CancelCurrentAction();
	}
}

FAgentTransitionSnapshot USquadManagerComponent::CacheAgentSnapshot(APlayerCharacter* OldAgent)
{
	FAgentTransitionSnapshot Snapshot{GetInitialSnapshot()};

	if (!IsValid(OldAgent))
	{
		return Snapshot;
	}

	constexpr float MinSpeed{10.f};
	
	if (UCharacterMovementComponent* MovementComponent = OldAgent->GetCharacterMovement())
	{
		Snapshot.Velocity = MovementComponent->Velocity;
		Snapshot.MovementMode = MovementComponent->MovementMode;
		Snapshot.bWasMoving = MovementComponent->Velocity.SizeSquared2D() > FMath::Square(MinSpeed);
	}

	if (UCharacterCombatComponent* CombatComponent = OldAgent->GetAgentCombatComponent())
	{
		Snapshot.bHasActiveAction = CombatComponent->IsAnyActionActive();
	}
	return Snapshot;
}

FAgentTransitionSnapshot USquadManagerComponent::GetInitialSnapshot()
{
	FAgentTransitionSnapshot Snapshot;
	Snapshot.Velocity = FVector::ZeroVector;
	Snapshot.bHasActiveAction = false;
	Snapshot.bWasMoving = false;
	Snapshot.MovementMode = MOVE_Walking;
	return Snapshot;
}

void USquadManagerComponent::HandleAgentSwitchIn(APlayerCharacter* NewAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot)
{
	UCharacterCombatComponent* CombatComponent = NewAgent->GetAgentCombatComponent();
	OwnerController->Possess(NewAgent);

	ApplyAgentActiveState(NewAgent);
	
	NewAgent->SetActorTransform(CalculateAgentSpawnTransform(Request));
	
	if (CombatComponent->IsAnyActionActive())
	{
		CombatComponent->CancelCurrentAction();
	}
	
	UCharacterMovementComponent* MovementComponent = NewAgent->GetCharacterMovement();
	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
	}
	
	switch (Request.SwitchInMode) 
	{
		case EAgentSwitchInMode::InitialIdle:			// todo: 这里要想一想怎么做
		case EAgentSwitchInMode::InheritLocomotion:
			if (MovementComponent)
			{
				MovementComponent->SetMovementMode(Snapshot.MovementMode);
				MovementComponent->Velocity = Snapshot.Velocity;
			}
			break;
		case EAgentSwitchInMode::EnterWithSwitchInAnim:
			CombatComponent->ExecuteSwitchInAction();
			break;
		case EAgentSwitchInMode::ExecuteChainAttack:
		case EAgentSwitchInMode::ExecuteQuickAssist:
		case EAgentSwitchInMode::ExecuteDefensiveAssist:
			CombatComponent->ExecuteSwitchAction(Request.SpecialActionToExecute, FCombatActionContext{Request.Enemy.Get()});
			break;
	}
}

void USquadManagerComponent::HandleAgentSwitchOut(APlayerCharacter* OldAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot)
{
	UCharacterCombatComponent* CombatComponent = OldAgent->GetAgentCombatComponent();

	OwnerController->UnPossess();
	
	if (Request.SwitchOutMode == EAgentSwitchOutMode::FinishActionThenExit)
	{
		if (CombatComponent->IsCurrentActionLogicFinished())
		{
			ApplyAgentOffFieldState(OldAgent);
		} else
		{
			ApplyAgentLingeringState(OldAgent);	
		}
		return;
	}
	
	if (Request.SwitchOutMode == EAgentSwitchOutMode::ExitWithSwitchOutAnim)
	{
		ApplyAgentLingeringState(OldAgent);
		const int32 RequestId = CombatComponent->ExecuteSwitchOutAction();
		if (RequestId != INDEX_NONE)
		{
			PendingLingeringExitRequestIds.Add(OldAgent, RequestId);
		} else
		{
			// Apply Switch Out Failed
			ApplyAgentLingeringState(OldAgent);
		}
		return;
	}
	
	if (Request.SwitchOutMode == EAgentSwitchOutMode::ExitImmediately)
	{
		ApplyAgentOffFieldState(OldAgent);
		return;
	}
	UE_LOG(LogTemp, Error, TEXT("Agent Switch Out went wrong. THIS LOG SHOULD NOT BE PRINTED"));
}

void USquadManagerComponent::BindAgentLingeringDelegate(APlayerCharacter* Agent)
{
	if (!IsValid(Agent))
	{
		return;
	}

	if (UCharacterCombatComponent* CombatComponent = Agent->GetAgentCombatComponent())
	{
		CombatComponent->OnCombatActionFinished.AddUObject(this, &USquadManagerComponent::OnLingeringAgentActionFinished);
		CombatComponent->OnActionLogicFinished.AddUObject(this, &USquadManagerComponent::OnPendingExitAgentActionLogicFinished);
	}
}

void USquadManagerComponent::UnBindAgentLingeringDelegate(APlayerCharacter* Agent)
{
	// todo: UnBind when agent die
}

void USquadManagerComponent::OnPendingExitAgentActionLogicFinished(APlayerCharacter* Agent, int32 RequestId)
{
	if (!Agent)
	{
		return;
	}

	const int32* ExistRequestId = PendingLingeringExitRequestIds.Find(Agent);
	if (ExistRequestId && *ExistRequestId != RequestId)
	{
		return;
	}

	PendingLingeringExitRequestIds.Remove(Agent);

	if (Agent->GetAgentPresence() == EAgentPresenceState::Lingering)
	{
		ApplyAgentOffFieldState(Agent);	
	}
}

void USquadManagerComponent::OnLingeringAgentActionFinished(APlayerCharacter* LingeringAgent, ECombatAnimRequestFinishReason Reason)
{
	if (!IsValid(LingeringAgent) || LingeringAgent->GetAgentPresence() != EAgentPresenceState::Lingering)
	{
		return;
	}

	PendingLingeringExitRequestIds.Remove(LingeringAgent);
	ApplyAgentOffFieldState(LingeringAgent);
}

void USquadManagerComponent::SwitchToAgent(const int32 TargetIndex, bool bIsPrevious, const FCharacterFrameDataBus& DataBus)
{
	if (TargetIndex != INDEX_NONE && TargetIndex != ActiveAgentIndex && Squad.IsValidIndex(TargetIndex))
	{
		FAgentTransitionRequest Request;
		Request.TargetAgentIndex = TargetIndex;
		Request.SwitchInMode = DataBus.HasMovementInput() ? EAgentSwitchInMode::InheritLocomotion : EAgentSwitchInMode::EnterWithSwitchInAnim;
		Request.SwitchOutMode = IsActiveAgentExecutingAction() ? EAgentSwitchOutMode::FinishActionThenExit : EAgentSwitchOutMode::ExitWithSwitchOutAnim;
		Request.SpawnPolicy = bIsPrevious ? EAgentSpawnPolicy::AgentRelativeLeft : EAgentSpawnPolicy::AgentRelativeRight;
		Request.CurrentAgent = GetActiveAgent();
		Request.Enemy = nullptr;
		Request.SpecialActionToExecute = nullptr;
		ExecuteAgentTransition(Request);
	}
}

void USquadManagerComponent::AgentChainAttack(const int32 TargetIndex, bool bIsPrevious)
{
	if (TargetIndex != INDEX_NONE && TargetIndex != ActiveAgentIndex && Squad.IsValidIndex(TargetIndex))
	{
		FAgentTransitionRequest Request;
		Request.TargetAgentIndex = TargetIndex;
		Request.SwitchInMode = EAgentSwitchInMode::ExecuteChainAttack;
		Request.SwitchOutMode = IsActiveAgentExecutingAction() ? EAgentSwitchOutMode::FinishActionThenExit : EAgentSwitchOutMode::ExitWithSwitchOutAnim;
		Request.SpawnPolicy = bIsPrevious ? EAgentSpawnPolicy::ChainAttackLeft : EAgentSpawnPolicy::ChainAttackRight;
		Request.CurrentAgent = GetActiveAgent();
		Request.Enemy = ChainAttackStatus.Enemy;
		Request.SpecialActionToExecute = bIsPrevious ? GetPreviousAgent()->GetSpecialAction(Combat::SpecialAction::ChainAttack) : GetNextAgent()->GetSpecialAction(Combat::SpecialAction::ChainAttack);
		ExecuteAgentTransition(Request);

		//OnUpdateCameraTransform.Broadcast(CalculateActionCameraPosition(Request));
	}
	CloseChainAttackWindow();
}

void USquadManagerComponent::AgentDefensiveAssist(const int32 TargetIndex, bool bIsPrevious)
{
	if (Squad.IsValidIndex(TargetIndex) && ActiveAgentIndex != TargetIndex)
	{
		FAgentTransitionRequest Request;
		Request.TargetAgentIndex = TargetIndex;
		Request.SwitchInMode = EAgentSwitchInMode::ExecuteDefensiveAssist;
		Request.SwitchOutMode = IsActiveAgentExecutingAction() ? EAgentSwitchOutMode::FinishActionThenExit : EAgentSwitchOutMode::ExitImmediately;
		Request.SpawnPolicy = EAgentSpawnPolicy::ParryAssistFacingTarget;
		Request.CurrentAgent = GetActiveAgent();
		Request.Enemy = PerfectAssistStatus.TargetEnemy;
		Request.SpecialActionToExecute = bIsPrevious ?	GetPreviousAgent()->GetSpecialAction(Combat::SpecialAction::DefensiveAssist) :
														GetNextAgent()->GetSpecialAction(Combat::SpecialAction::DefensiveAssist);

		if (!CanExecuteAgentTransition(Request))
		{
			return;
		}
		
		PrepareParryAssistCameraContext(Request, bIsPrevious);
		ExecuteAgentTransition(Request);
		
	}
}

void USquadManagerComponent::AgentQuickAssist(const int32 TargetIndex)
{
	if (Squad.IsValidIndex(TargetIndex) && ActiveAgentIndex != TargetIndex)
	{
		FAgentTransitionRequest Request;
		Request.TargetAgentIndex = TargetIndex;
		Request.SwitchInMode = EAgentSwitchInMode::ExecuteQuickAssist;
		Request.SwitchOutMode = IsActiveAgentExecutingAction() ? EAgentSwitchOutMode::FinishActionThenExit : EAgentSwitchOutMode::ExitWithSwitchOutAnim;
		Request.SpawnPolicy = EAgentSpawnPolicy::QuickAssistFacingTarget;
		Request.CurrentAgent = GetActiveAgent();
		Request.Enemy = QuickAssistStatus.TargetEnemy;
		Request.SpecialActionToExecute = GetNextAgent()->GetSpecialAction(Combat::SpecialAction::QuickAssist);
		ExecuteAgentTransition(Request);

		//OnUpdateCameraTransform.Broadcast(CalculateActionCameraPosition(Request));
	}
}

void USquadManagerComponent::AgentUltimateAttack()
{
	// Condition Check
	if (!GetActiveAgent())
	{
		return;
	}

	UCharacterCombatComponent* CombatComponent = GetActiveAgent()->GetAgentCombatComponent();
	if (!CombatComponent)
	{
		return;
	}

	UCombatActionStep* UltimateAction{CombatComponent->GetSpecialAction(Combat::SpecialAction::Ultimate)};
	if (!UltimateAction)
	{
		return;
	}

	if (!CombatComponent->MeetsActionRequirements(UltimateAction))
	{
		return;
	}
	
	FPendingUltimateCutInRequest Request;
	Request.Agent = GetActiveAgent();
	Request.UltimateAction = UltimateAction;
	Request.CameraStateTag = Combat::Camera::Status::UltimateCamera;		// todo: read in ActionStep
	Request.CutInSequence = UltimateAction->UltimateConfig.CutInSequence;
	Request.bIsValid = true;
	Request.BackgroundColor = UltimateAction->UltimateConfig.BackgroundColor;
	Request.StencilValue = UltimateAction->UltimateConfig.AgentStencilValue;

	OwnerController->RequestUltimateCutIn(Request);
}

ECombatEventHandleResult USquadManagerComponent::TriggerChainAttackWindow(const FCombatEventMessage& CombatEventMessage)
{
	APlayerCharacter* PreviousAgent{GetPreviousAgent()};
	APlayerCharacter* NextAgent{GetNextAgent()};
	if (!IsValid(PreviousAgent) || !IsValid(NextAgent))
	{
		return ECombatEventHandleResult::UnHandled;
	}
	
	UTexture2D* PreviousAgentHead{PreviousAgent->GetAgentHead()};
	UTexture2D* NextAgentHead{NextAgent->GetAgentHead()};

	if (!PreviousAgentHead || !NextAgentHead)
	{
		return ECombatEventHandleResult::UnHandled;
	} 
	
	ChainAttackStatus.bActive = true;
	ChainAttackStatus.Enemy = Cast<AEnemyCharacterBase>(CombatEventMessage.Target);
	OnTriggerChainAttackWindow.Broadcast(PreviousAgentHead, NextAgentHead);
	EnterChainAttackSlowMotion(GetWorld());
	
	return ECombatEventHandleResult::Handled;
}

void USquadManagerComponent::CloseChainAttackWindow()
{
	ChainAttackStatus.bActive = false;
	OnFinishChainAttack.Broadcast();
	ChainAttackStatus.ResetChainAttackWindow();
	ExitChainAttackSlowMotion(GetWorld());
}

void USquadManagerComponent::InitializeAgentSquad()
{
	if (!OwnerController.IsValid() || SquadPreset.IsEmpty())
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	Squad.Empty();

	for (TSubclassOf AgentClass : SquadPreset)
	{
		if (AgentClass)
		{
			APlayerCharacter* NewAgent = World->SpawnActorDeferred<APlayerCharacter>(
				AgentClass,
				FTransform::Identity,
				GetOwner(),
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			if (NewAgent)
			{
				NewAgent->SetActorHiddenInGame(true);
				NewAgent->SetActorEnableCollision(false);

				UGameplayStatics::FinishSpawningActor(NewAgent, FTransform::Identity);
				Squad.Add(NewAgent);

				BindAgentLingeringDelegate(NewAgent);
			}
		}
	}
	FAgentTransitionRequest Request;
	Request.TargetAgentIndex = 0;
	Request.SwitchInMode = EAgentSwitchInMode::InitialIdle;
	Request.SwitchOutMode = EAgentSwitchOutMode::None;
	Request.SpawnPolicy = EAgentSpawnPolicy::InitialSpawn;
	Request.CurrentAgent = nullptr;
	Request.Enemy = nullptr;
	Request.SpecialActionToExecute = nullptr;
	ExecuteAgentTransition(Request);
}

APlayerCharacter* USquadManagerComponent::GetPreviousAgent() const
{
	int32 Index{GetPreviousAgentIndex()};
	return Index == INDEX_NONE ? nullptr : Squad[Index];
}

APlayerCharacter* USquadManagerComponent::GetNextAgent() const
{
	int32 Index{GetNextAgentIndex()};
	return Index == INDEX_NONE ? nullptr : Squad[Index];
}

int32 USquadManagerComponent::GetPreviousAgentIndex() const
{
	const int32 AgentNums{Squad.Num()};
	
	if (AgentNums <= 0 || !Squad.IsValidIndex(ActiveAgentIndex))
	{
		return INDEX_NONE;
	}

	if (AgentNums == 1)
	{
		return ActiveAgentIndex;
	}
	int32 TargetIndex{(ActiveAgentIndex - 1 + AgentNums) % AgentNums};
	return Squad.IsValidIndex(TargetIndex) ? TargetIndex : INDEX_NONE; 
}

int32 USquadManagerComponent::GetNextAgentIndex() const
{
	const int32 AgentNums{Squad.Num()};
	if (AgentNums <= 0 || !Squad.IsValidIndex(ActiveAgentIndex))
	{
		return INDEX_NONE;
	}

	if (AgentNums == 1)
	{
		return ActiveAgentIndex;
	}

	const int32 TargetIndex{(ActiveAgentIndex + 1) % AgentNums};
	return Squad.IsValidIndex(TargetIndex) ? TargetIndex : INDEX_NONE; 
}

void USquadManagerComponent::RouteInput()
{
	if (!OwnerController.IsValid())
	{
		return;
	}

	if (UPlayerInputHandlerComponent* HandlerComp = OwnerController->GetPlayerInputHandlerComponent())
	{
		FCharacterFrameDataBus DataBus{HandlerComp->CommitFrameInput()};
		if (!SquadConsumeInput(DataBus))
		{
			AgentConsumeInput(DataBus);	
		}
	}
}

bool USquadManagerComponent::SquadConsumeInput(FCharacterFrameDataBus& DataBus)
{
//	Squad:
	// Chain Attack Window Open.
	if (ChainAttackStatus.bActive)
	{
		ConsumeChainAttackInput(DataBus);
		return true;		// intercept anyway
	}

	// Perfect Assist Window Open
	if (PerfectAssistStatus.bPerfectAssistWindowOpen)
	{
		if (ConsumePerfectAssistInput(DataBus))
		{
			PerfectAssistStatus.ResetPerfectAssistWindow();
			return true;
		}
	}
	
	// Quick Assist Window Open
	if (QuickAssistStatus.bQuickAssistWindowOpen)
	{
		if (ConsumeQuickAssistInput(DataBus))
		{
			CloseQuickAssistWindow();
			return true;
		}
	}
	
//  Agent:
	// Ultimate
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_Ultimate))
	{
		AgentUltimateAttack();
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_Ultimate);
		return true;
	}
	
	// Normal Switch
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Previous))
	{
		SwitchToAgent(GetPreviousAgentIndex(), true, DataBus);
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Previous);
		return true;
	}
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Next))
	{
		SwitchToAgent(GetNextAgentIndex(), false, DataBus);
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Next);
		return true;
	}
	return false;
}

void USquadManagerComponent::ConsumeChainAttackInput(FCharacterFrameDataBus& DataBus)
{
	if (GetPreviousAgent() && DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_Chain_Attack_Left))
	{
		AgentChainAttack(GetPreviousAgentIndex(), true);
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_Chain_Attack_Left);
	}
	else if (GetNextAgent() && DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_Chain_Attack_Right))
	{
		AgentChainAttack(GetNextAgentIndex(), false);
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_Chain_Attack_Right);
	}
	else if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_Chain_Attack_Cancel))
	{
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_Chain_Attack_Cancel);
		CloseChainAttackWindow();
	}
}

bool USquadManagerComponent::ConsumePerfectAssistInput(FCharacterFrameDataBus& DataBus)
{
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Previous))
	{
		AgentDefensiveAssist(GetPreviousAgentIndex(), true);
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Previous);
		return true;
	}

	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Next))
	{
		AgentDefensiveAssist(GetNextAgentIndex(), false);
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Next);
		return true;
	}
	return false;
}

bool USquadManagerComponent::ConsumeQuickAssistInput(FCharacterFrameDataBus& DataBus)
{
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Previous)
		|| DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Next))
	{
		AgentQuickAssist(GetNextAgentIndex());
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Previous);
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Next);
		return true;
	}
	return false;
}

void USquadManagerComponent::AgentConsumeInput(FCharacterFrameDataBus& DataBus)
{
	if (APlayerCharacter* ActiveAgent = GetActiveAgent())
	{
		ActiveAgent->ProcessFrameInput(DataBus);
	}
}

void USquadManagerComponent::QTEAdvanceCountDown(float DeltaTime)
{
	if (!ChainAttackStatus.bActive)
	{
		return;
	}
	// todo: 考虑时停
	float TimeDilation{GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation()};
	float RealDeltaTime{TimeDilation > 0.001 ? (DeltaTime / TimeDilation) : DeltaTime};
	ChainAttackStatus.QTERemainingTime -= RealDeltaTime;

	if (ChainAttackStatus.QTERemainingTime <= 0)
	{
		ChainAttackStatus.QTERemainingTime = 0.f;
		CloseChainAttackWindow();
	}
}

void USquadManagerComponent::EnterChainAttackSlowMotion(UWorld* World)
{
	if (!World)
	{
		return;
	}
	UGameplayStatics::SetGlobalTimeDilation(World, ChainAttackSlowMotionScale);
}

void USquadManagerComponent::ExitChainAttackSlowMotion(UWorld* World)
{
	if (!World)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
}

ECombatEventHandleResult USquadManagerComponent::TriggerPerfectAssistWindow(const FCombatEventMessage& CombatEventMessage)
{
	if (AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(CombatEventMessage.Source.Get()))
	{
		const FPerfectAssistStatePayload* Payload{CombatEventMessage.GetPayloadPtr<FPerfectAssistStatePayload>()};
		if (!Payload)
		{
			return ECombatEventHandleResult::UnHandled;
		}
		
		PerfectAssistStatus.bPerfectAssistWindowOpen = Payload->bWindowOpen;
		if (Payload->bWindowOpen)
		{
			PerfectAssistStatus.TargetEnemy = Enemy;
			PerfectAssistStatus.ParryReferenceOffset = Payload->ParryReferenceOffset;
		} else
		{
			PerfectAssistStatus.ResetPerfectAssistWindow();
		}
		return ECombatEventHandleResult::Handled;
	}
	
	return ECombatEventHandleResult::UnHandled;
}

ECombatEventHandleResult USquadManagerComponent::TriggerQuickAssistWindow(const FCombatEventMessage& CombatEventMessage)
{
	const FQuickAssistPayload* Payload = CombatEventMessage.GetPayloadPtr<FQuickAssistPayload>();
	if (!Payload)
	{
		return ECombatEventHandleResult::UnHandled;
	}

	if (AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(CombatEventMessage.Source.Get()))
	{
		if (APlayerCharacter* Agent = GetNextAgent())
		{
			QuickAssistStatus.bQuickAssistWindowOpen = true;
			QuickAssistStatus.TargetEnemy = Enemy;
			QuickAssistStatus.QuickAssistRemainingTime = QuickAssistStatus.QuickAssistCountDownDuration;
			OnTriggerQuickAssistWindow.Broadcast(Agent->GetAgentHead());

			return ECombatEventHandleResult::Handled;
		}
	}
	
	return ECombatEventHandleResult::UnHandled;
}

void USquadManagerComponent::CloseQuickAssistWindow()
{
	QuickAssistStatus.ResetQuickAssistWindow();
	OnFinishQuickAssist.Broadcast();
}

void USquadManagerComponent::QuickAssistAdvanceCountDown(float DeltaTime)
{
	if (!QuickAssistStatus.bQuickAssistWindowOpen)
	{
		return;
	}
	
	float TimeDilation{GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation()};
	float RealDeltaTime{TimeDilation > 0.001 ? (DeltaTime / TimeDilation) : DeltaTime};
	QuickAssistStatus.QuickAssistRemainingTime -= RealDeltaTime;

	if (QuickAssistStatus.QuickAssistRemainingTime <= 0.f)
	{
		QuickAssistStatus.QuickAssistRemainingTime = 0.f;
		CloseQuickAssistWindow();
	}
}

FTransform USquadManagerComponent::CalculateAgentSpawnTransform(const FAgentTransitionRequest& Request)
{
	FTransform SpawnTransform{GetActiveAgent() ? GetActiveAgent()->GetTransform() : FTransform::Identity};
	
	switch (Request.SpawnPolicy)
	{
		case EAgentSpawnPolicy::InitialSpawn:
			{
				SpawnTransform = GetInitialSpawnTransform();	
			}
			break;
		case EAgentSpawnPolicy::AgentRelativeLeft:
			{
				FVector SpawnLeftOffset = Request.CurrentAgent->GetActorRightVector() * 100.f;		// todo: hard code here
				SpawnTransform.AddToTranslation(-SpawnLeftOffset);
				SpawnTransform.SetRotation(Request.CurrentAgent->GetActorRotation().Quaternion());	// todo: rotation?
			}
			break;
		case EAgentSpawnPolicy::AgentRelativeRight:
			{
				FVector SpawnRightOffset = Request.CurrentAgent->GetActorRightVector() * 100.f;		// todo: hard code here
				SpawnTransform.AddToTranslation(SpawnRightOffset);
				SpawnTransform.SetRotation(Request.CurrentAgent->GetActorRotation().Quaternion());	// todo: rotation?	
			}
			break;
		case EAgentSpawnPolicy::ChainAttackLeft:
		case EAgentSpawnPolicy::ChainAttackRight:
			CalculateChainAttackSpawnTransform(Request, SpawnTransform);
			break;
		case EAgentSpawnPolicy::ParryAssistFacingTarget:
			{
				CalculateParrySpawnTransform(Request, SpawnTransform);
			}
			break;
		case EAgentSpawnPolicy::QuickAssistFacingTarget:
			{
				CalculateQuickAssistSpawnTransform(Request, SpawnTransform);
			}
			break;
	}
	
	return SpawnTransform;
}

void USquadManagerComponent::CalculateChainAttackSpawnTransform(const FAgentTransitionRequest& Request, FTransform& SpawnTransform)
{
	if (!IsValid(Request.Enemy) || !IsValid(Request.SpecialActionToExecute)
	|| !IsValid(Request.CurrentAgent) || !Request.SpecialActionToExecute->bIsChainAttack)
	{
		return;
	}

	const FVector EnemyLocation{Request.Enemy->GetActorLocation()};

	const FVector EnemyForward{FVector::VectorPlaneProject(Request.Enemy->GetActorForwardVector(), FVector::UpVector).GetSafeNormal2D()};
	if (EnemyForward.IsNearlyZero())
	{
		return;
	}

	const FVector EnemyRight{FVector::CrossProduct(FVector::UpVector, EnemyForward).GetSafeNormal2D()};
	if (EnemyRight.IsNearlyZero())
	{
		return;
	}

	const float SignSide{Request.SpawnPolicy == EAgentSpawnPolicy::ChainAttackLeft ? -1.f : 1.f};

	const float ForwardOffset{Request.SpecialActionToExecute->AttackEntryForwardOffset};
	const float RightOffset{Request.SpecialActionToExecute->AttackEntryLateralOffset};

	FVector TargetSpawnLocation{EnemyLocation
		+ ForwardOffset * EnemyForward
		+ RightOffset * EnemyRight * SignSide
	};
	TargetSpawnLocation.Z = Request.CurrentAgent->GetActorLocation().Z;
	FRotator TargetSpawnRotation{(EnemyLocation - TargetSpawnLocation).Rotation()};
	
	SpawnTransform.SetLocation(TargetSpawnLocation);
	SpawnTransform.SetRotation(TargetSpawnRotation.Quaternion());
	//DrawDebugCapsule(GetWorld(), SpawnTransform.GetLocation(), 50.f, 30.f, FQuat::Identity, FColor::Green, false, 15.f);
}

void USquadManagerComponent::CalculateParrySpawnTransform(const FAgentTransitionRequest& Request, FTransform& SpawnTransform)
{
	if (!IsValid(Request.Enemy) || !IsValid(Request.SpecialActionToExecute)
		|| !IsValid(Request.CurrentAgent) || !Request.SpecialActionToExecute->ParryConfig.bIsParryAction)
	{
		return;
	}

	FVector EnemyLocation{Request.Enemy->GetActorLocation()};
	FVector EnemyForwardDirection{Request.Enemy->GetActorForwardVector()};
	EnemyForwardDirection.Z = 0.f;
	EnemyForwardDirection.Normalize();

	float EnemyParryOffset{PerfectAssistStatus.ParryReferenceOffset};
	float AgentParryOffset{Request.SpecialActionToExecute->ParryConfig.ParrySocketOffset};
	float TotalParryOffset{EnemyParryOffset + AgentParryOffset};
	FVector WorldParryOffset{EnemyForwardDirection * TotalParryOffset};
	
	FVector ClashLocation{EnemyLocation + WorldParryOffset};
	FVector DesiredFacingDirection{-EnemyForwardDirection};
	
	ClashLocation.Z = Request.CurrentAgent->GetActorLocation().Z;
	SpawnTransform.SetLocation(ClashLocation);
	SpawnTransform.SetRotation(DesiredFacingDirection.Rotation().Quaternion());
}

void USquadManagerComponent::CalculateQuickAssistSpawnTransform(const FAgentTransitionRequest& Request, FTransform& SpawnTransform)
{
	if (!IsValid(Request.Enemy) || !IsValid(Request.SpecialActionToExecute)
	|| !IsValid(Request.CurrentAgent) || !Request.SpecialActionToExecute->bIsQuickAssist)
	{
		return;
	}
	
	const FVector EnemyLocation{Request.Enemy->GetActorLocation()};
	
	const FVector EnemyForward{FVector::VectorPlaneProject(Request.Enemy->GetActorForwardVector(), FVector::UpVector).GetSafeNormal2D()};
	if (EnemyForward.IsNearlyZero())
	{
		return;
	}

	const FVector EnemyRight{FVector::CrossProduct(FVector::UpVector, EnemyForward).GetSafeNormal2D()};
	if (EnemyRight.IsNearlyZero())
	{
		return;
	}
	
	const float ForwardOffset{Request.SpecialActionToExecute->AttackEntryForwardOffset};
	const float RightOffset{Request.SpecialActionToExecute->AttackEntryLateralOffset};

	FVector TargetSpawnLocation{EnemyLocation
		+ ForwardOffset * EnemyForward
		+ RightOffset * EnemyRight
	};
	FRotator TargetSpawnRotation{(EnemyLocation - TargetSpawnLocation).Rotation()};
	
	TargetSpawnLocation.Z = Request.CurrentAgent->GetActorLocation().Z;
	SpawnTransform.SetLocation(TargetSpawnLocation);
	SpawnTransform.SetRotation(TargetSpawnRotation.Quaternion());
}

FTransform USquadManagerComponent::GetInitialSpawnTransform() const
{
	FTransform SpawnTransform{FTransform::Identity};

	if (AActor* PlayerStart = GetWorld()->GetAuthGameMode()->ChoosePlayerStart(OwnerController.Get()))
	{
		SpawnTransform = PlayerStart->GetTransform();
	}
	return SpawnTransform;
}

FTransform USquadManagerComponent::CalculateActionCameraPosition(const FAgentTransitionRequest& Request)
{
	FTransform TargetCameraTransform{FTransform::Identity};
	if (!Request.SpecialActionToExecute)
	{
		return TargetCameraTransform;
	}
	
	const FCinematicCameraConfig& CameraConfig{Request.SpecialActionToExecute->CameraConfig};
	
	if (!CameraConfig.bEnableCinematicCamera)
	{
		return TargetCameraTransform;
	}

	FTransform AgentSpawnTransform{CalculateAgentSpawnTransform(Request)};
	FVector AgentLocation{AgentSpawnTransform.GetLocation()};
	
	FVector Forward{AgentSpawnTransform.GetRotation().GetForwardVector()};
	FVector Right{Request.SpawnPolicy == EAgentSpawnPolicy::ChainAttackLeft ?
										-AgentSpawnTransform.GetRotation().GetRightVector() : AgentSpawnTransform.GetRotation().GetRightVector()};
	FVector Up{AgentSpawnTransform.GetRotation().GetUpVector()};
	
	FVector CameraLocation{
		AgentLocation
			+ Forward * CameraConfig.LocalOffset.X
			+ Right * CameraConfig.LocalOffset.Y
			+ Up * CameraConfig.LocalOffset.Z
	};

	FVector LookAtTarget{AgentLocation};
	if (CameraConfig.Mode == ECameraLookAtMode::LookAtEnemy && Request.Enemy)
	{
		LookAtTarget = Request.Enemy->GetActorLocation();
	} else if (CameraConfig.Mode == ECameraLookAtMode::LookAtMiddle && Request.Enemy)
	{
		LookAtTarget = (Request.Enemy->GetActorLocation() + AgentLocation) / 2;
	}

	FRotator CameraRotation{UKismetMathLibrary::FindLookAtRotation(CameraLocation, LookAtTarget)};
	TargetCameraTransform.SetLocation(CameraLocation);
	TargetCameraTransform.SetRotation(CameraRotation.Quaternion());

	/*DrawDebugSphere(GetWorld(), CameraLocation, 10.f, 16, FColor::Purple, false, 10.f);
	DrawDebugDirectionalArrow(GetWorld(), CameraLocation, CameraLocation + 200.f * CameraRotation.Quaternion().GetForwardVector(),
		10.f, FColor::Purple, false, 10.f);
		*/
	
	return TargetCameraTransform;
}

/*void USquadManagerComponent::PrepareCameraRequest(const FAgentTransitionRequest& Request)
{
	if (!OwnerController.Get() || !Request.SpecialActionToExecute || !Request.Enemy)
	{
		return;
	}

	APlayerCharacter* TargetAgent{GetTargetAgent(Request.TargetAgentIndex)};
	if (!TargetAgent)
	{
		return;
	}

	const FCombatCameraConfig& CameraConfig{Request.SpecialActionToExecute->CombatCameraConfig};
	if (CameraConfig.CameraMode != ECombatCameraMode::ParryAssist)
	{
		return;
	}

	UCombatCameraDirectorComponent* DirectorComponent{OwnerController->GetCameraDirectorComponent()};
	if (!DirectorComponent)
	{
		return;
	}

	const FVector EnemyLocation{Request.Enemy->GetActorLocation()};
	FVector EnemyForward{FVector::VectorPlaneProject(Request.Enemy->GetActorForwardVector(), FVector::UpVector).GetSafeNormal()};
	if (EnemyForward.IsNearlyZero())
	{
		return;
	}

	FCombatCameraRequest CameraRequest;
	CameraRequest.Agent = TargetAgent;
	CameraRequest.Config = CameraConfig;
	CameraRequest.AnchorLocation = EnemyLocation + EnemyForward * PerfectAssistStatus.ParryReferenceOffset;
	CameraRequest.BasisForward = -EnemyForward;
	CameraRequest.SideSign = -1;	//todo

	DrawDebugSphere(GetWorld(), CameraRequest.AnchorLocation, 5.f, 8, FColor::Purple, false, 5.f);
	
	DirectorComponent->PrepareCameraRequest(CameraRequest);
}*/

void USquadManagerComponent::PrepareParryAssistCameraContext(const FAgentTransitionRequest& Request, bool bIsPrevious)
{
	if (!OwnerController.Get() || !Request.SpecialActionToExecute || !Request.Enemy)
	{
		return;
	}

	APlayerCharacter* TargetAgent{GetTargetAgent(Request.TargetAgentIndex)};
	if (!TargetAgent)
	{
		return;
	}

	const FCombatCameraConfig& CameraConfig{Request.SpecialActionToExecute->CombatCameraConfig};
	if (CameraConfig.CameraMode != ECombatCameraMode::ParryAssist)
	{
		return;
	}

	UCombatCameraDirectorComponent* DirectorComponent{OwnerController->GetCameraDirectorComponent()};
	if (!DirectorComponent)
	{
		return;
	}

	const FVector EnemyLocation{Request.Enemy->GetActorLocation()};
	const FVector EnemyToAgentDir{(EnemyLocation - TargetAgent->GetActorLocation()).GetSafeNormal()};
	FVector EnemyForward{FVector::VectorPlaneProject(EnemyToAgentDir, FVector::UpVector).GetSafeNormal()};
	if (EnemyForward.IsNearlyZero())
	{
		return;
	}

	FCombatCameraContext Context;
	Context.bHasAnchorLocation = true;
	Context.AnchorLocation = EnemyLocation + EnemyForward * PerfectAssistStatus.ParryReferenceOffset;
	Context.SideSign = bIsPrevious ? -1 : 1;
	
	DirectorComponent->PrepareCameraContext(ECombatCameraMode::ParryAssist, TargetAgent, Context);
}

FTransform USquadManagerComponent::CalculateUltimateCameraPosition(UCombatActionStep* Ultimate, const FTransform& AgentTransform)
{
	FTransform CameraTransform{FTransform::Identity};

	if (!Ultimate || !Ultimate->CameraConfig.bEnableCinematicCamera)
	{
		return CameraTransform;
	}

	const FCinematicCameraConfig& CameraConfig{Ultimate->CameraConfig};

	AEnemyCharacterBase* Enemy{GetActiveAgent()->FindClosestEnemy(Ultimate->MotionWarpingEffectiveDistance)};
	
	FVector AgentLocation{AgentTransform.GetLocation()};
	
	FVector Forward{AgentTransform.GetRotation().GetForwardVector()};
	FVector Right{AgentTransform.GetRotation().GetRightVector()};
	FVector Up{AgentTransform.GetRotation().GetUpVector()};
	
	FVector CameraLocation{
		AgentLocation
			+ Forward * CameraConfig.LocalOffset.X
			+ Right * CameraConfig.LocalOffset.Y
			+ Up * CameraConfig.LocalOffset.Z
	};

	FVector LookAtTarget{AgentLocation};
	if (CameraConfig.Mode == ECameraLookAtMode::LookAtEnemy && Enemy)
	{
		LookAtTarget = Enemy->GetActorLocation();
	} else if (CameraConfig.Mode == ECameraLookAtMode::LookAtMiddle && Enemy)
	{
		LookAtTarget = (Enemy->GetActorLocation() + AgentLocation) / 2;
	}
	FRotator CameraRotation{UKismetMathLibrary::FindLookAtRotation(CameraLocation, LookAtTarget)};
	
	CameraTransform.SetLocation(CameraLocation);
	CameraTransform.SetRotation(CameraRotation.Quaternion());

	DrawDebugSphere(GetWorld(), CameraLocation, 10.f, 16, FColor::Purple, false, 10.f);
	DrawDebugDirectionalArrow(GetWorld(), CameraLocation, CameraLocation + 200.f * CameraRotation.Quaternion().GetForwardVector(),
		10.f, FColor::Purple, false, 10.f);

	return CameraTransform;
}
