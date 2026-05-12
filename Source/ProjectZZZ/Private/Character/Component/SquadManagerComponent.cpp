#include "Character/Component/SquadManagerComponent.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerCharacter.h"
#include "Utility/ZZZGameplayTag.h"

void FChainAttackWindowStatus::ResetChainAttackWindow()
{
	QTEDuration = QTEDuration > 0.f ? QTEDuration : 3.f;
	QTERemainingTime = QTEDuration;
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
		FCombatEventDelegate Callback;
		Callback.BindUObject(this, &USquadManagerComponent::TriggerChainAttackWindow);
		Handle = EventBus->Subscribe(Combat::Event::ChainAttack, this, 1, Callback);
	}
}

void USquadManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RouteInput();
	QTEAdvanceCountDown(DeltaTime);
}

void USquadManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
	OwnerController = Cast<AZZZPlayerController>(GetOwner());
}

APlayerCharacter* USquadManagerComponent::GetActivePlayerCharacter() const
{
	if (Squad.IsValidIndex(ActiveAgentIndex))
	{
		return Squad[ActiveAgentIndex];
	}
	// not possible
	return nullptr;
}

void USquadManagerComponent::ExecuteAgentTransition(const FAgentTransitionRequest& Request)
{
	if (!CanExecuteAgentTransition(Request))
	{
		return;
	} 
	// 还有问题, 切代理人后位置不对,相机问题
	APlayerCharacter* OldAgent{GetActivePlayerCharacter()};
	APlayerCharacter* TargetAgent{Squad[Request.TargetAgentIndex]};
	
	const FAgentTransitionSnapshot Snapshot{CacheAgentSnapshot(OldAgent)};

	// 初始时无OldAgent
	if (OldAgent)
	{
		HandleAgentSwitchOut(OldAgent, Request, Snapshot);	
	}
	
	HandleAgentSwitchIn(TargetAgent, Request, Snapshot);
	ActiveAgentIndex = Request.TargetAgentIndex;
	
	// todo: Set Transform in HandleAgentSwitchIn?
	//TargetAgent->SetActorTransform(Request.SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

bool USquadManagerComponent::CanExecuteAgentTransition(const FAgentTransitionRequest& Request)
{
	if (!OwnerController.IsValid() || !Squad.IsValidIndex(Request.TargetAgentIndex)
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
		return TargetCombatComponent->CanAffordActionCost(Request.SpecialActionToExecute);
	}
	return true;
}

void USquadManagerComponent::ApplyAgentState(APlayerCharacter* Agent, EAgentPresenceState State)
{
	if (!IsValid(Agent))
	{
		return;
	}

	Agent->SetAgentPresence(State);
	switch (State)
	{
	case EAgentPresenceState::Active:
		ApplyAgentActiveState(Agent);
		break;

	case EAgentPresenceState::Lingering:
		ApplyAgentLingeringState(Agent);
		break;;

	case EAgentPresenceState::OffField:
		ApplyAgentOffFieldState(Agent);
		break;
	}
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

	if (UCharacterMovementComponent* MovementComponent = OldAgent->GetCharacterMovement())
	{
		Snapshot.Velocity = MovementComponent->Velocity;
		Snapshot.MovementMode = MovementComponent->MovementMode;
		Snapshot.bWasMoving = !MovementComponent->Velocity.IsNormalized();
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

FTransform USquadManagerComponent::CalculateSwitchInTransform(const EAgentSpawnPolicy Policy, APlayerCharacter* OldAgent) const
{
	ensureMsgf(OldAgent, TEXT("Invalid Agent. Can not calculate Switch In Transform"));
	FTransform TargetTransform{OldAgent->GetActorTransform()};
	switch (Policy)
	{
	case EAgentSpawnPolicy::AbsoluteTransform:
		if (AActor* StartSpot = GetWorld()->GetAuthGameMode()->ChoosePlayerStart(OwnerController.Get()))
		{
			TargetTransform.SetLocation(StartSpot->GetActorLocation());
		}
		break;
	case EAgentSpawnPolicy::RelativeRight:
		FVector SpawnOffset = OldAgent ? OldAgent->GetActorRightVector() * 100.f : FVector::ZeroVector;
		TargetTransform = OldAgent ? OldAgent->GetTransform() : FTransform::Identity;
		TargetTransform.AddToTranslation(SpawnOffset);
		TargetTransform.SetRotation(OldAgent->GetActorRotation().Quaternion());	
		break;
	case EAgentSpawnPolicy::FaceTarget:

		break;
	}

	// Chain Attack

	// Quick Assist

	// Parry Assist
	
	return TargetTransform;
}

void USquadManagerComponent::HandleAgentSwitchIn(APlayerCharacter* NewAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot)
{
	UCharacterCombatComponent* CombatComponent = NewAgent->GetAgentCombatComponent();
	OwnerController->Possess(NewAgent);
	
	if (CombatComponent->IsAnyActionActive())
	{
		CombatComponent->CancelCurrentAction();	//	
	}
	
	UCharacterMovementComponent* MovementComponent = NewAgent->GetCharacterMovement();
	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
	}

	ApplyAgentActiveState(NewAgent);
	NewAgent->SetActorTransform(Request.SpawnTransform);
	
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
	case EAgentSwitchInMode::ExecuteSpecialAction:
		// todo
		break;
	}
}

void USquadManagerComponent::HandleAgentSwitchOut(APlayerCharacter* OldAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot)
{
	UCharacterCombatComponent* CombatComponent = OldAgent->GetAgentCombatComponent();

	OwnerController->UnPossess();
	if (Request.SwitchOutMode == EAgentSwitchOutMode::FinishActionThenExit && Snapshot.bHasActiveAction)
	{
		ApplyAgentLingeringState(OldAgent);
	}
	else if (Request.SwitchOutMode == EAgentSwitchOutMode::ExitWithSwitchOutAnim)
	{
		ApplyAgentLingeringState(OldAgent);
		CombatComponent->ExecuteSwitchOutAction();
	}
	else if (Request.SwitchOutMode == EAgentSwitchOutMode::ExitImmediately)
	{
		ApplyAgentOffFieldState(OldAgent);
	}
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
	}
}

