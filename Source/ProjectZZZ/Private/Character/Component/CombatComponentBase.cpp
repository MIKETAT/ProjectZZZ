#include "Character/Component/CombatComponentBase.h"
#include "KismetTraceUtils.h"
#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/CharacterBase.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Engine/OverlapResult.h"
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
	
	if (IsValid(CombatAnimSchedulerComponent))
	{
		CombatAnimSchedulerComponent->OnAnimRequestFinished.AddDynamic(this, &ThisClass::HandleAnimFinished);
	}
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

bool UCombatComponentBase::ResolveAttackDetectionSegment(const FName& SegmentName, FResolvedAttackDetectionSegment& OutSegment)
{
	if (!IsValid(CurrentExecutionState.CurrentStep))
	{
		return false;
	}

	const FAttackDetectionConfig& Config{CurrentExecutionState.CurrentStep->AttackDetectionConfig};
	if (!Config.bEnableDetection)
	{
		return false;
	}

	const FAttackDetectionSegmentBinding* Binding = Config.Segments.FindByPredicate(
		[SegmentName](const FAttackDetectionSegmentBinding& SegmentBinding)
		{
			return SegmentBinding.SegmentName == SegmentName;
		});

	if (!Binding)
	{
		return false;
	}

	FAttackDetectionSpec Spec;
	if (Binding->SpecSource == EAttackDetectorSpecSource::Preset)
	{
		if (!IsValid(Binding->Preset))
		{
			return false;
		}
		Spec =  Binding->Preset->DetectionSpec;
	} else
	{
		Spec = Binding->InlineSpec;
	}

	OutSegment.DetectionSpec = Spec;
	OutSegment.SegmentName = SegmentName;
	OutSegment.DedupePolicy = Binding->DedupePolicy;
	OutSegment.SourceAction = CurrentExecutionState.CurrentStep.Get();
	OutSegment.ActionRequestId = CurrentExecutionState.MontageInstanceId;

	return true;
}

void UCombatComponentBase::EnableAttackDetection(const FGameplayTag& Tag, const FName& SegmentName)
{
	if (!IsValid(CurrentExecutionState.CurrentStep) || Tag != CurrentExecutionState.CurrentStep->ActionTag)
	{
		return;
	}

	FResolvedAttackDetectionSegment Segment;
	if (!ResolveAttackDetectionSegment(SegmentName, Segment))
	{
		return;
	}

	if (Segment.DetectionSpec.TriggerMode != EAttackDetectionTriggerMode::ContinuousWindow)
	{
		UE_LOG(LogTemp, Error, TEXT("Detection TriggerMode is not ContinuousWindow. Should not trigger in NotifyState"));
		return;
	}

	if (DetectionStatus.bActive)
	{
		UE_LOG(LogTemp, Error, TEXT("Overlapped Attack Detection"));
		DetectionStatus.ResetActivationState();
		return;
	}
	
	DetectionStatus.ResetActivationState();

	DetectionStatus.DetectionSegment = Segment;
	DetectionStatus.bActive = true;
	DetectionStatus.bIsFirstFrame = true;
	DetectionStatus.LastOwnerTransform = Character->GetActorTransform();

	if (Segment.DetectionSpec.PathSweepRotationPolicy == EActorPathSweepRotaionPolicy::LockOnBegin)
	{
		DetectionStatus.LockedForward = Character->GetActorForwardVector();
		DetectionStatus.LockedRotator = Character->GetActorRotation();
	}
}

void UCombatComponentBase::DisableAttackDetection(const FName& SegmentName)
{
	if (!DetectionStatus.bActive)
	{
		return;
	}

	if (DetectionStatus.DetectionSegment.SegmentName != SegmentName)
	{
		UE_LOG(LogTemp, Error, TEXT("Attack Detection Segment Name mismatch."));
		return;
	}
	
	DetectionStatus.ResetActivationState();
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

	// Allow HitReaction interrput self
	const bool bCurrentActionIsHitReaction{CurrentExecutionState.CurrentStep->bIsHitReaction};
	const bool bNewActionIsHitReaction{Step->bIsHitReaction};
	if (bCurrentActionIsHitReaction && bNewActionIsHitReaction)
	{
		return Step->Priority >= CurrentExecutionState.CurrentStep->Priority;
	}
	
	// Todo: Define explicit interruption conditions
	return  Step->Priority > CurrentExecutionState.CurrentStep->Priority || CurrentExecutionState.bProceedWindowOpen;
}

