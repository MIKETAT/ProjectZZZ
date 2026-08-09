#include "Character/Component/CombatComponentBase.h"
#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "Animation/AnimInstanceBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/CharacterBase.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Combat/AttackDetectionGeometry.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Engine/OverlapResult.h"
#include "Player/PlayerCharacter.h"
#include "Utility/ZZZGameplayTag.h"

UCombatComponentBase::UCombatComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	DetectionStatus = FAttackDetectionStatus{};
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
	OutSegment = FResolvedAttackDetectionSegment{};
	const UCombatActionStep* Action{CurrentExecutionState.CurrentStep.Get()};
	if (!Action)
	{
		return false;
	}

	if (!Action->AttackDetectionConfig.bEnableDetection)
	{
		return false;
	}

	const int32 BindingCount = Action->AttackDetectionConfig.CountSegmentBindings(SegmentName);
	if (BindingCount != 1)
	{
		return false;
	}
	
	const FAttackDetectionSegmentBinding* Binding = Action->AttackDetectionConfig.FindSegmentBinding(SegmentName);
	if (!Binding)
	{
		return false;
	}

	FAttackDetectionSpec Spec;
	bool bSpecValid = Binding->ResolveDetectionSpec(Spec);
	if (!bSpecValid)
	{
		return false;
	}

	OutSegment.DetectionSpec = Spec;
	OutSegment.SegmentName = SegmentName;
	OutSegment.DedupePolicy = Binding->DedupePolicy;
	OutSegment.SourceAction = Action;
	OutSegment.ActionRequestId = CurrentExecutionState.MontageInstanceId;

	return true;
}

