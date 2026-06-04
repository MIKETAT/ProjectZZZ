#include "Character/Component/CombatComponentBase.h"
#include "KismetTraceUtils.h"
#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/CharacterBase.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/PlayerCharacter.h"
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
}

void UCombatComponentBase::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	RefreshAttackDetection(DeltaTime);
	DebugPrintCurrentActionState();
}

USquadManagerComponent* UCombatComponentBase::GetSquadManagerComponent() const
{
	if (!Character)
	{
		return nullptr;
	}

	if (AZZZPlayerController* PC = Cast<AZZZPlayerController>(Character->GetController()))
	{
		return PC->GetSquadManagerComponent();
	}
	return nullptr;
}

void UCombatComponentBase::ProcessHitEvent(ACharacterBase* Victim, const FHitResult& HitResult, const FHitPayloadConfig& Config)
{
	if (!IsValid(Victim) || !IsValid(Character))
	{
		return;
	}
	
	FAttackContext AttackContext;
	AttackContext.Instigator = Character;
	AttackContext.Target = Victim;
	AttackContext.HitResult = HitResult;
	AttackContext.PayloadConfig = Config;
	AttackContext.InstigatorASC = AbilitySystemComponent;
	
	if (UCombatComponentBase* CombatComponent = Victim->GetCombatComp())
	{
		FAttackResult Result;
		Result.HitFeedbackEffectOnSelf = Config.HitFeedbackEffectOnSelf;
		CombatComponent->HandleIncomingDamage(AttackContext, Result);
		ProcessHitFeedback(Result);
	}
}

void UCombatComponentBase::EnableAttackDetection(const FGameplayTag& Tag, UAttackDetectionConfig* DetectionConfig)
{
	if (!IsValid(CurrentExecutionState.CurrentStep))
	{
		return;
	}

	if (Tag != CurrentExecutionState.CurrentStep->ActionTag)
	{
		return;
	}

	DetectionStatus.bEnableAttackDetection = true;
	DetectionStatus.DetectionConfig = DetectionConfig;
	DetectionStatus.AttackingAction = CurrentExecutionState.CurrentStep;
	DetectionStatus.HitActors.Empty();
	DetectionStatus.bIsFirstFrame = true;
}

void UCombatComponentBase::DisableAttackDetection()
{
	DetectionStatus.bEnableAttackDetection = false;
	DetectionStatus.HitActors.Empty();
	DetectionStatus.AttackingAction = nullptr;
	DetectionStatus.DetectionConfig = nullptr;
	DetectionStatus.LastWeaponRootTransform = FTransform::Identity;
	DetectionStatus.LastWeaponTipTransform = FTransform::Identity;
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
	return  Step->Priority > CurrentExecutionState.CurrentStep->Priority || CurrentExecutionState.bIsRecoveryWindowOpen;
}

/*FTransform UCombatComponentBase::CalculateShapeWorldTransform() const
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
}*/

void UCombatComponentBase::RefreshAttackDetection(float DeltaTime)
{
	if (!DetectionStatus.bEnableAttackDetection || !IsValid(Mesh) || !IsValid(DetectionStatus.DetectionConfig))
	{
		return;
	}
	
	FTransform LastWeaponRootTransform{DetectionStatus.LastWeaponRootTransform};
	FTransform LastWeaponTipTransform{DetectionStatus.LastWeaponTipTransform};
	
	FTransform CurrentWeaponRootTransform{Mesh->GetSocketTransform(DetectionStatus.DetectionConfig->WeaponRootSocketName)};
	FTransform CurrentWeaponTipTransform{Mesh->GetSocketTransform(DetectionStatus.DetectionConfig->WeaponTipSocketName)};

	// Update Weapon Socket Transform.
	DetectionStatus.LastWeaponRootTransform = CurrentWeaponRootTransform;
	DetectionStatus.LastWeaponTipTransform = CurrentWeaponTipTransform;
	
	// Initialize Weapon Socket Transform In First Frame.
	if (DetectionStatus.bIsFirstFrame)
	{
		DetectionStatus.bIsFirstFrame = false;
		return;
	}
	
	FTransform LastSubStepWeaponRootTransform{DetectionStatus.LastWeaponRootTransform};
	FTransform LastSubStepWeaponTipTransform{DetectionStatus.LastWeaponTipTransform};

	const int32 SubStepCount{DetectionStatus.DetectionConfig->SubStepCount};
	// Sweep
	for (int32 Step = 0; Step < SubStepCount; Step++)
	{
		float Alpha = Step / static_cast<float>(SubStepCount);
		FTransform CurrentSubStepWeaponRootTransform = UKismetMathLibrary::TLerp(LastWeaponRootTransform, CurrentWeaponRootTransform, Alpha);
		FTransform CurrentSubStepWeaponTipTransform = UKismetMathLibrary::TLerp(LastWeaponTipTransform, CurrentWeaponTipTransform, Alpha);
		
		SubStepAttackDetection(LastSubStepWeaponRootTransform, LastSubStepWeaponTipTransform,
			CurrentSubStepWeaponRootTransform, CurrentSubStepWeaponTipTransform);

		LastSubStepWeaponRootTransform = CurrentSubStepWeaponRootTransform;
		LastSubStepWeaponTipTransform = CurrentSubStepWeaponTipTransform;
	}
}

