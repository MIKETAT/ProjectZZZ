// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Component/CharacterCombatComponent.h"
#include "GameplayEffect.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/AgentAttributeSet.h"
#include "AI/EnemyCharacterBase.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/CharacterBase.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Component/HitStopComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerCharacter.h"
#include "Utility/ZZZGameplayTag.h"
#include "Character/Combat/CombatHitReactionAction.h"
#include "Character/Combat/ZZZCombatEventTypes.h"

UCharacterCombatComponent::UCharacterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCharacterCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeCombatStepList();

	// Bind Delegate
	
	if (IsValid(GetAgentAnimInstance()))
	{
		GetAgentAnimInstance()->OnCombatWindowChanged.AddDynamic(this, &ThisClass::HandleCombatWindowChange);
	}

	if (IsValid(CombatAnimSchedulerComponent))
	{
		CombatAnimSchedulerComponent->OnAnimRequestFinished.AddDynamic(this, &ThisClass::HandleAnimFinished);
	}
}

void UCharacterCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshInputActionBitmask(DeltaTime);
	ProcessInputAction(DeltaTime);
	ProcessBufferedInput(DeltaTime);
}

void UCharacterCombatComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UCharacterCombatComponent::ProcessInputAction(const float DeltaTime)
{
	// Invalid Character or No Input Action
	if (!IsValid(Character) || !CurrentInputActionBitmask.Any())
	{
		return;
	}
	
	const UCombatActionStep* TargetAction{SelectTargetAction()};
	if (!TargetAction)
	{
		return;
	}
	
	if (!IsAnyActionActive() || CanInterruptCurrentAction(TargetAction))
	{
		// Execute Target Action immediately
		ExecuteAction(TargetAction);
	} else if (CurrentExecutionState.bInputBufferWindowOpen)
	{
		// Todo: check buffer window and other window
		// Can't Execute now. Buffer TargetAction
		BufferInputIntent(TargetAction);
	}
}

void UCharacterCombatComponent::ProcessBufferedInput(const float DeltaTime)
{
	if (!PendingIntent.IsValid())
	{
		return;
	}

	if (CurrentExecutionState.bProceedWindowOpen && CurrentExecutionState.bHasConfirmedNextAction)
	{
		if (ExecuteAction(PendingIntent.ActionStep) != INDEX_NONE)
		{
			// Cost and CurrentExecutionState already handled in ExecuteAction
			PendingIntent.Reset();
		} 
	}
}

// Only for Player Character
UCombatActionStep* UCharacterCombatComponent::SelectTargetAction()
{
	UCombatActionStep* TargetAction{nullptr};
	CurrentInputActionBitmask.ForEachSetAction([&](EInputAction InputAction)
	{
		UCombatActionStep* ThisAction{SelectComboActionIntent(InputAction)};
		if (ThisAction == nullptr)
		{
			ThisAction = SelectCombatActionIntent(InputAction);
		}

		// current no target or this action has higher priority
		if (TargetAction == nullptr)
		{
			TargetAction = ThisAction;
		} else if (ThisAction && ThisAction->Priority > TargetAction->Priority)
		{
			TargetAction = ThisAction;
		}
	});

	APlayerCharacter* Player = Cast<APlayerCharacter>(Character);
	
	if (TargetAction && Player)
	{
		TargetAction->Montage = TargetAction->GetAnimMontage(Player->GetCharacterFrameDataBus());
	}
	
	return TargetAction;
}

UCombatActionStep* UCharacterCombatComponent::SelectComboActionIntent(const EInputAction Input)
{
	if (CurrentExecutionState.CurrentStep == nullptr || CurrentExecutionState.bInputBufferWindowOpen == false)
	{
		return nullptr;
	}
	
	if (const TObjectPtr<UCombatActionStep> Step = CurrentExecutionState.CurrentStep->ComboLinks.FindRef(Input))
	{
		if (CanAffordActionCost(Step.Get()))
		{
			return Step;
		}
	}

	return nullptr;
}

