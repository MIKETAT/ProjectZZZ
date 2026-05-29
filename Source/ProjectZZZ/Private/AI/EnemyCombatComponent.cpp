#include "AI/EnemyCombatComponent.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
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
	UE_LOG(LogTemp, Error, TEXT("Attack Detection: Enemy got Hit"));

	// Daze
	bool bIsDazeValueFull{IsDazeValueFull()};
	// todo: already in stun state
	if (bIsDazeValueFull && Context.PayloadConfig.bIsHeavyAttack/* && Not Stun Yet*/)
	{
		// Trigger Chain Attack
		if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>())
		{
			FCharacterDeathPayload Payload;
			EventBus->BroadcastEvent(Combat::Event::ChainAttack, Context.Instigator, Context.Target, Context.Instigator, Payload);
		}
		// Cancel Current Action/Play Stun Montage/Add Stun Tag
		return;
	}

	// Not Stun	
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
	
}

int32 UEnemyCombatComponent::ExecuteAction(const UCombatActionStep* ActionStep)
{
	if (!IsValid(ActionStep) || !IsValid(CombatAnimSchedulerComponent))
	{
		return INDEX_NONE;
	}

	if (IsDazeValueFull())
	{
		return INDEX_NONE;
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
		CurrentExecutionState.Reset();
		CurrentExecutionState.CurrentStep = ActionStep;
		CurrentExecutionState.MontageInstanceId = InstanceID;
		CurrentExecutionState.bHasSuccessfullyStarted = true;
	}
	
	return InstanceID;
}

float UEnemyCombatComponent::GetCurrentActionParryOffset() const
{
	float ParryOffset{0.f};

	if (IsValid(CurrentExecutionState.CurrentStep) && CurrentExecutionState.CurrentStep->ParriedActionConfig.bIsParriedAction)
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
			// Hit Fly?
			{
				checkf(Character, TEXT("Enemy Character Invalid"));
				if (UHitStopComponent* HitStopComponent = Character->GetHitStopComponent())
				{
					UE_LOG(LogTemp, Log, TEXT("Enemy Attack was Parried. Apply HitStop on Enemy"));
					HitStopComponent->ApplyHitStop(Result.HitStopDuration, Result.HitStopTimeScale);
				}
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
		// stun
		CancelCurrentAction();

		HandleStun();
	}
}

void UEnemyCombatComponent::HandleStun()
{
	if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>())
	{
		// 
	}
}

