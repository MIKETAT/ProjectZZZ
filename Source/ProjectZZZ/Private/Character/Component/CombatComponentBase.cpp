#include "Character/Component/CombatComponentBase.h"

#include "KismetTraceUtils.h"
#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/CharacterBase.h"
#include "Player/PlayerCharacter.h"
#include "Utility/KismetCustomTraceUtils.h"
#include "Utility/ZZZGameplayTag.h"

UCombatComponentBase::UCombatComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UCombatComponentBase::BeginPlay()
{
	Super::BeginPlay();
	
	CachePointers();

	if (IsValid(CombatAnimSchedulerComponent))
	{
		CombatAnimSchedulerComponent->OnAnimRequestFinished.AddDynamic(this, &ThisClass::HandleAnimFinished);
	}
}

void UCombatComponentBase::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	RefreshAttackDetection(DeltaTime);
}

void UCombatComponentBase::ProcessHitEvent(AActor* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config)
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
	
	if (UCombatComponentBase* Component = Victim->FindComponentByClass<UCombatComponentBase>())
	{
		FAttackResult Result;
		Component->HandleIncomingDamage(AttackContext, Result);
	}
}

void UCombatComponentBase::HandleIncomingDamage(const FAttackContext& Context, FAttackResult& OutResult)
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

void UCombatComponentBase::EnableAttackDetection(UCombatActionStep* ActionStep, const FHitShapeConfig& ShapeConfig)
{
	AttackDetectionConfig.bEnableAttackDetection = true;
	AttackDetectionConfig.ShapeConfig = ShapeConfig;
	AttackDetectionConfig.AttackingAction = ActionStep;
	AttackDetectionConfig.WeaponSweepState.Reset();
	AttackDetectionConfig.HitActors.Empty();
}

void UCombatComponentBase::DisableAttackDetection()
{
	AttackDetectionConfig.bEnableAttackDetection = false;
	AttackDetectionConfig.AttackingAction = nullptr;
	AttackDetectionConfig.WeaponSweepState.Reset();
	AttackDetectionConfig.HitActors.Empty();
}

bool UCombatComponentBase::IsAnyActionActive() const
{
	return CurrentExecutionState.CurrentStep && CurrentExecutionState.bHasSuccessfullyStarted; 
}

void UCombatComponentBase::CancelCurrentAction()
{
	if (CurrentExecutionState.CurrentStep)
	{
		CombatAnimSchedulerComponent->CancelAnimRequest(CurrentExecutionState.MontageInstanceId);
	}
}

bool UCombatComponentBase::CanInterruptCurrentAction(const UCombatActionStep* Step) const
{
	if (!IsValid(Step) || !IsValid(CurrentExecutionState.CurrentStep))
	{
		return false;
	} 
	// Todo: Define explicit interruption conditions
	return Step->Priority > CurrentExecutionState.CurrentStep->Priority || CurrentExecutionState.bIsRecoveryWindowOpen;
}

FTransform UCombatComponentBase::CalculateShapeWorldTransform() const
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

void UCombatComponentBase::RefreshAttackDetection(float DeltaTime)
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

void UCombatComponentBase::RefreshWeaponSweepDirection(float DeltaTime)
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

void UCombatComponentBase::CachePointers()
{
	Character = Cast<ACharacterBase>(GetOwner());
	Mesh = Character ? Character->GetMesh() : nullptr;

	if (Character && Mesh)
	{
		AnimInstance = Cast<UAnimInstanceBase>(Mesh->GetAnimInstance());
		AbilitySystemComponent = Cast<UAgentAbilitySystemComponent>(Character->GetAbilitySystemComponent());
		CombatAnimSchedulerComponent = Cast<UCombatAnimSchedulerComponent>(Character->GetCombatAnimSchedulerComponent());
	}
}

int32 UCombatComponentBase::ExecuteAction(const UCombatActionStep* Step)
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


void UCombatComponentBase::HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason)
{
	if (RequestID != CurrentExecutionState.MontageInstanceId)
	{
		return;
	}
	CurrentExecutionState.Reset();

	if (APlayerCharacter* Player = Cast<APlayerCharacter>(Character))
	{
		if (Player->GetAgentPresence() == EAgentPresenceState::Lingering)
		{
			Player->SwitchToOffField();
		}
	}
}

void UCombatComponentBase::InjectAndBindASC(UAgentAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC))
	{
		return;
	}

	AbilitySystemComponent = InASC;

	if (const UBaseCombatAttributeSet* BaseAttributeSet = AbilitySystemComponent->GetSet<UBaseCombatAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UBaseCombatAttributeSet::GetHealthAttribute()).AddUObject(this, &UCombatComponentBase::OnHealthChanged);
	}
	
	// EnemyAttribute Set
	/*if (const UEnemyAttributeSet* EnemyAttributeSet = AbilitySystemComponent->GetSet<UEnemyAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UEnemyAttributeSet::GetDazeAttribute()).AddUObject(this, &UCharacterCombatComponent::OnDazeChanged);
	}*/
}


void UCombatComponentBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && Data.OldValue > 0.f)
	{
		HandleDeath();
	}
}


void UCombatComponentBase::HandleDeath()
{
	Character->Die();
}