void USquadManagerComponent::UnBindAgentLingeringDelegate(APlayerCharacter* Agent)
{
	// todo: UnBind when agent die
}

void USquadManagerComponent::OnLingeringAgentActionFinished(APlayerCharacter* LingeringAgent)
{
	// todo: 这里不太对, 动作结束语义 与 切出动作结束语义 是不同的。不应该所有动作结束都广播一下，然后再OnLingeringAgentActionFinished判断状态是不是Lingering
	if (!IsValid(LingeringAgent) || LingeringAgent->GetAgentPresence() != EAgentPresenceState::Lingering)
	{
		return;
	}

	ApplyAgentOffFieldState(LingeringAgent);
}

void USquadManagerComponent::SwitchToPreviousAgent()
{
	SwitchToAgent(GetPreviousAgentIndex());
}

void USquadManagerComponent::SwitchToNextAgent()
{
	SwitchToAgent(GetNextAgentIndex());
}

void USquadManagerComponent::SwitchToAgent(const int32 TargetIndex)
{
	if (TargetIndex != INDEX_NONE && Squad.IsValidIndex(TargetIndex))
	{
		FAgentTransitionRequest Request;
		Request.TargetAgentIndex = TargetIndex;
		Request.SwitchInMode = GetActivePlayerCharacter()->HasMovementInput() ? EAgentSwitchInMode::InheritLocomotion : EAgentSwitchInMode::EnterWithSwitchInAnim;
		Request.SwitchOutMode = EAgentSwitchOutMode::ExitWithSwitchOutAnim;
		Request.SpawnPolicy = EAgentSpawnPolicy::RelativeRight;
		Request.SpawnTransform = CalculateSwitchInTransform(EAgentSpawnPolicy::RelativeRight, GetActivePlayerCharacter());
		ExecuteAgentTransition(Request);
	}
}

void USquadManagerComponent::AgentChainAttack(const int32 TargetIndex)
{
	if (Squad.IsValidIndex(TargetIndex) && ActiveAgentIndex != TargetIndex)
	{
		

		
	}
	CloseChainAttackWindow();
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
	OnTriggerChainAttack.Broadcast(PreviousAgentHead, NextAgentHead);
	return ECombatEventHandleResult::Handled;
}

void USquadManagerComponent::CloseChainAttackWindow()
{
	ChainAttackStatus.bActive = false;
	OnFinishChainAttack.Broadcast();
	ChainAttackStatus.ResetChainAttackWindow();
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
	Request.SpawnPolicy = EAgentSpawnPolicy::AbsoluteTransform;
	ensureMsgf(Squad.IsValidIndex(Request.TargetAgentIndex), TEXT("Invalid Target Index"));
	Request.SpawnTransform = CalculateSwitchInTransform(EAgentSpawnPolicy::AbsoluteTransform, Squad[Request.TargetAgentIndex]);
	ExecuteAgentTransition(Request);
}

