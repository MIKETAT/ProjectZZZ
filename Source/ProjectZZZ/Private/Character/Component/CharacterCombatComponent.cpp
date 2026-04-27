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
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Kismet/KismetStringLibrary.h"
#include "Utility/KismetCustomTraceUtils.h"
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

	RefreshAttackDetection(DeltaTime);
	
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
	Mesh = Character ? Character->GetMesh() : nullptr;
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

	if (TargetAction)
	{
		TargetAction->Montage = TargetAction->GetAnimMontage(Character->GetCharacterFrameDataBus());
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

	BindAttributeListeners();		// todo 挑选时机
}

bool UCharacterCombatComponent::IsAllowMovementInterruptAction() const
{
	return CurrentExecutionState.CurrentStep && CurrentExecutionState.bMovementInterruptWindowOpen;
}

void UCharacterCombatComponent::HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason)
{
	if (RequestID != CurrentExecutionState.MontageInstanceId)
	{
		return;
	}
	
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

void UCharacterCombatComponent::ProcessHitEvent(AActor* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config)
{
	if (!IsValid(Victim) || !IsValid(Character))
	{
		return;
	}
	
	FAttackContext AttackContext;
	AttackContext.Instigator = Character;
	AttackContext.HitResult = HitResult;
	AttackContext.PayloadConfig = Config;
	AttackContext.SourceASC = AbilitySystemComponent;

	//CombatContext.SourceAction = const_cast<UCombatActionStep*>(CurrentExecutionState.CurrentStep);
	
	if (UCombatComponentBase* Component = Victim->FindComponentByClass<UCombatComponentBase>())
	{
		FAttackResult Result;
		Component->HandleIncomingDamage(AttackContext, Result);
	}
}

void UCharacterCombatComponent::HandleIncomingDamage(const FAttackContext& Context, FAttackResult& OutResult)
{
	// Valid Check
	if (!Context.IsContextValid())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC{Context.SourceASC.Get()};
	UAbilitySystemComponent* TargetASC{Character->GetAbilitySystemComponent()};

	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		// Feedback? Invalid
		return;
	}

	// Todo: Dodge/Parry 等检查放在一个函数中？
	
	// Dodge Check
	if (AbilitySystemComponent->HasMatchingGameplayTag(Combat::Gait::Dodge))
	{
		// Dodged. Triggered XXX
		//Context.bWasEvaded = true;
		return;
	}
	// Parry Check
	
	// Apply Damage
	FGameplayEffectContextHandle ContextHandle{SourceASC->MakeEffectContext()};
	ContextHandle.AddSourceObject(GetOwner());

	if (Context.HitResult.bBlockingHit)
	{
		ContextHandle.AddHitResult(Context.HitResult);
	}

	FGameplayEffectSpecHandle SpecHandle{SourceASC->MakeOutgoingSpec(DamageEffectClass, 1, ContextHandle)};
	if (!SpecHandle.IsValid())
	{
		// Feedback Invalid
		return;
	}

	FGameplayEffectSpec* Spec{SpecHandle.Data.Get()};
	
	Spec->SetSetByCallerMagnitude(Combat::Data::DamageMultiplier, Context.PayloadConfig.DamageMultiplier);
	Spec->SetSetByCallerMagnitude(Combat::Data::DazeMultiplier, Context.PayloadConfig.DazeMultiplier);

	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec);
}

void UCharacterCombatComponent::ProcessHitFeedback(const FAttackResult& Result)
{
	

	
}

void UCharacterCombatComponent::EnableAttackDetection(UCombatActionStep* ActionStep, const FHitShapeConfig& Config)
{
	AttackDetectionConfig.bEnableAttackDetection = true;
	AttackDetectionConfig.ShapeConfig = Config;
	AttackDetectionConfig.AttackingAction = ActionStep;
	AttackDetectionConfig.WeaponSweepState.Reset();
	AttackDetectionConfig.HitActors.Empty();
}

void UCharacterCombatComponent::DisableAttackDetection()
{
	AttackDetectionConfig.bEnableAttackDetection = false;
	AttackDetectionConfig.AttackingAction = nullptr;
	AttackDetectionConfig.WeaponSweepState.Reset();
	AttackDetectionConfig.HitActors.Empty();
}

