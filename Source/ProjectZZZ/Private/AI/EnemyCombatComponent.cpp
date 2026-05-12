#include "AI/EnemyCombatComponent.h"

#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Combat/ZZZCombatEventTypes.h"
#include "Utility/ZZZGameplayTag.h"

UEnemyCombatComponent::UEnemyCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyCombatComponent::HandleIncomingDamage(const FAttackContext& Context, FAttackResult& OutResult)
{
	Super::HandleIncomingDamage(Context, OutResult);

	bool bIsDazeValueFull{IsDazeValueFull()};

	// todo: already in stun state

	if (bIsDazeValueFull && Context.PayloadConfig.bIsHeavyAttack/* && Not Stun Yet*/)
	{
		// Trigger Chain Attack
		if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>())
		{
			FTestDeathPayload Payload;
			EventBus->BroadcastEvent(Combat::Event::ChainAttack, nullptr, nullptr, nullptr, Payload);
		}
		
		// Cancel Current Action/Play Stun Montage/Add Stun Tag
	}
	
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
	Super::ProcessHitFeedback(Result);
	
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