UCombatActionStep* UCharacterCombatComponent::SelectCombatActionIntent(const EInputAction Input)
{
	for (UCombatActionStep* Step : CombatActionList)
	{
		if (!Step || Input != Step->TriggerInput)
		{
			continue;
		}

		// match RequiredTags and Cost
		if (AbilitySystemComponent->HasAllMatchingGameplayTags(Step->RequiredTags) && CanAffordActionCost(Step))
		{
			return Step;
		}
	}
	return nullptr;
}

void UCharacterCombatComponent::BufferInputIntent(const UCombatActionStep* ActionToBuffer)
{
	if (!ActionToBuffer)
	{
		return;
	}

	if (!PendingIntent.IsValid())
	{
		PendingIntent.SetIntent(ActionToBuffer, GetWorld()->GetTimeSeconds(), ActionToBuffer->Priority);
		CurrentExecutionState.bHasConfirmedNextAction = true;
		return;
	}

	// duplicated input action
	if (PendingIntent.ActionStep == ActionToBuffer)
	{
		return;
	}

	if (ActionToBuffer->Priority > PendingIntent.Priority)
	{
		PendingIntent.Reset();
		PendingIntent.SetIntent(ActionToBuffer, GetWorld()->GetTimeSeconds() + GlobalBufferLifespan, ActionToBuffer->Priority);
		CurrentExecutionState.bHasConfirmedNextAction = true;
	}
}

int32 UCharacterCombatComponent::ExecuteAction(const UCombatActionStep* ActionStep)
{
	if (!IsValid(ActionStep) || !IsValid(CombatAnimSchedulerComponent))
	{
		return INDEX_NONE;
	}

	AEnemyCharacterBase* Enemy{nullptr};
 	if (ActionStep->bIsAttackAction)
	{
		Enemy = FindClosestEnemy(ActionStep->WarpConfig.MotionWarpingEffectiveDistance);
	}
	
	if (IsAnyActionActive())
	{
		CombatAnimSchedulerComponent->CancelAnimRequest(CurrentExecutionState.MontageInstanceId);
	}
	
	FCombatAnimExecutionRequest Request;
	Request.Montage = ActionStep->Montage;
	Request.Priority = ActionStep->Priority;
	int32 InstanceID = CombatAnimSchedulerComponent->ExecuteAnimRequest(Request);
	if (InstanceID != INDEX_NONE)
	{
		// Execute successfully. Apply Cost. Reset Status.
		PayActionCost(ActionStep);

		TryApplyMotionWarpingIfNeeded(ActionStep, Enemy);
			
		CurrentExecutionState.Reset();
		CurrentExecutionState.CurrentStep = ActionStep;
		CurrentExecutionState.MontageInstanceId = InstanceID;
		CurrentExecutionState.bHasSuccessfullyStarted = true;
	}
	return InstanceID;
}

