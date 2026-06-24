// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Component/CharacterCombatComponent.h"
#include "GameplayEffect.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/AgentAttributeSet.h"
#include "AI/EnemyCharacterBase.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/CharacterBase.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Component/HitStopComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerCharacter.h"
#include "Utility/ZZZGameplayTag.h"
#include "Character/Combat/ZZZCombatEventTypes.h"
#include "Character/Component/CombatCameraDirectorComponent.h"

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
}

void UCharacterCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCharacterCombatComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UCharacterCombatComponent::ProcessFrameInput(const FPlayerInputs& FrameInputs)
{
	ProcessInputAction(FrameInputs);
	ProcessBufferedInput();
}

bool UCharacterCombatComponent::ActivateCombatCamera(const ECombatCameraMode CameraMode)
{
	APlayerCharacter* Agent{Cast<APlayerCharacter>(Character)};
	if (!Agent || CameraMode == ECombatCameraMode::None)
	{
		return false;
	}

	AZZZPlayerController* PC{Cast<AZZZPlayerController>(Agent->GetController())};
	if (!PC || !CurrentExecutionState.CurrentStep.IsValid())
	{
		return false; 
	}

	const UCombatActionStep* ActionStep{CurrentExecutionState.CurrentStep.Get()};
	if (!ActionStep)
	{
		return false;
	}
	
	const FCombatCameraConfig& Config{ActionStep->CombatCameraConfig};
	if (!Config.bEnableCombatCamera || Config.CameraMode != CameraMode)
	{
		return false;
	}

	UCombatCameraDirectorComponent* DirectorComponent{PC->GetCameraDirectorComponent()};
	if (!DirectorComponent)
	{
		return false;
	}

	FCombatCameraSectionContext Context;
	Context.Agent = Agent;
	Context.CameraMode = CameraMode;
	Context.CameraConfig = Config;
	Context.AgentSectionTransform = Agent->GetActorTransform();
	Context.Enemy = CurrentExecutionState.Enemy.Get();
	
	return DirectorComponent->ActivateCameraSection(Context); 
}

void UCharacterCombatComponent::DeactivateCombatCamera(const ECombatCameraMode CameraMode)
{
	APlayerCharacter* Agent{Cast<APlayerCharacter>(Character)};
	if (!Agent || CameraMode == ECombatCameraMode::None)
	{
		return;
	}

	AZZZPlayerController* PC{Cast<AZZZPlayerController>(Agent->GetController())};
	if (!PC)
	{
		return; 
	}
	
	UCombatCameraDirectorComponent* DirectorComponent{PC->GetCameraDirectorComponent()};
	if (!DirectorComponent)
	{
		return;
	}

	DirectorComponent->DeactivateCameraSection(CameraMode, Agent);
}

void UCharacterCombatComponent::ProcessInputAction(const FPlayerInputs& FrameInputs)
{
	// Invalid Character or No Input Action
	if (!IsValid(Character) || !FrameInputs.InputActionBitmask.Any())
	{
		return;
	}
	
	const UCombatActionStep* TargetAction{SelectTargetAction(FrameInputs)};
	if (!TargetAction)
	{
		return;
	}
	
	if (!IsAnyActionActive() || CanInterruptCurrentAction(TargetAction))
	{
		// Execute Target Action immediately
		ExecuteAction(TargetAction, FCombatActionContext());
	} else if (CurrentExecutionState.bInputBufferWindowOpen)
	{
		// Todo: check buffer window and other window
		// Can't Execute now. Buffer TargetAction
		BufferInputIntent(TargetAction);
	}
}

void UCharacterCombatComponent::ProcessBufferedInput()
{
	if (!PendingIntent.IsValid())
	{
		return;
	}

	if (CurrentExecutionState.bProceedWindowOpen && CurrentExecutionState.bHasConfirmedNextAction)
	{
		if (!IsAnyActionActive() || CanInterruptCurrentAction(PendingIntent.ActionStep))
		{
			if (ExecuteAction(PendingIntent.ActionStep, FCombatActionContext()) != INDEX_NONE)
			{
				// Cost and CurrentExecutionState already handled in ExecuteAction
				PendingIntent.Reset();
			} 	
		}
	}
}