// Todo: 未来CombatComponent如果拆分, 这些监听就可以分开实现了
void UCharacterCombatComponent::BindAttributeListeners()
{
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	// Base AttributeSet
	if (const UBaseCombatAttributeSet* BaseAttributeSet = AbilitySystemComponent->GetSet<UBaseCombatAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UBaseCombatAttributeSet::GetHealthAttribute()).AddUObject(this, &UCharacterCombatComponent::OnHealthChanged);
	}

	// Agent Attribute Set
	if (const UAgentAttributeSet* AgentAttributeSet = AbilitySystemComponent->GetSet<UAgentAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UAgentAttributeSet::GetEnergyAttribute()).AddUObject(this, &UCharacterCombatComponent::OnEnergyChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UAgentAttributeSet::GetDecibelsAttribute()).AddUObject(this, &UCharacterCombatComponent::OnDecibelsChanged);
	}
	
	// EnemyAttribute Set
	if (const UEnemyAttributeSet* EnemyAttributeSet = AbilitySystemComponent->GetSet<UEnemyAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UEnemyAttributeSet::GetDazeAttribute()).AddUObject(this, &UCharacterCombatComponent::OnDazeChanged);
	}
	
}

void UCharacterCombatComponent::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && Data.OldValue > 0.f)
	{
		HandleDeath();
	}
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

	}
	
	
}


void UCharacterCombatComponent::HandleDeath()
{
	Character->Die();
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

FTransform UCharacterCombatComponent::CalculateShapeWorldTransform() const
{
	FTransform BaseTransform{FTransform::Identity};
	
	if (!IsValid(Mesh) || !AttackDetectionConfig.ShapeConfig.IsValid())
	{
		return BaseTransform;
	}
	
	if (AttackDetectionConfig.ShapeConfig.AttackBoneName != NAME_None)
	{
		BaseTransform = Mesh->GetSocketTransform(AttackDetectionConfig.ShapeConfig.AttackBoneName, RTS_World);
	} else
	{
		BaseTransform = Mesh->GetComponentTransform();
	}

	return AttackDetectionConfig.ShapeConfig.RelativeTransform * BaseTransform;
}

void UCharacterCombatComponent::RefreshAttackDetection(float DeltaTime)
{
	if (!AttackDetectionConfig.bEnableAttackDetection)
	{
		return;
	}
	
	if (!IsValid(Mesh))
	{
		return;
	}
	
	UWorld* World = Mesh->GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	
	RefreshWeaponSweepDirection(DeltaTime);
	
	FTransform CurrentTransform{CalculateShapeWorldTransform()};
	FVector CurrentLocation{CurrentTransform.GetLocation()};
	FQuat CurrentRotation{CurrentTransform.GetRotation()};
	FVector SweepDirection = AttackDetectionConfig.WeaponSweepState.WeaponSweepDirection;

	FVector StartLocation{FVector::ZeroVector};
	FVector EndLocation{FVector::ZeroVector};
	FQuat ShapeRotation = CurrentRotation * AttackDetectionConfig.ShapeConfig.ShapeOrientation.Quaternion();

	switch (AttackDetectionConfig.ShapeConfig.ShapeType)
	{
		case EHitShapeType::Box:
			StartLocation = CurrentTransform.TransformPosition(AttackDetectionConfig.ShapeConfig.ShapeCenter);
			EndLocation = StartLocation + SweepDirection * 2.f * FMath::Max(AttackDetectionConfig.ShapeConfig.BoxHalfExtents.X, AttackDetectionConfig.ShapeConfig.BoxHalfExtents.Y);
			break;
		case EHitShapeType::Sphere:
			StartLocation = CurrentTransform.TransformPosition(AttackDetectionConfig.ShapeConfig.ShapeCenter);
			EndLocation = StartLocation + SweepDirection * 2.f * AttackDetectionConfig.ShapeConfig.SphereRadius;
			break;
		case EHitShapeType::Capsule:
			StartLocation = CurrentTransform.TransformPosition(AttackDetectionConfig.ShapeConfig.ShapeCenter);
			EndLocation = StartLocation + SweepDirection * 2.f * AttackDetectionConfig.ShapeConfig.CapsuleRadius;
			break;
	}
	
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Mesh->GetOwner());
	CollisionParams.bTraceComplex = false;

	FCollisionShape Shape = AttackDetectionConfig.ShapeConfig.GetCollisionShape();

	TArray<FHitResult> Hits;
	bool bHit = World->SweepMultiByChannel(
		Hits,
		StartLocation,
		EndLocation,
		ShapeRotation,
		ECC_Pawn,
		Shape,
		CollisionParams);

	// Draw Debug
	bool bShowDebug = true;
	FLinearColor HitColor = FLinearColor::Red;
	FLinearColor TraceColor = FLinearColor::Green;
	float DrawTime = 2.f;
	
	if (bShowDebug)
	{
		switch (AttackDetectionConfig.ShapeConfig.ShapeType)
		{
			case EHitShapeType::Sphere:
				DrawDebugSphereTraceMulti(World, StartLocation, EndLocation, AttackDetectionConfig.ShapeConfig.SphereRadius, EDrawDebugTrace::ForDuration,
					bHit, Hits, TraceColor, HitColor, DrawTime);
				break;
			case EHitShapeType::Capsule:
				DrawDebugCapsuleTraceMulti_WithOrientation(World, StartLocation, EndLocation, ShapeRotation, AttackDetectionConfig.ShapeConfig.CapsuleRadius, AttackDetectionConfig.ShapeConfig.CapsuleHalfHeight,
					EDrawDebugTrace::ForDuration, bHit, Hits, TraceColor, HitColor, DrawTime);
				break;
			case EHitShapeType::Box:
				DrawDebugBoxTraceMulti(World, StartLocation, EndLocation, AttackDetectionConfig.ShapeConfig.BoxHalfExtents, ShapeRotation.Rotator(), EDrawDebugTrace::ForDuration,
					bHit, Hits, TraceColor, HitColor, DrawTime);
				break;
		}
	}

	if (bHit)
	{
		 for (auto& Hit : Hits)
		 {
		 	AActor* HitActor = Hit.GetActor();
		 	if (HitActor && !AttackDetectionConfig.HitActors.Contains(HitActor))
		 	{
		 		AttackDetectionConfig.HitActors.Add(HitActor);
		 		UE_LOG(LogTemp, Error, TEXT("Hit Actor: %s"), *HitActor->GetName());
		 		if (IsValid(AttackDetectionConfig.AttackingAction))
		 		{
		 			ProcessHitEvent(HitActor, Hit, AttackDetectionConfig.AttackingAction.Get()->HitPayloadConfig);	
		 		}
		 	}
		 }
	}
}