void UCharacterCombatComponent::TryApplyMotionWarpingIfNeeded(const UCombatActionStep* ActionStep, const AEnemyCharacterBase* Enemy)
{
	// Motion Warping
	if (!IsValid(ActionStep) || !IsValid(Enemy) || !ActionStep->WarpConfig.bEnableMotionWarp)
	{
		return;
	}

	APlayerCharacter* Agent = Cast<APlayerCharacter>(Character);
	float AgentRadius{0.f};
	float EnemyRadius{0.f};
	if (UCapsuleComponent* AgentCap = Agent->GetCapsuleComponent())
	{
		AgentRadius = AgentCap->GetScaledCapsuleRadius();
	}
	if (UCapsuleComponent* EnemyCaps = Enemy->GetCapsuleComponent())
	{
		EnemyRadius = EnemyCaps->GetScaledCapsuleRadius();
	}
	float StandOffDistance{AgentRadius + EnemyRadius};
	
	Agent->GetMotionWarpingComponent()->RemoveAllWarpTargets();

	if (ActionStep->WarpConfig.TrackingMode == EMotionWarpTrackingMode::DynamicComponent)
	{
		FVector WarpOffset{FVector(StandOffDistance, 0.f, 0.f)};
		FRotator FacingEnemyRotation{FRotator(0.f, 180.f, 0.f)};
		Character->SetActorRotation(FacingEnemyRotation, ETeleportType::TeleportPhysics);
		Agent->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromComponent(
			ActionStep->WarpConfig.WarpTargetName,
			Enemy->GetRootComponent(),
			NAME_None,
			true,
			EWarpTargetLocationOffsetDirection::TargetsForwardVector,
			WarpOffset,
			FacingEnemyRotation);
	} else if (ActionStep->WarpConfig.TrackingMode == EMotionWarpTrackingMode::StaticWorldPoint)
	{
		FVector AgentLocation{Agent->GetActorLocation()};
		FVector EnemyLocation{Enemy->GetActorLocation()};
		AgentLocation.Z = 0.f;
		EnemyLocation.Z = 0.f;
		FVector DirectionToTarget{(EnemyLocation - AgentLocation).GetSafeNormal()};
		FVector TargetLocation{EnemyLocation - (DirectionToTarget * StandOffDistance)};
		TargetLocation.Z = Agent->GetActorLocation().Z;
		FRotator TargetRotation{DirectionToTarget.Rotation()};

		DrawDebugSphere(GetWorld(), TargetLocation, 15.f, 10, FColor::Yellow, false, 8.f);
		Character->SetActorRotation(TargetRotation, ETeleportType::TeleportPhysics);
		Agent->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocationAndRotation(
			ActionStep->WarpConfig.WarpTargetName,
			TargetLocation,
			TargetRotation);
	}
}

AEnemyCharacterBase* UCharacterCombatComponent::FindClosestEnemy(const float MaxDistance)
{
	AEnemyCharacterBase* Enemy{nullptr};
	if (!IsValid(Character))
	{
		return Enemy;
	}

	FVector AgentLocation{Character->GetActorLocation()};

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	TArray<AActor*> IgnoreActors;
	TArray<AActor*> OutActors;
	IgnoreActors.Add(Character);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AgentLocation,
		MaxDistance,
		ObjectTypes,
		AEnemyCharacterBase::StaticClass(),
		IgnoreActors,
		OutActors);

	if (OutActors.IsEmpty())
	{
		return Enemy;
	}

	AActor* Candidate{nullptr};
	float MaxDistanceSquared{MAX_FLT};
	for (AActor* Actor : OutActors)
	{
		double DistanceSquared{(Actor->GetActorLocation() - AgentLocation).SizeSquared2D()};
		if (DistanceSquared < MaxDistanceSquared)
		{
			MaxDistanceSquared = DistanceSquared;
			Candidate = Actor;
		}
	}
	Enemy = Cast<AEnemyCharacterBase>(Candidate);
	return Enemy;
}

void UCharacterCombatComponent::RefreshInputActionBitmask(const float DeltaTime)
{
	CurrentInputActionBitmask = InputActionBitmask;
	InputActionBitmask.Reset();
}

void UCharacterCombatComponent::InitializeCombatStepList()
{
	if (!IsValid(AgentCombatSteps))
	{
		return;
	}

	for (UCombatActionStep* Step : AgentCombatSteps->CombatSteps)
	{
		if (Step)
		{
			CombatActionList.Add(Step);
		}
	}
	// Sort
	CombatActionList.Sort([](const UCombatActionStep& A, const UCombatActionStep& B){ return A.Priority > B.Priority; });
}

bool UCharacterCombatComponent::IsAllowMovementInterruptAction() const
{
	return CurrentExecutionState.CurrentStep && CurrentExecutionState.bMovementInterruptWindowOpen;
}

