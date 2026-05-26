#include "Character/Component/SquadManagerComponent.h"

#include "MotionWarpingComponent.h"
#include "AI/EnemyCharacterBase.h"
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
	Enemy = nullptr;
}

void FPerfectAssistWindowStatus::ResetPerfectAssistWindow()
{
	bPerfectAssistWindowOpen = false;
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
	}
}

void USquadManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RouteInput();
	QTEAdvanceCountDown(DeltaTime);
	
	// debug
#if WITH_EDITOR
	if (APlayerCharacter* Agent = Cast<APlayerCharacter>(GetActiveAgent()))
	{
		if (UMotionWarpingComponent* WarpComp = Agent->GetMotionWarpingComponent())
		{
			// 你可以填入你想要观察的那个锚点名字，比如 "ParryTarget"
			FName DebugTargetName = FName("ParryTarget");
            
			// 去底层的缓存池里找这个目标
			if (const FMotionWarpingTarget* Target = WarpComp->FindWarpTarget(DebugTargetName))
			{
				// Target->Location 和 Target->Rotation 就是引擎底层应用了所有规则 
				// (Component移动、TargetsForwardVector、ParryOffset) 之后，最终解算出来的世界绝对坐标！
				FVector FinalWorldPos = Target->Location;
				FRotator FinalRotation = Target->Rotation;


				if (Target->bFollowComponent && Target->Component.IsValid())
				{
					// 获取怪物根组件此刻(这一帧)的实时绝对坐标
					FTransform CompTransform = Target->Component->GetSocketTransform(Target->BoneName);
                    
					// 还原 EWarpTargetLocationOffsetDirection::TargetsForwardVector 的底层空间矩阵乘法
					FVector Forward = CompTransform.GetRotation().GetForwardVector();
					FVector Right = CompTransform.GetRotation().GetRightVector();
					FVector Up = CompTransform.GetRotation().GetUpVector();
                    
					// 实时位置 = 怪物原点 + (正前 * 偏移X) + (正右 * 偏移Y) + (正上 * 偏移Z)
					FinalWorldPos = CompTransform.GetLocation() 
								  + (Forward * Target->LocationOffset.X)
								  + (Right * Target->LocationOffset.Y)
								  + (Up * Target->LocationOffset.Z);
                                  
					// 实时朝向 = 怪物的实时朝向 叠加 我们设定的 180 度旋转
					FinalRotation = (CompTransform.GetRotation() * Target->RotationOffset.Quaternion()).Rotator();
				}
                
				DrawDebugSphere(GetWorld(), FinalWorldPos, 15.f, 16, FColor::Purple, false, -1.f, 0, 2.f);
				
				if (Target->Component.IsValid())
				{
					FVector RootPos = Target->Component->GetComponentLocation();
					DrawDebugLine(GetWorld(), RootPos, FinalWorldPos, FColor::Yellow, false, -1.f, 0, 1.f);
				}

				FVector FacingDir = FinalRotation.Vector();
				
				DrawDebugDirectionalArrow(GetWorld(), FinalWorldPos, FinalWorldPos + FacingDir * 50.f,
					10.f, FColor::Green, false, -1.f, 0, 2.f);
			}
		}
	}
#endif
	
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
	// 还有问题, 切代理人后位置不对,相机问题
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

/*FTransform USquadManagerComponent::CalculateSwitchInTransform(const EAgentSpawnPolicy Policy, APlayerCharacter* OldAgent) const
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
	case EAgentSpawnPolicy::RelativeLeft:
		FVector SpawnLeftOffset = OldAgent ? OldAgent->GetActorRightVector() * 100.f : FVector::ZeroVector;
		TargetTransform = OldAgent ? OldAgent->GetTransform() : FTransform::Identity;
		TargetTransform.AddToTranslation(-SpawnLeftOffset);
		TargetTransform.SetRotation(OldAgent->GetActorRotation().Quaternion());	
		break;
	case EAgentSpawnPolicy::RelativeRight:
		FVector SpawnRightOffset = OldAgent ? OldAgent->GetActorRightVector() * 100.f : FVector::ZeroVector;
		TargetTransform = OldAgent ? OldAgent->GetTransform() : FTransform::Identity;
		TargetTransform.AddToTranslation(SpawnRightOffset);
		TargetTransform.SetRotation(OldAgent->GetActorRotation().Quaternion());	
		break;
	case EAgentSpawnPolicy::FaceTarget:

		break;
	}

	// Chain Attack

	// Quick Assist

	// Parry Assist
	
	return TargetTransform;
}*/