void UCharacterCombatComponent::RefreshWeaponSweepDirection(float DeltaTime)
{
	FVector WeaponPos{AttackDetectionConfig.ShapeConfig.ShapeCenter};
	const FTransform CurrentTransform{CalculateShapeWorldTransform()};

	FVector WeaponEndPose{CurrentTransform.TransformPosition(WeaponPos)};
	FVector LastWeaponEndPose{AttackDetectionConfig.WeaponSweepState.LastFrameWeaponEndPosition};

	FVector WeaponSweepDirection{WeaponEndPose - LastWeaponEndPose};
	if (!WeaponSweepDirection.Normalize())
	{
		return;
	}

	float BlendAlpha = DeltaTime / AttackDetectionConfig.WeaponSweepState.DirectionBlendTime;
	BlendAlpha = FMath::Clamp(BlendAlpha, 0.2f, 1);

	FQuat LastWeaponDirectionQuat{AttackDetectionConfig.WeaponSweepState.WeaponSweepDirection.ToOrientationQuat()};
	FQuat TargetWeaponDirectionQuat{WeaponSweepDirection.ToOrientationQuat()};
	FQuat CurrentWeaponDirectionQuat = FQuat::Slerp(LastWeaponDirectionQuat, TargetWeaponDirectionQuat, BlendAlpha);
	AttackDetectionConfig.WeaponSweepState.WeaponSweepDirection = CurrentWeaponDirectionQuat.GetForwardVector();
	AttackDetectionConfig.WeaponSweepState.LastFrameWeaponEndPosition = WeaponEndPose;
}