// Only for Player Character
UCombatActionStep* UCharacterCombatComponent::SelectTargetAction(const FPlayerInputs& FrameInputs)
{
	UCombatActionStep* TargetAction{nullptr};
	FrameInputs.InputActionBitmask.ForEachSetAction([&](EInputAction InputAction)
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
		TargetAction->Montage = TargetAction->GetAnimMontage(FrameInputs.RawMovementInput);
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
		if (MeetsActionRequirements(Step.Get()))
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
		if (MeetsActionRequirements(Step))
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

int32 UCharacterCombatComponent::ExecuteAction(const UCombatActionStep* ActionStep, const FCombatActionContext& Context)
{
	if (!IsValid(ActionStep) || !IsValid(CombatAnimSchedulerComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("Execute Action Failed. Invalid Action or Invalid CombatAnimSchedulerComponent"));
		return INDEX_NONE;
	}

	AEnemyCharacterBase* Enemy{nullptr};
 	if (ActionStep->bIsAttackAction)
	{
		Enemy = FindClosestEnemy(ActionStep->MotionWarpingEffectiveDistance);
	}

	if (Context.Enemy.IsValid())
	{
		Enemy = Cast<AEnemyCharacterBase>(Context.Enemy.Get());
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
		DetectionStatus.ResetAll();
		
		CurrentExecutionState.CurrentStep = ActionStep;
		CurrentExecutionState.Enemy = Enemy;
		CurrentExecutionState.MontageInstanceId = InstanceID;
		CurrentExecutionState.bHasSuccessfullyStarted = true;
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("Execute Action Failed. Instance ID == INDEX_NONE"));
	}
	return InstanceID;
}

int32 UCharacterCombatComponent::ExecuteUltimateAction(const FCombatActionContext& Context)
{
	return ExecuteAction(GetSpecialAction(Combat::SpecialAction::Ultimate), Context);
}

void UCharacterCombatComponent::TryApplyMotionWarpingIfNeeded(const UCombatActionStep* ActionStep, const AEnemyCharacterBase* Enemy)
{
	APlayerCharacter* Agent = Cast<APlayerCharacter>(Character);
	if (!IsValid(ActionStep) || !IsValid(Agent) || !Agent->GetMotionWarpingComponent() || !ActionStep->bEnableMotionWarp)
	{
		return;
	}
	
	Agent->GetMotionWarpingComponent()->RemoveAllWarpTargets();
	
	// out of Motion Warping Range. Still try to adjust agent rotation toward closet enemy.
	if (Enemy == nullptr)
	{
		if (AEnemyCharacterBase* ClosetEnemy = FindClosestEnemy(1200.f)) {	// hard code for now
			const FVector ToEnemy{(ClosetEnemy->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D()};
			if (!ToEnemy.IsNearlyZero())
			{
				Character->SetActorRotation(ToEnemy.Rotation(), ETeleportType::TeleportPhysics);	
			}
		}
		return;
	}
	
	for (const FMotionWarpConfig& Config : ActionStep->WarpConfigs)
	{
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
		
		if (Config.TrackingMode == EMotionWarpTrackingMode::DynamicComponent)
		{
			FVector WarpOffset{FVector(StandOffDistance, 0.f, 0.f)};
			FRotator FacingEnemyRotation{FRotator(0.f, 180.f, 0.f)};
			Agent->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromComponent(
				Config.WarpTargetName,
				Enemy->GetRootComponent(),
				NAME_None,
				true,
				EWarpTargetLocationOffsetDirection::TargetsForwardVector,
				WarpOffset,
				FacingEnemyRotation);
		} else if (Config.TrackingMode == EMotionWarpTrackingMode::StaticWorldPoint)
		{
			ApplyStaticPointMotionWarping(Config, Agent, Enemy);
		}
	}
}

void UCharacterCombatComponent::ApplyStaticPointMotionWarping(const FMotionWarpConfig& Config, const APlayerCharacter* Agent, const AEnemyCharacterBase* Enemy)
{
	if (!Agent)
	{
		return;
	}
	
	FVector AgentLocation{Agent->GetActorLocation()};
	
	switch (Config.Rules)
	{
		case EMotionWarpCalculationRules::EnemyRelativeFacing:
			{
				if (!Enemy)
				{
					break;
				}
				FVector EnemyLocation{Enemy->GetActorLocation()};
				FRotator FacingEnemyRotation{(EnemyLocation - AgentLocation).GetSafeNormal2D().Rotation()};
				if (const FMotionWarpCalcMethod_EnemyRelative* Method = Config.CalculationMethod.GetPtr<FMotionWarpCalcMethod_EnemyRelative>())
				{
					FVector Offset{FVector::ZeroVector};
					if (Method->bCloseToTarget)
					{
						float Distance{Agent->GetCapsuleComponent()->GetScaledCapsuleRadius() + Enemy->GetCapsuleComponent()->GetScaledCapsuleRadius()};
						Offset = Enemy->GetActorRotation().Quaternion().GetForwardVector() * Distance;
					} else
					{
						Offset = Method->OffsetFromTarget;
					}
					
					FVector TargetLocation{Enemy->GetActorLocation() + Enemy->GetActorRotation().RotateVector(Offset)};
					TargetLocation.Z = Agent->GetActorLocation().Z;
					FRotator TargetRotation{FacingEnemyRotation};	// facing

					Character->SetActorRotation(TargetRotation, ETeleportType::TeleportPhysics);
					Agent->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocationAndRotation(
						Config.WarpTargetName,
						TargetLocation,
						TargetRotation);
				}
			}
			break;
		case EMotionWarpCalculationRules::PiercingLine:
			{
				if (!Enemy)
				{
					break;
				}
				FVector EnemyLocation{Enemy->GetActorLocation()};
				FRotator FacingEnemyRotation{(EnemyLocation - AgentLocation).GetSafeNormal2D().Rotation()};
				if (const FMotionWarpCalcMethod_PiercingLine* Method = Config.CalculationMethod.GetPtr<FMotionWarpCalcMethod_PiercingLine>())
				{
					float PiercingLength{Method->PiercingLength};
					FVector TargetLocation{EnemyLocation + FacingEnemyRotation.Quaternion().GetForwardVector() * PiercingLength};
					FRotator TargetRotation{FacingEnemyRotation.GetInverse()};//
					Character->SetActorRotation(FacingEnemyRotation, ETeleportType::TeleportPhysics);
					Agent->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocationAndRotation(
						Config.WarpTargetName,
						TargetLocation,
						TargetRotation);
				}
			}
			break;
		case EMotionWarpCalculationRules::BackToOrigin:
			{
				Agent->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocationAndRotation(
						Config.WarpTargetName,
						AgentLocation,
						Agent->GetActorRotation());
				//DrawDebugCapsule(GetWorld(), AgentLocation, 50.f, 30.f, Agent->GetActorRotation().Quaternion(), FColor::Green, false, 5.f);
			}
			break;

		default:
			{}	
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

void UCharacterCombatComponent::NotifyActionLogicFinished(const FGameplayTag& Tag)
{
	if (!CurrentExecutionState.CurrentStep.IsValid() || CurrentExecutionState.CurrentStep->ActionTag != Tag)
	{
		return;
	}

	APlayerCharacter* Agent{Cast<APlayerCharacter>(Character)};
	if (!Agent)
	{
		return;
	}

	CurrentExecutionState.bActionLogicFinished = true;
	CurrentExecutionState.LogicFinishedActionRequestId = CurrentExecutionState.MontageInstanceId;
	
	OnActionLogicFinished.Broadcast(Agent, CurrentExecutionState.MontageInstanceId);
}

bool UCharacterCombatComponent::IsCurrentActionLogicFinished() const
{
	return		CurrentExecutionState.CurrentStep.IsValid()
			&&	CurrentExecutionState.bHasSuccessfullyStarted
			&&	CurrentExecutionState.bActionLogicFinished
			&&	CurrentExecutionState.MontageInstanceId == CurrentExecutionState.LogicFinishedActionRequestId;
}

/*void UCharacterCombatComponent::RefreshInputActionBitmask(const float DeltaTime)
{
	CurrentInputActionBitmask = InputActionBitmask;
	InputActionBitmask.Reset();
}*/

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
	return CurrentExecutionState.CurrentStep.IsValid() && CurrentExecutionState.bMovementInterruptWindowOpen;
}

void UCharacterCombatComponent::HandleCombatWindowChange(const FGameplayTag Tag, bool bIsOpen, UAnimMontage* SourceMontage)
{
	if (!IsValid(SourceMontage) || !CurrentExecutionState.CurrentStep.IsValid()
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

bool UCharacterCombatComponent::MeetsActionRequirements(const UCombatActionStep* Step) const
{
	if (!IsValid(Step) || !IsValid(Step->CostGameplayEffect) || !IsValid(AbilitySystemComponent))
	{
		return false;
	}

	if (!AbilitySystemComponent->HasAllMatchingGameplayTags(Step->RequiredTags))
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
	if (Tag == Combat::SpecialAction::Ultimate)
	{
		return UltimateAction;
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
	if (CurrentExecutionState.CurrentStep.IsValid() && CurrentExecutionState.CurrentStep->ParryConfig.bIsParryAction
		&& AbilitySystemComponent->HasMatchingGameplayTag(Combat::Status::Agent::Parry))
	{
		// Jump to Section
		Result.ResultType = EAttackResultType::Parried;
		Result.ParryInstigator = Character;
		
		// Apply GE On Enemy
		const FParryActionConfig& ParryConfig{CurrentExecutionState.CurrentStep->ParryConfig};
		ApplyGameplayEffectOnTarget(AbilitySystemComponent, Context.InstigatorASC.Get(), ParryConfig.ParryEffectOnEnemy); 

		// Play Sound
		if (ParryConfig.ParrySuccessSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this,
												ParryConfig.ParrySuccessSound,
												Character->GetActorLocation(),
												ParryConfig.ParrySuccessVolume,
												ParryConfig.ParrySuccessPitch);
		}
		
		// Apply HitStop
		Character->GetHitStopComponent()->ApplyHitStop(ParryConfig.HitStopDuration, ParryConfig.HitStopTimeScale);
		
		// HitStop on Enemy
		Result.HitStopDuration = ParryConfig.HitStopDuration;
		Result.HitStopTimeScale = ParryConfig.HitStopTimeScale;
		
		CombatAnimSchedulerComponent->RequestMontageSetNextSection(CurrentExecutionState.MontageInstanceId, ParryConfig.LoopSectionName, ParryConfig.FollowSectionName);

		// BroadCast Event
		if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>())
		{
			EventBus->BroadcastEvent(Combat::Event::ParrySucceed, Character, Context.Target, Context.Instigator, FPlainPayload());	// ParrySucceed Payload
		}
		return;
	}

	// Apply Damage
	ApplyImpactEffectOnTarget(SourceASC, TargetASC, Context);
	
	// HitReaction
	ExecuteHitReaction(Context.Instigator, Context.PayloadConfig.AttackStrength);

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
	ExecuteSwitchAction(SwitchInAction, FCombatActionContext());
}

int32 UCharacterCombatComponent::ExecuteSwitchOutAction()
{
	return ExecuteSwitchAction(SwitchOutAction, FCombatActionContext());
}

int32 UCharacterCombatComponent::ExecuteSwitchAction(UCombatActionStep* Action, const FCombatActionContext& Context)
{
	if (Action && (!IsAnyActionActive() || CanInterruptCurrentAction(Action)))
	{
		UE_LOG(LogTemp, Error, TEXT("Execute Switch Action. Action Name: %s"), *Action->GetName());
		return ExecuteAction(Action, Context);
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("Execute Switch Action Failed. Action Name: %s"), *Action->GetName());
		return INDEX_NONE;
	}
}

void UCharacterCombatComponent::OnEnergyChanged(const FOnAttributeChangeData& Data)
{
}

void UCharacterCombatComponent::OnDecibelsChanged(const FOnAttributeChangeData& Data)
{
}