void UCombatComponentBase::RefreshAttackDetection(float DeltaTime)
{
	if (!DetectionStatus.bActive)
	{
		return;
	}

	if (DetectionStatus.DetectionSegment.ActionRequestId != CurrentExecutionState.MontageInstanceId)
	{
		DetectionStatus.ResetAll();
		return;
	}
	
	switch (DetectionStatus.DetectionSegment.DetectionSpec.DetectionMode)
	{
		case EAttackDetectionMode::WeaponSweep:
			RefreshWeaponSweep();
			break;
		case EAttackDetectionMode::ActorPathSweep:
			RefreshActorPathSweep();
			break;
		case EAttackDetectionMode::ShapeQuery:
		//	Continuous Query
			RefreshShapeQuery();	
			break;
		default:
			break;
	}
}

void UCombatComponentBase::RefreshWeaponSweep()
{
	if (DetectionStatus.DetectionSegment.DetectionSpec.DetectionMode != EAttackDetectionMode::WeaponSweep)
	{
		return;
	}	
	
	FTransform LastWeaponRootTransform{DetectionStatus.LastWeaponRootTransform};
	FTransform LastWeaponTipTransform{DetectionStatus.LastWeaponTipTransform};
	
	FTransform CurrentWeaponRootTransform{Mesh->GetSocketTransform(DetectionStatus.DetectionSegment.DetectionSpec.WeaponRootSocketName)};
	FTransform CurrentWeaponTipTransform{Mesh->GetSocketTransform(DetectionStatus.DetectionSegment.DetectionSpec.WeaponTipSocketName)};

	// Update Weapon Socket Transform.
	DetectionStatus.LastWeaponRootTransform = CurrentWeaponRootTransform;
	DetectionStatus.LastWeaponTipTransform = CurrentWeaponTipTransform;
	
	// Initialize Weapon Socket Transform In First Frame.
	if (DetectionStatus.bIsFirstFrame)
	{
		DetectionStatus.bIsFirstFrame = false;
		return;
	}
	
	FTransform LastSubStepWeaponRootTransform{LastWeaponRootTransform};
	FTransform LastSubStepWeaponTipTransform{LastWeaponTipTransform};
	
	const int32 SubStepCount{DetectionStatus.DetectionSegment.DetectionSpec.SubStepCount};
	// Sweep
	for (int32 Step = 0; Step < SubStepCount; Step++)
	{
		float Alpha = (Step + 1) / static_cast<float>(SubStepCount);
		FTransform CurrentSubStepWeaponRootTransform = UKismetMathLibrary::TLerp(LastWeaponRootTransform, CurrentWeaponRootTransform, Alpha);
		FTransform CurrentSubStepWeaponTipTransform = UKismetMathLibrary::TLerp(LastWeaponTipTransform, CurrentWeaponTipTransform, Alpha);
		
		SubStepAttackDetection(LastSubStepWeaponRootTransform, LastSubStepWeaponTipTransform,
			CurrentSubStepWeaponRootTransform, CurrentSubStepWeaponTipTransform);

		LastSubStepWeaponRootTransform = CurrentSubStepWeaponRootTransform;
		LastSubStepWeaponTipTransform = CurrentSubStepWeaponTipTransform;
	}
}