void UCombatComponentBase::EnableAttackDetection(const UAnimSequenceBase* SourceAnimation, const FName& SegmentName)
{
	if (!IsSourceAnimationFromCurrentActionMontage(SourceAnimation))
	{
		return;
	}

	FResolvedAttackDetectionSegment Segment;
	if (!ResolveAttackDetectionSegment(SegmentName, Segment))
	{
		return;
	}

	if (Segment.DetectionSpec.DetectionMode != EAttackDetectionMode::WeaponSweep
		&& Segment.DetectionSpec.DetectionMode != EAttackDetectionMode::ActorPathSweep
		&& Segment.DetectionSpec.DetectionMode != EAttackDetectionMode::ShapeQueryContinuous)
	{
		UE_LOG(LogTemp, Error, TEXT("DetectionMode is not ContinuousWindow. Should not trigger in NotifyState"));
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

	if (DetectionStatus.DetectionSegment.DetectionSpec.DetectionMode == EAttackDetectionMode::ActorPathSweep
		&& Segment.DetectionSpec.PathSweepRotationPolicy == EActorPathSweepRotationPolicy::LockOnBegin)
	{
		DetectionStatus.LockedForward = Character->GetActorForwardVector();
		DetectionStatus.LockedRotator = Character->GetActorRotation();
	}
}

void UCombatComponentBase::DisableAttackDetection(const UAnimSequenceBase* SourceAnimation, const FName& SegmentName)
{
	if (!IsSourceAnimationFromCurrentActionMontage(SourceAnimation))
	{
		return;
	}
	
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
	return CurrentExecutionState.CurrentStep.IsValid() && CurrentExecutionState.bHasSuccessfullyStarted; 
}

void UCombatComponentBase::CancelCurrentAction()
{
	if (CurrentExecutionState.CurrentStep.IsValid())
	{
		CombatAnimSchedulerComponent->CancelAnimRequest(CurrentExecutionState.MontageInstanceId);
	}
}

bool UCombatComponentBase::CanInterruptCurrentAction(const UCombatActionStep* Step) const
{
	if (!IsValid(Step) || !CurrentExecutionState.CurrentStep.IsValid())
	{
		return false;
	}

	// Allow HitReaction interrupt self
	const bool bCurrentActionIsHitReaction{CurrentExecutionState.CurrentStep->bIsHitReaction};
	const bool bNewActionIsHitReaction{Step->bIsHitReaction};
	if (bCurrentActionIsHitReaction && bNewActionIsHitReaction)
	{
		return Step->Priority >= CurrentExecutionState.CurrentStep->Priority;
	}
	
	// Todo: Define explicit interruption conditions
	return Step->Priority > CurrentExecutionState.CurrentStep->Priority || CurrentExecutionState.bProceedWindowOpen;
}

void UCombatComponentBase::RefreshAttackDetection(float DeltaTime)
{
	if (!DetectionStatus.bActive)
	{
		return;
	}
	
	switch (DetectionStatus.DetectionSegment.DetectionSpec.DetectionMode)
	{
		case EAttackDetectionMode::WeaponSweep:
			RefreshWeaponSweep(DeltaTime);
			break;
		case EAttackDetectionMode::ActorPathSweep:
			RefreshActorPathSweep();
			break;
		case EAttackDetectionMode::ShapeQueryInstant:
		//	todo:Continuous Query
			//RefreshShapeQueryContinuous();
			break;
		default:
			break;
	}
}

void UCombatComponentBase::RefreshWeaponSweep(const float DeltaTime)
{
	if (!Mesh || !GetWorld())
	{
		return;
	}

	const FAttackDetectionSpec& Spec = DetectionStatus.DetectionSegment.DetectionSpec;
	if (!Mesh->DoesSocketExist(Spec.WeaponRootSocketName) || !Mesh->DoesSocketExist(Spec.WeaponTipSocketName))
	{
		return;
	}
	
	if (Spec.DetectionMode != EAttackDetectionMode::WeaponSweep)
	{
		return;
	}

	FTransform CurrentWeaponRootTransform{Mesh->GetSocketTransform(Spec.WeaponRootSocketName)};
	FTransform CurrentWeaponTipTransform{Mesh->GetSocketTransform(Spec.WeaponTipSocketName)};

	FTransform PreviousWeaponRootTransform;
	FTransform PreviousWeaponTipTransform;
	float GeometryDeltaTime = DeltaTime;
	
	if (DetectionStatus.bIsFirstFrame)
	{
		DetectionStatus.bIsFirstFrame = false;

		// overlay for first frame
		PreviousWeaponRootTransform = CurrentWeaponRootTransform;
		PreviousWeaponTipTransform = CurrentWeaponTipTransform;
		GeometryDeltaTime = 0.f;
	} else
	{
		PreviousWeaponRootTransform = DetectionStatus.LastWeaponRootTransform;
		PreviousWeaponTipTransform = DetectionStatus.LastWeaponTipTransform;
	}
	
	DetectionStatus.LastWeaponRootTransform = CurrentWeaponRootTransform;
	DetectionStatus.LastWeaponTipTransform = CurrentWeaponTipTransform;
	
	TArray<FAttackSweepGeometry> Sweeps;
	if (!AttackDetectionGeometry::BuildWeaponSweepGeometry(
		Spec, GeometryDeltaTime,
		PreviousWeaponRootTransform, PreviousWeaponTipTransform,
		CurrentWeaponRootTransform, CurrentWeaponTipTransform,
		Sweeps))
	{
		return;
	}

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.bTraceComplex = false;

	for (const auto& Geometry : Sweeps)
	{
		// Overlap
		if (Geometry.Start.Equals(Geometry.End, UE_KINDA_SMALL_NUMBER))
		{
			TArray<FOverlapResult> OverlapResults;
			GetWorld()->OverlapMultiByChannel(
				OverlapResults,
				Geometry.End,
				Geometry.Rotation,
				Spec.TraceChannel,
				Geometry.CollisionShape,
				CollisionParams);

			const FTransform QueryTransform{Geometry.Rotation, Geometry.End};
			for (const FOverlapResult& OverlapResult : OverlapResults)
			{
				ProcessDetectionResults(MakeDetectedTargetFromOverlap(OverlapResult, QueryTransform), DetectionStatus.DetectionSegment);
			}
			// Draw Debug
			if (DebugConfig.bDrawDebug)
			{
				FAttackShapeQueryGeometry DrawGeometry;
				DrawGeometry.WorldTransform = FTransform(Geometry.Rotation, Geometry.End, FVector::OneVector);
				DrawGeometry.CollisionShape = Geometry.CollisionShape;
				DrawDebugAttackDetectionShape(DrawGeometry);
			}
		} else
		{
			// Sweep
			TArray<FHitResult> HitResults;
			GetWorld()->SweepMultiByChannel(
				HitResults,
				Geometry.Start,
				Geometry.End,
				Geometry.Rotation,
				Spec.TraceChannel,
				Geometry.CollisionShape,
				CollisionParams
			);
		
			for (const FHitResult& HitResult : HitResults)
			{
				ProcessDetectionResults(MakeDetectedTargetFromHit(HitResult), DetectionStatus.DetectionSegment);	
			}
			// Draw Debug
			if (DebugConfig.bDrawDebug)
			{
				bool bHit{!HitResults.IsEmpty()};
				DrawDebugSweepShape(GetWorld(), Geometry.Start, Geometry.End, Geometry.Rotation, Geometry.CollisionShape, bHit, HitResults);
			}
		}
	}
}

void UCombatComponentBase::RefreshActorPathSweep()
{
	if (!GetWorld())
	{
		return;
	}
	
	const FAttackDetectionSpec& Spec{DetectionStatus.DetectionSegment.DetectionSpec};
	if (!DetectionStatus.bActive || Spec.DetectionMode != EAttackDetectionMode::ActorPathSweep)
	{
		return;
	}

	// Enable 时已记录到LastOwnerTransform
	FTransform LastTransform = DetectionStatus.LastOwnerTransform;
	FTransform CurrentTransform{Character->GetActorTransform()};
	DetectionStatus.LastOwnerTransform = CurrentTransform;
	DetectionStatus.bIsFirstFrame = false;

	if (Spec.PathSweepRotationPolicy == EActorPathSweepRotationPolicy::LockOnBegin)
	{
		LastTransform.SetRotation(DetectionStatus.LockedRotator.Quaternion());
		CurrentTransform.SetRotation(DetectionStatus.LockedRotator.Quaternion());
	}
	
	FAttackSweepGeometry Geometry;
	if (!AttackDetectionGeometry::BuildActorPathSweepGeometry(Spec, LastTransform, CurrentTransform, Geometry))
	{
		return;
	}
	
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.bTraceComplex = false;

	// Overlap when zero length
	if (Geometry.Start.Equals(Geometry.End, UE_KINDA_SMALL_NUMBER))
	{	
		TArray<FOverlapResult> OverlapResults;
		GetWorld()->OverlapMultiByChannel(
			OverlapResults,
			Geometry.End,
			Geometry.Rotation,
			Spec.TraceChannel,
			Geometry.CollisionShape,
			CollisionParams);

		const FTransform QueryTransform{Geometry.Rotation, Geometry.End};
		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			ProcessDetectionResults(MakeDetectedTargetFromOverlap(OverlapResult, QueryTransform), DetectionStatus.DetectionSegment);
		}
		
		if (DebugConfig.bDrawDebug)
		{
			FAttackShapeQueryGeometry DebugGeometry;
			DebugGeometry.WorldTransform = FTransform(Geometry.Rotation, Geometry.End, FVector::OneVector);
			DebugGeometry.CollisionShape = Geometry.CollisionShape;
			DrawDebugAttackDetectionShape(DebugGeometry);
		}
	} else
	{
		// Sweep
		TArray<FHitResult> OutHitResult;
		bool bHit = GetWorld()->SweepMultiByChannel(
			OutHitResult,
			Geometry.Start,
			Geometry.End,
			Geometry.Rotation,
			Spec.TraceChannel,
			Geometry.CollisionShape,
			CollisionParams);
		
		for (const FHitResult& HitResult: OutHitResult)
		{
			ProcessDetectionResults(MakeDetectedTargetFromHit(HitResult), DetectionStatus.DetectionSegment);
		}

		if (DebugConfig.bDrawDebug)
		{
			DrawDebugSweepShape(
				GetWorld(),
				Geometry.Start,
				Geometry.End,
				Geometry.Rotation,
				Geometry.CollisionShape,
				bHit,
				OutHitResult,
				DebugConfig.DrawTime);
		}
	}
}

