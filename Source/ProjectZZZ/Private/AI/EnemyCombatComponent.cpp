#include "AI/EnemyCombatComponent.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "AI/EnemyAIController.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/CharacterBase.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Combat/ZZZCombatEventTypes.h"
#include "Character/Component/HitStopComponent.h"
#include "Utility/ZZZGameplayTag.h"

UEnemyCombatComponent::UEnemyCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyCombatComponent::HandleIncomingDamage(const FAttackContext& Context, FAttackResult& Result)
{
	UAbilitySystemComponent* SourceASC{Context.InstigatorASC.Get()};
	UAbilitySystemComponent* TargetASC{GetAbilitySystemComponent()};
	
	if (!IsValid(SourceASC) || !IsValid(TargetASC) || !Context.IsContextValid() || !IsValid(CombatAnimSchedulerComponent))
	{
		return;
	}
	
	// Apply Damage
	Result.ResultType = EAttackResultType::Hit;
	ApplyImpactEffectOnTarget(SourceASC, TargetASC, Context);

	// Daze
	bool bIsDazeValueFull{IsDazeValueFull()};
	bool bIsHeavyAttack{Context.PayloadConfig.bIsHeavyAttack};
	if (bIsDazeValueFull && bIsHeavyAttack && !bIsStunned)
	{
		// Cancel Current Action/Play Stun Montage/Add Stun Tag
		EnterStunState();

		// Trigger Chain Attack
		if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>())
		{
			FChainAttackPayload Payload;
			EventBus->BroadcastEvent(Combat::Event::ChainAttack, Context.Instigator, Context.Target, Context.Instigator, Payload);
		}	
	}
	
	// Not Stun
	ExecuteHitReaction(Context.Instigator, Context.PayloadConfig.AttackStrength);
}

int32 UEnemyCombatComponent::ExecuteAction(const UCombatActionStep* ActionStep, const FCombatActionContext& Context)
{
	if (!IsValid(ActionStep) || !IsValid(CombatAnimSchedulerComponent))
	{
		return INDEX_NONE;
	}

	if (IsAnyActionActive() && !CanInterruptCurrentAction(ActionStep))
	{
		return INDEX_NONE;
	}
	
	FCombatAnimExecutionRequest Request;
	Request.Montage = ActionStep->Montage;
	Request.bUseWeaponSweepDetection = ActionStep->bUseWeaponSweep;
	if (ActionStep->bIsHitReaction)
	{
		Request.PlayRate = 2.f;
	}

	int32 InstanceID = CombatAnimSchedulerComponent->ExecuteAnimRequest(Request);
	if (InstanceID != INDEX_NONE)
	{
		CurrentExecutionState.Reset();
		DetectionStatus.ResetAll();
		CurrentExecutionState.CurrentStep = ActionStep;
		CurrentExecutionState.MontageInstanceId = InstanceID;
		CurrentExecutionState.bHasSuccessfullyStarted = true;
	}
	
	return InstanceID;
}

float UEnemyCombatComponent::GetCurrentActionParryOffset() const
{
	float ParryOffset{0.f};

	if (CurrentExecutionState.CurrentStep.IsValid() && CurrentExecutionState.CurrentStep->ParriedActionConfig.bIsParriedAction)
	{
		ParryOffset = CurrentExecutionState.CurrentStep->ParriedActionConfig.ParryOffset;
	}

	return ParryOffset;
}

void UEnemyCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UEnemyCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UEnemyCombatComponent::ProcessHitFeedback(const FAttackResult& Result)
{
	switch (Result.ResultType)
	{
		case EAttackResultType::Invalid:
			break;
		case EAttackResultType::Hit:
			break;
		case EAttackResultType::Killed:
			break;
		case EAttackResultType::Dodged:
			// Agent Dodge this attack
			break;
		case EAttackResultType::Parried:
			{
				checkf(Character, TEXT("Enemy Character Invalid"));
				if (UHitStopComponent* HitStopComponent = Character->GetHitStopComponent())
				{
					UE_LOG(LogTemp, Log, TEXT("Enemy Attack was Parried. Apply HitStop on Enemy"));
					HitStopComponent->ApplyHitStop(Result.HitStopDuration, Result.HitStopTimeScale);
				}
				ExecuteHitReaction(Result.ParryInstigator.Get(), EAttackStrength::Light_Knockback);
			}
			break;
	}
}

void UEnemyCombatComponent::InjectAndBindASC(UAgentAbilitySystemComponent* InASC)
{
	Super::InjectAndBindASC(InASC);

	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}
	
	// EnemyAttribute Set
	if (AbilitySystemComponent->GetSet<UEnemyAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UEnemyAttributeSet::GetDazeAttribute()).AddUObject(this, &UEnemyCombatComponent::OnDazeChanged);
	}
}

bool UEnemyCombatComponent::IsDazeValueFull() const
{
	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	float CurrentDaze{AbilitySystemComponent->GetNumericAttribute(UEnemyAttributeSet::GetDazeAttribute())};
	float MaxDaze{AbilitySystemComponent->GetNumericAttribute(UEnemyAttributeSet::GetMaxDazeAttribute())};
	return CurrentDaze >= MaxDaze;
}


void UEnemyCombatComponent::OnDazeChanged(const FOnAttributeChangeData& Data)
{
	// todo: IsDead

	float MaxDaze = AbilitySystemComponent->GetNumericAttribute(UEnemyAttributeSet::GetMaxDazeAttribute());
	if (Data.OldValue < MaxDaze && Data.NewValue >= MaxDaze)
	{

	}
}

void UEnemyCombatComponent::EnterStunState()
{
	if (bIsStunned || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	bIsStunned = true;
	AbilitySystemComponent->AddLooseGameplayTag(Combat::Status::Enemy::Stunned);
	UpdateBlackBoardStunState();
	
	CancelCurrentAction();

	GetWorld()->GetTimerManager().SetTimer(
		StunRecoveryTimerHandle,
		this,
		&UEnemyCombatComponent::ExitStunState,
		StunDuration,
		false);
}

void UEnemyCombatComponent::ExitStunState()
{
	if (!bIsStunned || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	bIsStunned = false;
	AbilitySystemComponent->RemoveLooseGameplayTag(Combat::Status::Enemy::Stunned);
	UpdateBlackBoardStunState();
	
	// Reset Daze
	ApplyGameplayEffectOnSelf(AbilitySystemComponent, ResetDazeGE);
}

void UEnemyCombatComponent::UpdateBlackBoardStunState()
{
	if (!IsValid(Character))
	{
		return;
	}

	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(Character->GetController()))
	{
		AIController->SetBBBool(AI::BlackBoard::IsStunned, bIsStunned);
	}
}