void UCombatComponentBase::SubStepAttackDetection(const FTransform& LastWeaponRootTransform, const FTransform& LastWeaponTipTransform,
													const FTransform& CurrentWeaponRootTransform, const FTransform& CurrentWeaponTipTransform)
{
	const FAttackDetectionSpec& Spec{DetectionStatus.DetectionSegment.DetectionSpec};
	const int32 SampleCount{Spec.SampleCount};
	
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.bTraceComplex = false;
	
	FCollisionShape Shape = Spec.SweepShapeConfig.GetCollisionShape();
	
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
			Spec.TraceChannel,
			Shape,
			CollisionParams);

		if (DebugConfig.bDrawDebug)
		{
			bool bHit{false};
			TArray<FHitResult> Hits;
		
			switch (Spec.SweepShapeConfig.ShapeType)
			{
				case ESweepShapeType::Sphere:
					DrawDebugSphereTraceMulti(
						GetWorld(),
						LastSampleLocation,
						CurrentSampleLocation,
						Spec.SweepShapeConfig.SphereRadius,
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

		for (const FHitResult& HitResult : TempHits)
		{
			ProcessDetectionResults(MakeDetectedTargetFromHit(HitResult), DetectionStatus.DetectionSegment, DetectionStatus.HitActors);	
		}
	}
}

void UCombatComponentBase::RefreshActorPathSweep()
{
	const FAttackDetectionSpec& Spec{DetectionStatus.DetectionSegment.DetectionSpec};
	
	if (!DetectionStatus.bActive || Spec.DetectionMode != EAttackDetectionMode::ActorPathSweep)
	{
		return;
	}

	FTransform LastTransform{DetectionStatus.LastOwnerTransform};
	FTransform CurrentTransform{Character->GetActorTransform()};
	
	DetectionStatus.LastOwnerTransform = CurrentTransform;

	if (DetectionStatus.bIsFirstFrame)
	{
		DetectionStatus.bIsFirstFrame = false;
		return;
	}

	if (Spec.PathSweepRotationPolicy == EActorPathSweepRotaionPolicy::LockOnBegin)
	{
		LastTransform.SetRotation(DetectionStatus.LockedRotator.Quaternion());
		CurrentTransform.SetRotation(DetectionStatus.LockedRotator.Quaternion());
	}

	const FVector Start{LastTransform.TransformPosition(Spec.SweepShapeLocalOffset.GetLocation())};
	const FVector End{CurrentTransform.TransformPosition(Spec.SweepShapeLocalOffset.GetLocation())};

	const FQuat BaseRotation{Spec.PathSweepRotationPolicy == EActorPathSweepRotaionPolicy::LockOnBegin
		? DetectionStatus.LockedRotator.Quaternion()
		: Character->GetActorQuat()};

	const FQuat TraceRotation{BaseRotation * Spec.SweepShapeConfig.ShapeRotation.Quaternion()};
	
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.bTraceComplex = false;

	TArray<FHitResult> OutHitResult;
	bool bHit = GetWorld()->SweepMultiByChannel(
		OutHitResult,
		Start,
		End,
		TraceRotation,
		Spec.TraceChannel,
		Spec.SweepShapeConfig.GetCollisionShape(),
		CollisionParams);

	if (DebugConfig.bDrawDebug)
	{
		DrawDebugSweepShape(
			GetWorld(),
			Start,
			End,
			TraceRotation,
			Spec.SweepShapeConfig.GetCollisionShape(),
			bHit,
			OutHitResult,
			DebugConfig.DrawTime);
	}
	
	for (const FHitResult& HitResult: OutHitResult)
	{
		ProcessDetectionResults(MakeDetectedTargetFromHit(HitResult), DetectionStatus.DetectionSegment, DetectionStatus.HitActors);
	}
}

void UCombatComponentBase::TriggerAttackDetectionQuery(const FGameplayTag& Tag, const FName& SegmentName)
{
	if (!CurrentExecutionState.CurrentStep || Tag != CurrentExecutionState.CurrentStep->ActionTag)
	{
		return;
	}
	
	FResolvedAttackDetectionSegment Segment;
	if (!ResolveAttackDetectionSegment(SegmentName, Segment))
	{
		return;
	}

	const FAttackDetectionSpec& Spec{Segment.DetectionSpec};
	
	if (Spec.DetectionMode != EAttackDetectionMode::ShapeQuery || Spec.TriggerMode != EAttackDetectionTriggerMode::InstantQuery)
	{
		return;
	}

	FTransform TargetTransform{FTransform::Identity};
	if (Spec.ReferenceType == EAttackQueryReference::Owner)
	{
		FTransform BaseTransform{Character->GetActorTransform()};
		TargetTransform.SetLocation(BaseTransform.TransformPosition(Spec.QueryLocalOffset.GetLocation()));
		TargetTransform.SetRotation(BaseTransform.GetRotation() * Spec.QueryLocalOffset.GetRotation());
	} else if (Spec.ReferenceType == EAttackQueryReference::OwnerSocket)
	{
		FTransform ReferenceTransform{Character->GetMesh()->GetSocketTransform(Spec.ReferenceSocketName)};
		TargetTransform.SetLocation(ReferenceTransform.TransformPosition(Spec.QueryLocalOffset.GetLocation()));
		TargetTransform.SetRotation(ReferenceTransform.TransformRotation(Spec.QueryLocalOffset.GetRotation() * Spec.SweepShapeConfig.ShapeRotation.Quaternion()));
	}

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.bTraceComplex = false;
	
	TArray<FOverlapResult> OutHitResults;
	GetWorld()->OverlapMultiByChannel(
		OutHitResults,
		TargetTransform.GetLocation(),
		TargetTransform.GetRotation(),
		Spec.TraceChannel,
		Spec.SweepShapeConfig.GetCollisionShape(),
		CollisionParams);
	
	// Draw Debug
	DrawDebugAttackDetectionShape(Spec.SweepShapeConfig, TargetTransform);
	
	TSet<TObjectKey<AActor>> InstantQueryHitActors;
	for (const FOverlapResult& OverlapResult : OutHitResults)
	{
		ProcessDetectionResults(MakeDetectedTargetFromOverlap(OverlapResult, TargetTransform), Segment, InstantQueryHitActors);
	}
}

// todo: No need for persistent ShapeQuery Currently 
void UCombatComponentBase::RefreshShapeQuery()
{
	if (DetectionStatus.DetectionSegment.DetectionSpec.TriggerMode != EAttackDetectionTriggerMode::ContinuousWindow)
	{
		return;
	}
	
}

FAttackDetectedTarget UCombatComponentBase::MakeDetectedTargetFromHit(const FHitResult& HitResult)
{
	FAttackDetectedTarget Target;
	Target.Actor = HitResult.GetActor();
	Target.Component = HitResult.GetComponent();
	Target.bIsHitResult = true;
	Target.HitResult = HitResult;
	Target.QueryLocation = HitResult.ImpactPoint;
	
	return Target;
}

FAttackDetectedTarget UCombatComponentBase::MakeDetectedTargetFromOverlap(const FOverlapResult& OverlapResult, const FTransform& QueryTransform)
{
	FAttackDetectedTarget Target;
	Target.Actor = OverlapResult.GetActor();
	Target.Component = OverlapResult.GetComponent();
	Target.bIsHitResult = false;
	Target.QueryLocation = QueryTransform.GetLocation();

	FHitResult HitResult;
	HitResult.ImpactPoint = QueryTransform.GetLocation();
	HitResult.Component = OverlapResult.GetComponent();
	HitResult.Location = QueryTransform.GetLocation();
	HitResult.TraceStart = QueryTransform.GetLocation();
	HitResult.TraceEnd = QueryTransform.GetLocation();
	
	Target.HitResult = HitResult;
	
	return Target;
}

void UCombatComponentBase::ProcessDetectionResults(const FAttackDetectedTarget& DetectedTarget, const FResolvedAttackDetectionSegment& Segment, TSet<TObjectKey<AActor>>& ActivationHitActors)
{
	AActor* HitActor{DetectedTarget.Actor.Get()};
	if (!HitActor || HitActor == Character)
	{
		return;
	}

	bool bPassHitDedupe{false};
	if (Segment.DetectionSpec.TriggerMode == EAttackDetectionTriggerMode::InstantQuery)
	{
		bPassHitDedupe = PassHitDedupe(HitActor, Segment, ActivationHitActors);	
	} else if (Segment.DetectionSpec.TriggerMode == EAttackDetectionTriggerMode::ContinuousWindow)
	{
		bPassHitDedupe = PassHitDedupe(HitActor, Segment, DetectionStatus.HitActors);
	}
	
	if (!bPassHitDedupe)
	{
		return;
	}

	ACharacterBase* Victim{Cast<ACharacterBase>(HitActor)};
	if (!Victim)
	{
		return;
	}
	
	if (const UCombatActionStep* SourceAction = Segment.SourceAction.Get())
	{
		ProcessHitEvent(Victim, DetectedTarget.HitResult, SourceAction->HitPayloadConfig);
	}
}

bool UCombatComponentBase::PassHitDedupe(AActor* HitActor, const FResolvedAttackDetectionSegment& Segment, TSet<TObjectKey<AActor>>& ActivationHitActors)
{
	if (!HitActor || HitActor == Character)
	{
		return false;
	}

	const TObjectKey<AActor> HitKey{HitActor};

	switch (Segment.DedupePolicy)
	{
		case EHitDedupePolicy::None:
			return false;
		case EHitDedupePolicy::OncePerAction:
			{
				if (DetectionStatus.ActionHitActors.Contains(HitKey))
				{
					return false;
				}
				DetectionStatus.ActionHitActors.Add(HitKey);
				return true;
			}
		case EHitDedupePolicy::OncePerActivation:
			{
				if (ActivationHitActors.Contains(HitKey))
				{
					return false;
				}
				ActivationHitActors.Add(HitKey);
				return true;
			}	
		default:
			return false;
	}
}

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
	
	if (HitReactionAction && (!IsAnyActionActive() || CanInterruptCurrentAction(HitReactionAction)))
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
	
	OnCombatActionFinished.Broadcast(Cast<APlayerCharacter>(Character), Reason);

	CurrentExecutionState.Reset();
	DetectionStatus.ResetAll();
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

	}


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
	
	GEngine->AddOnScreenDebugMessage(
				7,
				0.f,
				FColor::Green,
				FString::Printf(TEXT("bMovementInterruptWindowOpen = %s"),
					CurrentExecutionState.bMovementInterruptWindowOpen ? TEXT("true") : TEXT("false"))
			);

}