void UCharacterCombatComponent::HandleCombatWindowChange(const FGameplayTag Tag, bool bIsOpen, UAnimMontage* SourceMontage)
{
	if (!IsValid(SourceMontage) || !IsValid(CurrentExecutionState.CurrentStep)
		|| !IsValid(CurrentExecutionState.CurrentStep->Montage) || SourceMontage != CurrentExecutionState.CurrentStep->Montage)
	{
		return;
	}
				
	// Input Buffer Window
	if (Tag == Combat::CombatWindows::InputBufferWindow)
	{
		CurrentExecutionState.bInputBufferWindowOpen = bIsOpen;
		return;
	}
	
	// Process Window
	if (Tag == Combat::CombatWindows::ProceedWindow)
	{
		CurrentExecutionState.bProceedWindowOpen = bIsOpen;
		return;
	}
	
	// IsRecovery Window
	if (Tag == Combat::CombatWindows::IsRecoveryWindow)
	{
		CurrentExecutionState.bIsRecoveryWindowOpen = bIsOpen;
		return;
	}

	// Parry
	if (Tag == Combat::CombatWindows::ParryWindow)
	{
		CurrentExecutionState.bParryWindowOpen = bIsOpen;
		return;
	}

	// Movement Interrupt
	if (Tag == Combat::CombatWindows::MovementInterruptWindow)
	{
		CurrentExecutionState.bMovementInterruptWindowOpen = bIsOpen;
		return;
	}
}

bool UCharacterCombatComponent::CanAffordActionCost(const UCombatActionStep* Step) const
{
	if (!IsValid(Step) || !IsValid(Step->CostGameplayEffect) || !IsValid(AbilitySystemComponent))
	{
		return false;
	}
	if (AbilitySystemComponent->CanApplyAttributeModifiers(Step->CostGameplayEffect->GetDefaultObject<UGameplayEffect>(), 1, AbilitySystemComponent->MakeEffectContext()))
	{
		return true;
	}
	return false;
}

bool UCharacterCombatComponent::CanExecuteSwitchAction(const UCombatActionStep* Step) const
{
	APlayerCharacter* Agent{Cast<APlayerCharacter>(Character)};
	return Agent && Agent->GetAgentPresence() == EAgentPresenceState::OffField;
}

void UCharacterCombatComponent::PayActionCost(const UCombatActionStep* Step)
{
	if (!IsValid(AbilitySystemComponent) || !IsValid(Step) || !IsValid(Step->CostGameplayEffect))
	{
		return;
	}
	
	FGameplayEffectContextHandle EffectContext{AbilitySystemComponent->MakeEffectContext()};
	FGameplayEffectSpecHandle EffectSpec{AbilitySystemComponent->MakeOutgoingSpec(Step->CostGameplayEffect, 1.f, EffectContext)};
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
}

UCombatActionStep* UCharacterCombatComponent::GetSpecialAction(const FGameplayTag& Tag) const
{
	if (Tag == Combat::SpecialAction::ChainAttack)
	{
		return ChainAttackAction;
	}
	if (Tag == Combat::SpecialAction::QuickAssist)
	{
		return QuickAssistAction;
	}
	if (Tag == Combat::SpecialAction::DefensiveAssist)
	{
		return DefensiveAssistAction;
	}
	return nullptr;
}