void UCombatComponentBase::SubStepAttackDetection(	const FTransform& LastWeaponRootTransform, const FTransform& LastWeaponTipTransform,
													const FTransform& CurrentWeaponRootTransform, const FTransform& CurrentWeaponTipTransform)
{
	const int32 SampleCount{DetectionStatus.DetectionConfig->SampleCount};
	
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.bTraceComplex = false;
	FCollisionShape Shape = DetectionStatus.DetectionConfig->SweepShapeConfig.GetCollisionShape();
	
	for (int32 i = 0; i < SampleCount; i++)
	{
		float Alpha = (SampleCount > 1) ? i / static_cast<float>(SampleCount - 1) : 0.f;

		FVector LastSampleLocation{FMath::Lerp(LastWeaponRootTransform.GetLocation(), LastWeaponTipTransform.GetLocation(), Alpha)};
		FVector CurrentSampleLocation{FMath::Lerp(CurrentWeaponRootTransform.GetLocation(), CurrentWeaponTipTransform.GetLocation(), Alpha)};

		TArray<FHitResult> TempHits;
		GetWorld()->SweepMultiByChannel(
			TempHits,
			LastSampleLocation,
			CurrentSampleLocation,
			FQuat::Identity,
			DetectionStatus.DetectionConfig->Channel,
			Shape,
			CollisionParams);

		if (DebugConfig.bDrawDebug)
		{
			bool bHit{false};
			TArray<FHitResult> Hits;
		
			switch (DetectionStatus.DetectionConfig->SweepShapeConfig.ShapeType)
			{
				case ESweepShapeType::Sphere:
					DrawDebugSphereTraceMulti(
						GetWorld(),
						LastSampleLocation,
						CurrentSampleLocation,
						DetectionStatus.DetectionConfig->SweepShapeConfig.SphereRadius,
						EDrawDebugTrace::ForDuration,
						bHit,
						Hits,
						DebugConfig.TraceColor,
						DebugConfig.HitColor,
						DebugConfig.DrawTime);
				break;
				case ESweepShapeType::Box:
				case ESweepShapeType::Capsule:
				default:
				break;
			}
		}
		
		// Process Hit Result
		for (auto& Hit : TempHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && !DetectionStatus.HitActors.Contains(HitActor))
			{
				DetectionStatus.HitActors.Add(HitActor);
				UE_LOG(LogTemp, Error, TEXT("Hit Actor: %s"), *HitActor->GetName());

				ACharacterBase* Victim{Cast<ACharacterBase>(HitActor)};
				 
				if (IsValid(DetectionStatus.AttackingAction))
				{
					ProcessHitEvent(Victim, Hit, DetectionStatus.AttackingAction.Get()->HitPayloadConfig);
				}
			}
		}
	}
	
}

/*void UCombatComponentBase::RefreshWeaponSweepDirection(float DeltaTime)
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
}*/

UAbilitySystemComponent* UCombatComponentBase::GetAbilitySystemComponent() const
{
	if (!IsValid(Character))
	{
		return nullptr;
	}
	return Character->GetAbilitySystemComponent();
}