void UCombatComponentBase::DrawDebugAttackDetectionShape(const FSweepShapeConfig& ShapeConfig, const FTransform& TargetTransform)
{
	if (DebugConfig.bDrawDebug)
	{
		switch (ShapeConfig.ShapeType)
		{
		case ESweepShapeType::Capsule:
			{
				DrawDebugCapsule(
					GetWorld(),
					TargetTransform.GetLocation(),
					ShapeConfig.CapsuleHalfHeight,
					ShapeConfig.CapsuleRadius,
					TargetTransform.GetRotation(),
					FColor::Green,
					false,
					DebugConfig.DrawTime);
			}
			break;
		case ESweepShapeType::Box:
			{
				DrawDebugBox(
					GetWorld(),
					TargetTransform.GetLocation(),
					ShapeConfig.BoxHalfExtents,
					FColor::Green,
					false,
					DebugConfig.DrawTime);
			}
			break;
		case ESweepShapeType::Sphere:
			{
				DrawDebugSphere(
					GetWorld(),
					TargetTransform.GetLocation(),
					ShapeConfig.SphereRadius,
					10,
					FColor::Green,
					false,
					DebugConfig.DrawTime);
			}
			break;
		}
	}
}

void UCombatComponentBase::DrawDebugSweepShape(const UWorld* World, const FVector& Start, const FVector& End,
	const FQuat& TraceRotation, const FCollisionShape& Shape, const bool bHit, const TArray<FHitResult>& HitResults,
	const float LifeTime, const float Thickness)
{
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!World)
    {
        return;
    }

    const FColor TraceColor = bHit ? FColor::Red : FColor::Green;
    const FColor HitColor = FColor::Yellow;

    DrawDebugLine(
        World,
        Start,
        End,
        TraceColor,
        false,
        LifeTime,
        0,
        Thickness
    );

    if (Shape.IsSphere())
    {
        const float Radius = Shape.GetSphereRadius();

        DrawDebugSphere(World, Start, Radius, 16, TraceColor, false, LifeTime, 0, Thickness);
        DrawDebugSphere(World, End, Radius, 16, TraceColor, false, LifeTime, 0, Thickness);

        // Sphere Sweep 的整体范围，本质上接近一根 Capsule。
        const FVector Delta = End - Start;
        if (!Delta.IsNearlyZero())
        {
            const FVector Center = (Start + End) * 0.5f;
            const float HalfHeight = Delta.Size() * 0.5f + Radius;
            const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(Delta.GetSafeNormal()).ToQuat();

            DrawDebugCapsule(
                World,
                Center,
                HalfHeight,
                Radius,
                CapsuleRotation,
                TraceColor,
                false,
                LifeTime,
                0,
                Thickness
            );
        }
    }
    else if (Shape.IsCapsule())
    {
        const float Radius = Shape.GetCapsuleRadius();
        const float HalfHeight = Shape.GetCapsuleHalfHeight();

        DrawDebugCapsule(World, Start, HalfHeight, Radius, TraceRotation, TraceColor, false, LifeTime, 0, Thickness);
        DrawDebugCapsule(World, End, HalfHeight, Radius, TraceRotation, TraceColor, false, LifeTime, 0, Thickness);

        // 画几条边线，表示 Capsule 从 Start 移动到 End。
        const FVector X = TraceRotation.GetAxisX();
        const FVector Y = TraceRotation.GetAxisY();

        DrawDebugLine(World, Start + X * Radius, End + X * Radius, TraceColor, false, LifeTime, 0, Thickness);
        DrawDebugLine(World, Start - X * Radius, End - X * Radius, TraceColor, false, LifeTime, 0, Thickness);
        DrawDebugLine(World, Start + Y * Radius, End + Y * Radius, TraceColor, false, LifeTime, 0, Thickness);
        DrawDebugLine(World, Start - Y * Radius, End - Y * Radius, TraceColor, false, LifeTime, 0, Thickness);
    }
    else if (Shape.IsBox())
    {
        const FVector Extent = Shape.GetExtent();

        DrawDebugBox(World, Start, Extent, TraceRotation, TraceColor, false, LifeTime, 0, Thickness);
        DrawDebugBox(World, End, Extent, TraceRotation, TraceColor, false, LifeTime, 0, Thickness);

        const FVector X = TraceRotation.GetAxisX();
        const FVector Y = TraceRotation.GetAxisY();
        const FVector Z = TraceRotation.GetAxisZ();

        for (int32 SX = -1; SX <= 1; SX += 2)
        {
            for (int32 SY = -1; SY <= 1; SY += 2)
            {
                for (int32 SZ = -1; SZ <= 1; SZ += 2)
                {
                    const FVector Offset =
                        X * Extent.X * SX +
                        Y * Extent.Y * SY +
                        Z * Extent.Z * SZ;

                    DrawDebugLine(
                        World,
                        Start + Offset,
                        End + Offset,
                        TraceColor,
                        false,
                        LifeTime,
                        0,
                        Thickness
                    );
                }
            }
        }
    }
    else if (Shape.IsLine())
    {
        DrawDebugLine(World, Start, End, TraceColor, false, LifeTime, 0, Thickness);
    }

    for (const FHitResult& Hit : HitResults)
    {
        DrawDebugSphere(World, Hit.ImpactPoint, 8.0f, 8, HitColor, false, LifeTime, 0, Thickness);

        DrawDebugLine(
            World,
            Hit.ImpactPoint,
            Hit.ImpactPoint + Hit.ImpactNormal * 40.0f,
            HitColor,
            false,
            LifeTime,
            0,
            Thickness
        );
    }
#endif
}