void UCombatComponentBase::TriggerAttackDetectionQuery(const UAnimSequenceBase* SourceAnimation, const FName& SegmentName)
{
	if (!IsSourceAnimationFromCurrentActionMontage(SourceAnimation))
	{
		return;
	}
	
	FResolvedAttackDetectionSegment Segment;
	if (!ResolveAttackDetectionSegment(SegmentName, Segment))
	{
		return;
	}

	const FAttackDetectionSpec& Spec{Segment.DetectionSpec};
	
	if (Spec.DetectionMode != EAttackDetectionMode::ShapeQueryInstant)
	{
		return;
	}

	FTransform ReferenceTransform = FTransform::Identity;
	switch (Spec.ReferenceType)
	{
		case EAttackQueryReference::Owner:
			ReferenceTransform = Character->GetActorTransform();
			break;
		case EAttackQueryReference::OwnerSocket:
			{
				if (!Mesh || !Mesh->DoesSocketExist(Spec.ReferenceSocketName))
				{
					return;
				}
				ReferenceTransform = Mesh->GetSocketTransform(Spec.ReferenceSocketName);
			}
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("Attack Detection Query, Invalid Reference Type."));
			return;
	}
	
	FAttackShapeQueryGeometry Geometry;
	if (!AttackDetectionGeometry::BuildShapeQueryGeometry(Spec, ReferenceTransform, Geometry))
	{
		return;
	}

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.bTraceComplex = false;
	
	TArray<FOverlapResult> OutHitResults;
	GetWorld()->OverlapMultiByChannel(
		OutHitResults,
		Geometry.WorldTransform.GetLocation(),
		Geometry.WorldTransform.GetRotation(),
		Spec.TraceChannel,
		Geometry.CollisionShape,
		CollisionParams);
	
	// Draw Debug
	DrawDebugAttackDetectionShape(Geometry);
	
	for (const FOverlapResult& OverlapResult : OutHitResults)
	{
		ProcessDetectionResults(MakeDetectedTargetFromOverlap(OverlapResult, Geometry.WorldTransform), Segment);
	}
}

