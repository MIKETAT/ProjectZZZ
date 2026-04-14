// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Component/CharacterCombatComponent.h"

#include "GameplayEffect.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/CharacterBase.h"
#include "Kismet/KismetStringLibrary.h"
#include "Utility/ZZZGameplayTag.h"

UCharacterCombatComponent::UCharacterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UCharacterCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeCombatStepList();

	// Bind Delegate
	if (IsValid(CombatAnimSchedulerComponent))
	{
		CombatAnimSchedulerComponent->OnAnimRequestFinished.AddDynamic(this, &ThisClass::HandleAnimFinished);
	}

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

	if (CurrentExecutionState.CurrentStep)
	{
		GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Green,
		FString::Printf(TEXT("Current Action: %s, ProceedWindowOpen: %s"), *CurrentExecutionState.CurrentStep->ActionName.ToString(),
		*UKismetStringLibrary::Conv_BoolToString(CurrentExecutionState.bProceedWindowOpen)));	
	}
	
}

void UCharacterCombatComponent::InitializeComponent()
{
	Super::InitializeComponent();
	Character = Cast<ACharacterBase>(GetOwner());

	// Bind Delegate
	
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

const UCombatActionStep* UCharacterCombatComponent::SelectTargetAction() const
{
	const UCombatActionStep* TargetAction{nullptr};
	CurrentInputActionBitmask.ForEachSetAction([&](EInputAction InputAction)
	{
		const UCombatActionStep* ThisAction{SelectComboActionIntent(InputAction)};
		if (ThisAction == nullptr)
		{
			ThisAction = SelectCombatActionIntent(InputAction);
		}

		// current no target or this action has higher priority
		if (TargetAction == nullptr)
		{
			TargetAction = ThisAction;
		} else if (ThisAction->Priority > TargetAction->Priority)
		{
			TargetAction = ThisAction;
		}
	});

	if (TargetAction)
	{
		TargetAction->Montage = TargetAction->GetAnimMontage();
	}
	
	return TargetAction;
}

const UCombatActionStep* UCharacterCombatComponent::SelectComboActionIntent(const EInputAction Input) const
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

const UCombatActionStep* UCharacterCombatComponent::SelectCombatActionIntent(const EInputAction Input) const
{
	for (const UCombatActionStep* Step : CombatActionList)
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
		UE_LOG(LogTemp, Warning, TEXT("Buffer Input Action : %s"), *PendingIntent.ActionStep->ActionName.ToString());
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
		UE_LOG(LogTemp, Warning, TEXT("Buffer Input Action : %s"), *PendingIntent.ActionStep->ActionName.ToString());
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

void UCharacterCombatComponent::TryInitComponents()
{
	if (AbilitySystemComponent || CombatAnimSchedulerComponent)
	{
		return;
	}
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	Character = Cast<ACharacterBase>(Owner);
	if (Character)
	{
		AnimInstance = Cast<UAnimInstanceBase>(Character->GetMesh()->GetAnimInstance());
		AbilitySystemComponent = Cast<UAgentAbilitySystemComponent>(Character->GetAbilitySystemComponent());
		CombatAnimSchedulerComponent = Cast<UCombatAnimSchedulerComponent>(Character->GetCombatAnimSchedulerComponent());
	}
}

bool UCharacterCombatComponent::IsAllowMovementCancelAction() const
{
	return CurrentExecutionState.CurrentStep && CurrentExecutionState.bProceedWindowOpen;
}

void UCharacterCombatComponent::HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason)
{
	if (RequestID != CurrentExecutionState.MontageInstanceId)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("HandleAnimFinished. RequestID = %d, Reset State"), RequestID);
	CurrentExecutionState.Reset();
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
	
	// Proceed Window
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

bool UCharacterCombatComponent::IsAnyActionActive() const
{
	return CurrentExecutionState.CurrentStep && CurrentExecutionState.bHasSuccessfullyStarted; 
}

bool UCharacterCombatComponent::CanInterruptCurrentAction(const UCombatActionStep* Step) const
{
	if (!IsValid(Step) || !IsValid(CurrentExecutionState.CurrentStep))
	{
		return false;
	} 
	// Todo: Define explicit interruption conditions
	return Step->Priority > CurrentExecutionState.CurrentStep->Priority || CurrentExecutionState.bIsRecoveryWindowOpen;
}

int32 UCharacterCombatComponent::ExecuteAction(const UCombatActionStep* Step)
{
	if (!IsValid(Step) || !IsValid(CombatAnimSchedulerComponent))
	{
		return INDEX_NONE;
	}

	if (IsAnyActionActive())
	{
		CombatAnimSchedulerComponent->CancelAnimRequest(CurrentExecutionState.MontageInstanceId);
	}
	
	FCombatAnimExecutionRequest Request;
	Request.Montage = Step->Montage;
	Request.Priority = Step->Priority;
	return CombatAnimSchedulerComponent->ExecuteAnimRequest(Request);
}

void UCharacterCombatComponent::CancelCurrentAction()
{
	if (CurrentExecutionState.CurrentStep)
	{
		CombatAnimSchedulerComponent->CancelAnimRequest(CurrentExecutionState.MontageInstanceId);
	}
	
}