void USquadManagerComponent::HandleAgentSwitchIn(APlayerCharacter* NewAgent, const FAgentTransitionRequest& Request, const FAgentTransitionSnapshot& Snapshot)
{
	UCharacterCombatComponent* CombatComponent = NewAgent->GetAgentCombatComponent();
	OwnerController->Possess(NewAgent);

	ApplyAgentActiveState(NewAgent);
	
	NewAgent->SetActorTransform(CalculateAgentSpawnTransform(Request));
	
	if (CombatComponent->IsAnyActionActive())
	{
		CombatComponent->CancelCurrentAction();	//	
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
		CombatComponent->ExecuteSwitchAction(Request.SpecialActionToExecute);
		break;
	case EAgentSwitchInMode::ExecuteQuickAssist:
		break;
	case EAgentSwitchInMode::ExecuteDefensiveAssist:
		ExecuteDefensiveAssist(NewAgent, Request);
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
	UE_LOG(LogTemp, Error, TEXT("Agent Switch Out went wrong. THIS LOG SHOULD NOT BE PRINTED"))
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
	SwitchToAgent(GetPreviousAgentIndex(), true);
}

void USquadManagerComponent::SwitchToNextAgent()
{
	SwitchToAgent(GetNextAgentIndex(), false);
}

void USquadManagerComponent::SwitchToAgent(const int32 TargetIndex, bool bIsPrevious)
{
	if (TargetIndex != INDEX_NONE && Squad.IsValidIndex(TargetIndex))
	{
		FAgentTransitionRequest Request;
		Request.TargetAgentIndex = TargetIndex;
		Request.SwitchInMode = GetActiveAgent()->HasMovementInput() ? EAgentSwitchInMode::InheritLocomotion : EAgentSwitchInMode::EnterWithSwitchInAnim;
		Request.SwitchOutMode = EAgentSwitchOutMode::ExitWithSwitchOutAnim;
		Request.SpawnPolicy = bIsPrevious ? EAgentSpawnPolicy::AgentRelativeLeft : EAgentSpawnPolicy::AgentRelativeRight;
		Request.CurrentAgent = GetActiveAgent();
		Request.Enemy = nullptr;
		Request.SpecialActionToExecute = nullptr;
		ExecuteAgentTransition(Request);
	}
}

void USquadManagerComponent::AgentChainAttack(const int32 TargetIndex, bool bIsPrevious)
{
	if (Squad.IsValidIndex(TargetIndex) && ActiveAgentIndex != TargetIndex)
	{
		FAgentTransitionRequest Request;
		Request.TargetAgentIndex = TargetIndex;
		Request.SwitchInMode = EAgentSwitchInMode::ExecuteChainAttack;
		Request.SwitchOutMode = EAgentSwitchOutMode::ExitWithSwitchOutAnim;
		Request.SpawnPolicy = bIsPrevious ? EAgentSpawnPolicy::AgentRelativeLeft : EAgentSpawnPolicy::AgentRelativeRight;
		Request.CurrentAgent = GetActiveAgent();
		Request.Enemy = ChainAttackStatus.Enemy;
		Request.SpecialActionToExecute = bIsPrevious ? GetPreviousAgent()->GetSpecialAction(Combat::SpecialAction::ChainAttack) : GetNextAgent()->GetSpecialAction(Combat::SpecialAction::ChainAttack);
		ExecuteAgentTransition(Request);
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
		Request.SwitchOutMode = IsActiveAgentExecutingAction() ? EAgentSwitchOutMode::FinishActionThenExit : EAgentSwitchOutMode::ExitImmediately;		// todo: 确认
		Request.SpawnPolicy = EAgentSpawnPolicy::ParryAssistFacingTarget;
		Request.CurrentAgent = GetActiveAgent();
		Request.Enemy = PerfectAssistStatus.TargetEnemy;
		Request.SpecialActionToExecute = bIsPrevious ?	GetPreviousAgent()->GetSpecialAction(Combat::SpecialAction::DefensiveAssist) :
														GetNextAgent()->GetSpecialAction(Combat::SpecialAction::DefensiveAssist);
		ExecuteAgentTransition(Request);
	}
}

void USquadManagerComponent::ExecuteDefensiveAssist(APlayerCharacter* NewAgent, const FAgentTransitionRequest& Request)
{
	if (!IsValid(NewAgent) || !IsValid(Request.SpecialActionToExecute))
	{
		UE_LOG(LogTemp, Error, TEXT("Execute Defensive Assist Failed. New Agent or Defensive Assist Action Invalid"));
		return;
	}
	
	/*UMotionWarpingComponent* MotionWarping = NewAgent->GetMotionWarpingComponent();
	if (!IsValid(MotionWarping))
	{
		return;
	}
	
	MotionWarping->RemoveAllWarpTargets();
	
	FVector FinalOffset{FVector::ZeroVector};
	FinalOffset.X = PerfectAssistStatus.ParryReferenceOffset + Request.SpecialActionToExecute->AssistConfig.ParrySocketOffset;
	FRotator FaceEnemyRotation(0.f, 180.f, 0.f);
	
	MotionWarping->AddOrUpdateWarpTargetFromComponent(
		Request.SpecialActionToExecute->AssistConfig.WarpTargetName,
		Request.Enemy->GetRootComponent(),
		NAME_None,
		true,
		EWarpTargetLocationOffsetDirection::TargetsForwardVector,
		FinalOffset,
		FaceEnemyRotation);*/
	
	
	UCharacterCombatComponent* CombatComponent = NewAgent->GetAgentCombatComponent();
	CombatComponent->ExecuteSwitchAction(Request.SpecialActionToExecute);
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
	Request.SwitchOutMode = EAgentSwitchOutMode::None;
	Request.SpawnPolicy = EAgentSpawnPolicy::InitialSpawn;
	Request.CurrentAgent = nullptr;
	Request.Enemy = nullptr;
	Request.SpecialActionToExecute = nullptr;
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
//	Squad:
	// Chain Attack Window Open.
	if (ChainAttackStatus.bActive)
	{
		ConsumeChainAttackInput(DataBus);
		return true;		// intercept anyway
	}

	// Perfect Assist
	if (PerfectAssistStatus.bPerfectAssistWindowOpen)
	{
		if (ConsumePerfectAssistInput(DataBus))
		{
			PerfectAssistStatus.bPerfectAssistWindowOpen = false;
			// todo: 极限支援的State如何重置
		}
		
	}

	
	// Quick Assist
	
//  Agent:
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
		AgentChainAttack(GetPreviousAgentIndex(), true);
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_Chain_Attack_Left);
		// Consume
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

void USquadManagerComponent::AgentConsumeInput(FCharacterFrameDataBus& DataBus)
{
	if (APlayerCharacter* ActiveAgent = GetActiveAgent())
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

ECombatEventHandleResult USquadManagerComponent::TriggerPerfectAssistWindow(const FCombatEventMessage& CombatEventMessage)
{
	if (!CombatEventMessage.Payload.IsValid() || CombatEventMessage.Payload.GetScriptStruct() != FPerfectAssistStatePayload::StaticStruct())
	{
		return ECombatEventHandleResult::UnHandled;
	}

	if (AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(CombatEventMessage.Source.Get()))
	{
		const FPerfectAssistStatePayload& Payload = CombatEventMessage.Payload.Get<FPerfectAssistStatePayload>();
		PerfectAssistStatus.bPerfectAssistWindowOpen = Payload.bWindowOpen;
		PerfectAssistStatus.TargetEnemy = Enemy;
		PerfectAssistStatus.ParryReferenceOffset = Payload.ParryReferenceOffset;
		return ECombatEventHandleResult::Handled;
	}
	
	return ECombatEventHandleResult::UnHandled;
}

FTransform USquadManagerComponent::CalculateAgentSpawnTransform(const FAgentTransitionRequest& Request)
{
	FTransform SpawnTransform{GetActiveAgent() ? GetActiveAgent()->GetTransform() : FTransform::Identity};
	
	switch (Request.SpawnPolicy)
	{
		case EAgentSpawnPolicy::InitialSpawn:
			SpawnTransform = GetInitialSpawnTransform();
			break;
		
		case EAgentSpawnPolicy::AgentRelativeLeft:
			FVector SpawnLeftOffset = Request.CurrentAgent->GetActorRightVector() * 100.f;		// todo: hard code here
			SpawnTransform.AddToTranslation(-SpawnLeftOffset);
			SpawnTransform.SetRotation(Request.CurrentAgent->GetActorRotation().Quaternion());	// todo: rotation?
			break;
		
		case EAgentSpawnPolicy::AgentRelativeRight:
			FVector SpawnRightOffset = Request.CurrentAgent->GetActorRightVector() * 100.f;		// todo: hard code here
			SpawnTransform.AddToTranslation(SpawnRightOffset);
			SpawnTransform.SetRotation(Request.CurrentAgent->GetActorRotation().Quaternion());	// todo: rotation?
			break;
		case EAgentSpawnPolicy::ParryAssistFacingTarget:
			CalculateParrySpawnTransform(Request, SpawnTransform);
			break;
	}
	return SpawnTransform;
}

void USquadManagerComponent::CalculateParrySpawnTransform(const FAgentTransitionRequest& Request, FTransform& SpawnTransform)
{
	if (!IsValid(Request.Enemy) || !IsValid(Request.SpecialActionToExecute)
		|| !IsValid(Request.CurrentAgent) || !Request.SpecialActionToExecute->AssistConfig.bIsAssistAction)
	{
		return;
	}

	FVector EnemyLocation{Request.Enemy->GetActorLocation()};
	FVector EnemyForwardDirection{Request.Enemy->GetActorForwardVector()};
	EnemyForwardDirection.Z = 0.f;
	EnemyForwardDirection.Normalize();

	float EnemyParryOffset{PerfectAssistStatus.ParryReferenceOffset};
	float AgentParryOffset{Request.SpecialActionToExecute->AssistConfig.ParrySocketOffset};
	float TotalParryOffset{EnemyParryOffset + AgentParryOffset};
	FVector WorldParryOffset{EnemyForwardDirection * TotalParryOffset};
	
	FVector ClashLocation{EnemyLocation + WorldParryOffset};
	FVector DesiredFacingDirection{-EnemyForwardDirection};
	
	ClashLocation.Z = Request.CurrentAgent->GetActorLocation().Z;
	SpawnTransform.SetLocation(ClashLocation);
	SpawnTransform.SetRotation(DesiredFacingDirection.Rotation().Quaternion());
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