// todo: No need for persistent ShapeQuery Currently 
void UCombatComponentBase::RefreshShapeQueryContinuous()
{
	
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

void UCombatComponentBase::ProcessDetectionResults(const FAttackDetectedTarget& DetectedTarget, const FResolvedAttackDetectionSegment& Segment)
{
	AActor* HitActor{DetectedTarget.Actor.Get()};
	if (!HitActor || HitActor == Character)
	{
		return;
	}

	if (Segment.DetectionSpec.DetectionMode != EAttackDetectionMode::None)
	{
		if (!PassHitDedupe(HitActor, Segment))
		{
			return;
		}
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

bool UCombatComponentBase::PassHitDedupe(AActor* HitActor, const FResolvedAttackDetectionSegment& Segment)
{
	if (!HitActor || HitActor == Character)
	{
		return false;
	}

	const TObjectKey<AActor> HitKey{HitActor};

	switch (Segment.DedupePolicy)
	{
		case EHitDedupePolicy::None:
			return true;
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
				if (DetectionStatus.SegmentHitActors.Contains(HitKey))
				{
					return false;
				}
				DetectionStatus.SegmentHitActors.Add(HitKey);
				return true;
			}	
		default:
			return true;
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
			UBaseCombatAttributeSet::GetHealthAttribute()).AddUObject(this, &UCombatComponentBase::HandleHealthChanged);
	}
}


void UCombatComponentBase::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	// UI
	if (!AbilitySystemComponent || !Character)
	{
		return;
	}

	float CurrentHealth{Data.NewValue};
	float MaxHealth{Character->GetBaseCombatAttribute()->GetMaxHealth()};

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	
	if (CurrentHealth <= 0.f && Data.OldValue > 0.f)
	{
		HandleDeath();
	}
}


void UCombatComponentBase::HandleDeath()
{
	Character->Die();
}

bool UCombatComponentBase::IsSourceAnimationFromCurrentActionMontage(const UAnimSequenceBase* SourceAnimation) const
{
	if (!SourceAnimation || !CurrentExecutionState.CurrentStep.IsValid())
	{
		return false;
	}

	return CurrentExecutionState.CurrentStep->Montage == SourceAnimation;
}

void UCombatComponentBase::DebugPrintCurrentActionState()
{
	if (!bShowActionDebugInfo)
	{
		return;
	}

	
	if (CurrentExecutionState.CurrentStep.IsValid())
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

void UCombatComponentBase::DrawDebugAttackDetectionShape(const FAttackShapeQueryGeometry& Geometry)
{
	if (DebugConfig.bDrawDebug)
	{
		const FCollisionShape& Shape = Geometry.CollisionShape;
		const FTransform& Transform = Geometry.WorldTransform;

		// Capsule
		if (Shape.IsCapsule())
		{
			DrawDebugCapsule(
					GetWorld(),
					Transform.GetLocation(),
					Shape.GetCapsuleHalfHeight(),
					Shape.GetCapsuleRadius(),
					Transform.GetRotation(),
					FColor::Green,
					false,
					DebugConfig.DrawTime);
		}
		// Box
		else if (Shape.IsBox()) 
		{
			DrawDebugBox(
				GetWorld(),
				Transform.GetLocation(),
				Shape.GetExtent(),
				Transform.GetRotation(),
				FColor::Green,
				false,
				DebugConfig.DrawTime);
		}
		else if (Shape.IsSphere())
		{
			DrawDebugSphere(
				GetWorld(),
				Transform.GetLocation(),
				Shape.GetSphereRadius(),
				10,
				FColor::Green,
				false,
				DebugConfig.DrawTime);
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