void UCombatComponentBase::ApplyGameplayEffectOnTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const TSubclassOf<UGameplayEffect>& GE)
{
	if (!IsValid(SourceASC) || !IsValid(TargetASC) || !IsValid(GE))
	{
		UE_LOG(LogTemp, Warning, TEXT("Apply GameplayEffect Failed."));
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddInstigator(Character, Character);
	FGameplayEffectSpecHandle SpecHandle{SourceASC->MakeOutgoingSpec(GE, 1.f, ContextHandle)};
	if (!SpecHandle.IsValid())
	{
		return;
	}
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UCombatComponentBase::ApplyGameplayEffectOnSelf(UAbilitySystemComponent* ASC, const TSubclassOf<UGameplayEffect>& GE)
{
	ApplyGameplayEffectOnTarget(ASC, ASC, GE);
}

void UCombatComponentBase::ApplyImpactEffectOnTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FAttackContext& Context)
{
	if (!IsValid(SourceASC) || !IsValid(TargetASC) || !Context.IsContextValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Apply Impact GameplayEffect Failed."));
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddInstigator(Character, Character);
	if (Context.HitResult.bBlockingHit)
	{
		ContextHandle.AddHitResult(Context.HitResult);	
	}
	
	FGameplayEffectSpecHandle SpecHandle{SourceASC->MakeOutgoingSpec(Context.PayloadConfig.ImpactEffectOnTarget, 1.f, ContextHandle)};

	if (!SpecHandle.IsValid())
	{
		return;
	}

	FGameplayEffectSpec* Spec{SpecHandle.Data.Get()};
	Spec->SetSetByCallerMagnitude(Combat::Data::DamageMultiplier, Context.PayloadConfig.DamageMultiplier);
	Spec->SetSetByCallerMagnitude(Combat::Data::DazeMultiplier, Context.PayloadConfig.DazeMultiplier);
	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec);
}

EHitReactionDirection UCombatComponentBase::CalculateHitReactionDirection(const FVector& AttackerLocation) const
{
	if (!IsValid(Character))
	{
		return EHitReactionDirection::None;
	}

	FVector MyForward{Character->GetActorForwardVector()};
	FVector MyRight{Character->GetActorRightVector()};
	FVector MyLocation{Character->GetActorLocation()};

	FVector DirToAttacker{(AttackerLocation - MyLocation).GetSafeNormal()};

	double ForwardDot{FVector::DotProduct(MyForward, DirToAttacker)};
	double RightDot{FVector::DotProduct(MyRight, DirToAttacker)};

	if (ForwardDot > 0.f)
	{
		return EHitReactionDirection::Front;
	} else
	{
		return EHitReactionDirection::Back;
	}
}

void UCombatComponentBase::ExecuteHitReaction(const AActor* Instigator, const EAttackStrength Strength)
{
	EHitReactionDirection Direction{EHitReactionDirection::Front};
	if (Instigator)
	{
		Direction = CalculateHitReactionDirection(Instigator->GetActorLocation());	
	}
	
	const UCombatActionStep* HitReactionAction{nullptr};
	if (const FDirectionalHitReactionActions* Actions = HitReactionMap.Find(Strength))
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
		ExecuteAction(HitReactionAction, FCombatActionContext());
	}
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

int32 UCombatComponentBase::ExecuteAction(const UCombatActionStep* ActionStep, const FCombatActionContext& Context)
{
	return INDEX_NONE;
}

void UCombatComponentBase::HandleAnimFinished(int32 RequestID, ECombatAnimRequestFinishReason Reason)
{
	if (RequestID != CurrentExecutionState.MontageInstanceId)
	{
		return;
	}

	// todo: 先cast. 后续根据敌人是否拥有同样的逻辑(HandleAnimFinished是否是玩家独有的)修改
	OnCombatActionFinished.Broadcast(Cast<APlayerCharacter>(Character));
	
	UE_LOG(LogTemp, Error, TEXT("Action Anim Finished, Montage = %s, Request ID = %d, Reason = %s"),
	*CurrentExecutionState.CurrentStep->Montage->GetName(),
	RequestID, *UEnum::GetValueAsString(Reason));

	CurrentExecutionState.Reset();
	/*if (GetSquadManagerComponent())
	{
		GetSquadManagerComponent()->ResetTargetEnemy();
	}*/
}

void UCombatComponentBase::InjectAndBindASC(UAgentAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC))
	{
		return;
	}

	AbilitySystemComponent = InASC;

	if (AbilitySystemComponent->GetSet<UBaseCombatAttributeSet>())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UBaseCombatAttributeSet::GetHealthAttribute()).AddUObject(this, &UCombatComponentBase::OnHealthChanged);
	}
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

void UCombatComponentBase::DebugPrintCurrentActionState()
{
	if (!bShowActionDebugInfo)
	{
		return;
	}
	
	if (CurrentExecutionState.CurrentStep)
	{
		
		GEngine->AddOnScreenDebugMessage(
			0,
			0.f,
			FColor::White,
			FString::Printf(TEXT("Name = %s"), *CurrentExecutionState.CurrentStep->GetName())
		);

		GEngine->AddOnScreenDebugMessage(
			1,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("bInputBufferWindowOpen = %s"),
				CurrentExecutionState.bInputBufferWindowOpen ? TEXT("true") : TEXT("false"))
		);

		GEngine->AddOnScreenDebugMessage(
			2,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("bProceedWindowOpen = %s"),
				CurrentExecutionState.bProceedWindowOpen ? TEXT("true") : TEXT("false"))
		);

		GEngine->AddOnScreenDebugMessage(
			3,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("bIsRecoveryWindowOpen = %s"),
				CurrentExecutionState.bIsRecoveryWindowOpen ? TEXT("true") : TEXT("false"))
		);

		GEngine->AddOnScreenDebugMessage(
			4,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("bParryWindowOpen = %s"),
				CurrentExecutionState.bParryWindowOpen ? TEXT("true") : TEXT("false"))
		);

		GEngine->AddOnScreenDebugMessage(
			5,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("bHasConfirmedNextAction = %s"),
				CurrentExecutionState.bHasConfirmedNextAction ? TEXT("true") : TEXT("false"))
		);

		GEngine->AddOnScreenDebugMessage(
			6,
			0.f,
			FColor::Green,
			FString::Printf(TEXT("bHasSuccessfullyStarted = %s"),
				CurrentExecutionState.bHasSuccessfullyStarted ? TEXT("true") : TEXT("false"))
		);
	}
}