/*void USquadManagerComponent::AgentSwapImplementation(const int32 AgentIndex)
{
	if (!Squad.IsValidIndex(AgentIndex) || !OwnerController.IsValid())
	{
		return;
	}

	APlayerCharacter* CurrentAgent{GetActivePlayerCharacter()};
	APlayerCharacter* TargetAgent{Squad[AgentIndex]};
	FVector InheritedVelocity{FVector::ZeroVector};
	FRotator InheritedRotation{FRotator::ZeroRotator};
	bool bCurrentAgentMoving{false};
	
	// Switch out Current Agent
	if (CurrentAgent)
	{
		OwnerController->UnPossess();
		// AI Controller?
		
		CurrentAgent->SetAgentPresence(EAgentPresenceState::Lingering);

		InheritedVelocity = CurrentAgent->GetCharacterMovement()->Velocity;
		InheritedRotation = CurrentAgent->GetActorRotation();
		bCurrentAgentMoving = !InheritedVelocity.IsNearlyZero();
		
		// If CurrentAgent has unfinished action. Continue until it's finished.
		UCharacterCombatComponent* CurrentAgentCombatComp = CurrentAgent->GetAgentCombatComponent();
		checkf(CurrentAgentCombatComp, TEXT("Switch Out Agent CombatComponent Invalid"));
		if (!CurrentAgentCombatComp->IsAnyActionActive())
		{
			// If CurrentAgent is not play any animation/action. Play Switch Out Animation then go OffField.
			CurrentAgentCombatComp->ExecuteSwitchOutAction();
		}
	}

	// Switch In Target Agent
	if (TargetAgent)
	{
		FVector SpawnOffset = CurrentAgent ? CurrentAgent->GetActorRightVector() * 100.f : FVector::ZeroVector;
		FTransform SpawnTransform = CurrentAgent ? CurrentAgent->GetTransform() : FTransform::Identity;
		SpawnTransform.AddToTranslation(SpawnOffset);

		TargetAgent->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		
		TargetAgent->SwitchToOnField();
		
		OwnerController->Possess(TargetAgent);

		UCharacterCombatComponent* TargetCombatComp = TargetAgent->GetAgentCombatComponent();
		checkf(TargetCombatComp, TEXT("Switch In Agent CombatComponent Invalid"));
		
		if (OwnerController->HasMovementInput() && bCurrentAgentMoving)
		{
			TargetAgent->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			TargetAgent->GetCharacterMovement()->Velocity = InheritedVelocity;
			TargetAgent->SetActorRotation(InheritedRotation);
		} else
		{
			TargetCombatComp->ExecuteSwitchInAction();
		}
		
		ActiveAgentIndex = AgentIndex;
	}
}*/

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
		FCharacterFrameDataBus DataBus{HandlerComp->GetCharacterFrameDataBus()};
		if (!SquadConsumeInput(DataBus))
		{
			AgentConsumeInput(DataBus);
		}	
	}
}

bool USquadManagerComponent::SquadConsumeInput(FCharacterFrameDataBus& DataBus)
{
	// Chain Attack Window Open.
	if (ChainAttackStatus.bActive)
	{
		ConsumeChainAttackInput(DataBus);
		return true;		// intercept anyway
	}
	
	// Is Quick Assist Activated
	
	// Normal Switch
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Previous))
	{
		SwitchToPreviousAgent();
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Previous);
		return true;
	}
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Next))
	{
		SwitchToNextAgent();
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Next);
		return true;
	}
	return false;
}

void USquadManagerComponent::ConsumeChainAttackInput(FCharacterFrameDataBus& DataBus)
{
	if (GetPreviousAgent() && DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_Chain_Attack_Left))
	{
		AgentChainAttack(GetPreviousAgentIndex());
	}
	else if (GetNextAgent() && DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_Chain_Attack_Right))
	{
		AgentChainAttack(GetNextAgentIndex());
	}
	else if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_Chain_Attack_Cancel))
	{
		CloseChainAttackWindow();
	}
}

void USquadManagerComponent::AgentConsumeInput(FCharacterFrameDataBus& DataBus)
{
	if (APlayerCharacter* ActiveAgent = GetActivePlayerCharacter())
	{
		ActiveAgent->RefreshCharacterFrameInputData(DataBus);
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