void UCharacterCombatComponent::HandleIncomingDamage(const FAttackContext& Context, FAttackResult& Result)
{
	UAbilitySystemComponent* SourceASC{Context.InstigatorASC.Get()};
	UAbilitySystemComponent* TargetASC{GetAbilitySystemComponent()};
	
	if (!IsValid(SourceASC) || !IsValid(TargetASC) || !Context.IsContextValid()
		|| !IsValid(CombatAnimSchedulerComponent) || !IsValid(AbilitySystemComponent))
	{
		return;
	}
	
	UE_LOG(LogTemp, Error, TEXT("Attack Detection: Agent gonna got Hit. Now check Dodge and Parry"));
	
	// Dodge Check
	if (AbilitySystemComponent->HasMatchingGameplayTag(Combat::Status::Agent::Dodge))
	{
		Result.ResultType = EAttackResultType::Dodged;
		return;
	}

	// Parry Check
	if (CurrentExecutionState.CurrentStep && CurrentExecutionState.CurrentStep->ParryConfig.bIsParryAction
		&& AbilitySystemComponent->HasMatchingGameplayTag(Combat::Status::Agent::Parry))
	{
		// Jump to Section
		Result.ResultType = EAttackResultType::Parried;
		
		// Apply GE On Enemy
		const FParryActionConfig& ParryConfig{CurrentExecutionState.CurrentStep->ParryConfig};
		ApplyGameplayEffectOnTarget(AbilitySystemComponent, Context.InstigatorASC.Get(), ParryConfig.ParryEffectOnEnemy);

		// Apply HitStop
		Character->GetHitStopComponent()->ApplyHitStop(ParryConfig.HitStopDuration, ParryConfig.HitStopTimeScale);
		
		// HitStop on Enemy
		Result.HitStopDuration = ParryConfig.HitStopDuration;
		Result.HitStopTimeScale = ParryConfig.HitStopTimeScale;
		
		CombatAnimSchedulerComponent->RequestMontageJumpToSection(CurrentExecutionState.MontageInstanceId, ParryConfig.SuccessSectionName);
		return;
	}

	// Apply Damage
	ApplyImpactEffectOnTarget(SourceASC, TargetASC, Context);

	// HitReaction
	EHitReactionDirection Direction{EHitReactionDirection::Front};
	if (Context.Instigator)
	{
		Direction = CalculateHitReactionDirection(Context.Instigator->GetActorLocation());	
	}
	
	const UCombatActionStep* HitReactionAction{nullptr};
	if (const FDirectionalHitReactionActions* Actions = HitReactionMap.Find(Context.PayloadConfig.AttackStrength))
	{
		switch (Direction)
		{
			case EHitReactionDirection::Front: HitReactionAction = Actions->FrontHit; break;
			case EHitReactionDirection::Back: HitReactionAction = Actions->BackHit; break;
			default: break;
		}
	}
	
	if (HitReactionAction)
	{
		ExecuteAction(HitReactionAction);
	}

	// Quick Assist
	if (Context.PayloadConfig.AttackStrength == EAttackStrength::Launch)
	{
		if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>())
		{
			FQuickAssistPayload Payload;
			EventBus->BroadcastEvent(Combat::Event::QuickAssist, Context.Instigator, Character, Context.Instigator, Payload);
		}
	}
}

void UCharacterCombatComponent::ProcessHitFeedback(const FAttackResult& Result)
{
	switch (Result.ResultType)
	{
		case EAttackResultType::Invalid:
			break;
		case EAttackResultType::Hit:
			if (IsValid(Result.HitFeedbackEffectOnSelf)){
				ApplyGameplayEffectOnTarget(AbilitySystemComponent, AbilitySystemComponent, Result.HitFeedbackEffectOnSelf);
				break;
			}
		case EAttackResultType::Parried:
			// Not Possible for Agent
			break;
		case EAttackResultType::Dodged:
			// Not Possible for Agent
			break;
		case EAttackResultType::Killed:
			break;
	}
}

void UCharacterCombatComponent::InjectAndBindASC(UAgentAbilitySystemComponent* InASC)
{
	Super::InjectAndBindASC(InASC);
	
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}
	
	// Agent Attribute Set
	if (AbilitySystemComponent->GetSet<UAgentAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UAgentAttributeSet::GetEnergyAttribute()).AddUObject(this, &UCharacterCombatComponent::OnEnergyChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UAgentAttributeSet::GetDecibelsAttribute()).AddUObject(this, &UCharacterCombatComponent::OnDecibelsChanged);
	}
}

void UCharacterCombatComponent::ExecuteSwitchInAction()
{
	ExecuteSwitchAction(SwitchInAction);
}

void UCharacterCombatComponent::ExecuteSwitchOutAction()
{
	ExecuteSwitchAction(SwitchOutAction);
}

void UCharacterCombatComponent::ExecuteSwitchAction(UCombatActionStep* Action)
{
	if (!IsAnyActionActive() || CanInterruptCurrentAction(Action))
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteSwitchAction"));
		ExecuteAction(Action);
	}
}

void UCharacterCombatComponent::OnEnergyChanged(const FOnAttributeChangeData& Data)
{
}

void UCharacterCombatComponent::OnDecibelsChanged(const FOnAttributeChangeData& Data)
{
}
