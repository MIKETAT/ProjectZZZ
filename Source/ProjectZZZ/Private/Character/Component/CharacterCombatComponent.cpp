// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Component/CharacterCombatComponent.h"

#include "GameplayEffect.h"
#include "KismetTraceUtils.h"
#include "AbilitySystem/AgentAttributeSet.h"
#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/CharacterBase.h"
#include "Player/PlayerCharacter.h"
#include "Utility/KismetCustomTraceUtils.h"
#include "Utility/ZZZGameplayTag.h"

UCharacterCombatComponent::UCharacterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCharacterCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeCombatStepList();

	// Bind Delegate
	
	if (IsValid(AnimInstance))
	{
		AnimInstance->OnCombatWindowChanged.AddDynamic(this, &ThisClass::HandleCombatWindowChange);
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
		int32 InstanceID = ExecuteAction(TargetAction);

		if (InstanceID != INDEX_NONE)
		{
			// Execute successfully. Apply Cost. Reset Status.
			PayActionCost(TargetAction);
			if (CurrentExecutionState.CurrentStep)
			{
				UE_LOG(LogTemp, Error, TEXT("Execute New Action: %s. ReSet Previous State"), *CurrentExecutionState.CurrentStep->ActionName.ToString());	
			}
			CurrentExecutionState.Reset();
			CurrentExecutionState.CurrentStep = TargetAction;
			CurrentExecutionState.MontageInstanceId = InstanceID;
			CurrentExecutionState.bHasSuccessfullyStarted = true;
			
			PendingIntent.Reset();
		}
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
		int32 InstanceID = ExecuteAction(PendingIntent.ActionStep);

		if (InstanceID != INDEX_NONE)
		{
			// Execute successfully. Apply Cost. Reset Status.
			PayActionCost(PendingIntent.ActionStep);
			if (CurrentExecutionState.CurrentStep)
			{
				UE_LOG(LogTemp, Error, TEXT("Execute Buffered Action: %s. ReSet Previous State"), *CurrentExecutionState.CurrentStep->ActionName.ToString());	
			}
			CurrentExecutionState.Reset();
			CurrentExecutionState.CurrentStep = PendingIntent.ActionStep;
			CurrentExecutionState.MontageInstanceId = InstanceID;
			CurrentExecutionState.bHasSuccessfullyStarted = true;
			
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

void UCharacterCombatComponent::ProcessHitFeedback(const FAttackResult& Result)
{
}

void UCharacterCombatComponent::InjectAndBindASC(UAgentAbilitySystemComponent* InASC)
{
	Super::InjectAndBindASC(InASC);
	
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}
	
	// Agent Attribute Set
	if (const UAgentAttributeSet* AgentAttributeSet = AbilitySystemComponent->GetSet<UAgentAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UAgentAttributeSet::GetEnergyAttribute()).AddUObject(this, &UCharacterCombatComponent::OnEnergyChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UAgentAttributeSet::GetDecibelsAttribute()).AddUObject(this, &UCharacterCombatComponent::OnDecibelsChanged);
	}
}

void UCharacterCombatComponent::ExecuteSwitchInAction()
{
	ExecuteSwitchAction(SwitchInMontage);
}

void UCharacterCombatComponent::ExecuteSwitchOutAction()
{
	ExecuteSwitchAction(SwitchOutMontage);
}

void UCharacterCombatComponent::ExecuteSwitchAction(UAnimMontage* Montage)
{
	if (!IsValid(Montage))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Switch Montage"));
		return;
	}
	
	if (IsAnyActionActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("Exists Active Action. Play Switch In Action Failed"));
		return;
	}

	if (!IsValid(CombatAnimSchedulerComponent))
	{
		return;
	}
	
	FCombatAnimExecutionRequest Request;
	Request.Montage = Montage;
	
	// todo: CurrentExecutionState的更新需要规范一下
	CurrentExecutionState.CurrentStep = nullptr;
	CurrentExecutionState.MontageInstanceId = CombatAnimSchedulerComponent->ExecuteAnimRequest(Request);
	CurrentExecutionState.bHasSuccessfullyStarted = true;
}


void UCharacterCombatComponent::OnEnergyChanged(const FOnAttributeChangeData& Data)
{
}

void UCharacterCombatComponent::OnDecibelsChanged(const FOnAttributeChangeData& Data)
{
}

void UCharacterCombatComponent::OnDazeChanged(const FOnAttributeChangeData& Data)
{
	// todo: IsDead

	float MaxDaze = AbilitySystemComponent->GetNumericAttribute(UEnemyAttributeSet::GetMaxDazeAttribute());
	if (Data.NewValue >= MaxDaze/* not stun yet */)
	{
		// stun
		CancelCurrentAction();

		HandleStun();

	}
	
	
}



void UCharacterCombatComponent::HandleStun()
{
	
}

