#include "Character/Component/HitDetectionComponent.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Character/CharacterBase.h"
#include "Components/LineBatchComponent.h"
#include "Engine/OverlapResult.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"

UHitDetectionComponent::UHitDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHitDetectionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Character = Cast<ACharacterBase>(GetOwner());
	OwnerMesh = Character ? Character->GetMesh() : nullptr;
	if (!OwnerMesh)
	{
		SetComponentTickEnabled(false);
		return;
	}

	AddTickPrerequisiteComponent(OwnerMesh);
	BuildCollisionData();
}

void UHitDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsHitDetectionActive())
	{
		if (PreviousObservedMontageInstanceID != INDEX_NONE)
		{
			CapturePreviousObservedFrame();
		}
		return;
	}

	FHitDetectionFrameData CurrentFrame;
	if (!BuildCurrentDetectionFrame(CurrentFrame))
	{
		ResetHitDetectionStatus();
		return;
	}

	const FHitDetectionFrameData PreviousFrameBeforeHitDetection = PreviousDetectionFrame;
	if (!PerformHitDetection(CurrentFrame))
	{
		ResetHitDetectionStatus();
		return;
	}

	if (PreviousObservedMontageInstanceID == ActiveMontageInstanceID)
	{
		PreviousObservedFrame = CurrentFrame;
	}

	if (bEnableTrajectoryVisualization || bEnableTrajectoryValidation)
	{
		const double NotifyEndTime = WeaponSocketDamageStateData.Last().FrameTimeInMontage;
		FHitDetectionFrameData ValidationFrame = CurrentFrame;
		bool bHasValidValidationFrame = true;

		// Is current frame exceed NotifyEnd?
		if (CurrentFrame.MontageTime > NotifyEndTime + Tolerance)
		{
			const double FrameInterval = CurrentFrame.MontageTime - PreviousFrameBeforeHitDetection.MontageTime;
			// PreviousFrame also exceed NotifyEnd
			if (PreviousFrameBeforeHitDetection.MontageTime > NotifyEndTime + Tolerance || FrameInterval < Tolerance)
			{
				bHasValidValidationFrame = false;	// can't interpolate
			} else
			{
				// Interp NotifyEnd Frame
				const double Alpha = FMath::Clamp((NotifyEndTime - PreviousFrameBeforeHitDetection.MontageTime) / FrameInterval, 0.f, 1.f);
				ValidationFrame.MontageTime = NotifyEndTime;
				ValidationFrame.BoneToMesh = InterpTransform(PreviousFrameBeforeHitDetection.BoneToMesh, CurrentFrame.BoneToMesh, Alpha);
				ValidationFrame.MeshToWorld = InterpTransform(PreviousFrameBeforeHitDetection.MeshToWorld, CurrentFrame.MeshToWorld, Alpha);
			}
		}
		
		if (bHasValidValidationFrame)
		{
			CaptureRealTimeReferenceCollisionShape(ValidationFrame);
			
			if (bEnableTrajectoryValidation)
			{
				UpdateTrajectoryValidation(PreviousFrameBeforeHitDetection, ValidationFrame, DeltaTime);
			}
		}
	}
	
	if (bPendingDisable)
	{
		if (bEnableTrajectoryValidation)
		{
			FinalizeTrajectoryValidation();
		}
		
		if (bEnableTrajectoryVisualization)
		{
			DrawCompleteTrajectoryView();
		}
		ResetHitDetectionStatus();
	}
}

void UHitDetectionComponent::EnableHitDetection(const FAnimNotifyEventReference& EventReference)
{
	if (!OwnerMesh || IsHitDetectionActive() || CollisionShapes.IsEmpty() || TraceChannel == ECC_MAX)
	{
		return;
	}
	
	const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();
	const UE::Anim::FAnimNotifyMontageInstanceContext* Context = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	if (!Context || !AnimInstance || Context->MontageInstanceID == INDEX_NONE || !NotifyEvent)
	{
		return;
	}

	FAnimMontageInstance* MontageInstance = AnimInstance->GetMontageInstanceForID(Context->MontageInstanceID);
	if (!MontageInstance || !MontageInstance->IsActive())
	{
		return;
	}
	
	if (!CacheDamageStateData(WeaponSocketDamageStateData, WeaponBoneName, EventReference))
	{
		WeaponSocketDamageStateData.Reset();
		return;
	}

	// Cache succeeds. Enable Detection
	ActiveMontageInstanceID = Context->MontageInstanceID;
	ActiveMontageSlotIndex = NotifyEvent->GetSlotIndex();
	
	ReferenceShapeRecord.Reset();
	SweepDebugRecords.Reset();
	ValidationState.Reset();

	const bool bUsePreviousObservedFrame = PreviousObservedFrame.IsValid()
			&& PreviousObservedMontageInstanceID == ActiveMontageInstanceID
			&& PreviousObservedFrame.MontageTime <= MontageInstance->GetPosition() + Tolerance;

	if (bUsePreviousObservedFrame)
	{
		PreviousDetectionFrame = PreviousObservedFrame;
	} else
	{
		PreviousDetectionFrame.Reset();
	}
}

void UHitDetectionComponent::DisableHitDetection(const FAnimNotifyEventReference& EventReference)
{
	if (!IsHitDetectionActive() || !OwnerMesh)
	{
		return;
	}

	const UE::Anim::FAnimNotifyMontageInstanceContext* Context = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	if (!Context || Context->MontageInstanceID != ActiveMontageInstanceID)
	{
		return;
	}
	
	bPendingDisable = true;
}

bool UHitDetectionComponent::PerformHitDetection(const FHitDetectionFrameData& CurrentFrame)
{
	if (!IsHitDetectionActive() || !IsValid(OwnerMesh) || TraceChannel == ECC_MAX
		|| WeaponSocketDamageStateData.IsEmpty())
	{
		return false;
	}
	
	TArray<FHitResult> TotalHitResults;
	FCollisionQueryParams QueryParams = FCollisionQueryParams();
	QueryParams.AddIgnoredActor(Character);
	QueryParams.bTraceComplex = false;
	QueryParams.bFindInitialOverlaps = true;

	const double NotifyEndTime = WeaponSocketDamageStateData.Last().FrameTimeInMontage;
	const bool bCurrentFrameExceedNotifyEnd = CurrentFrame.MontageTime > NotifyEndTime + Tolerance;
	
	// First Detection
	if (!PreviousDetectionFrame.IsValid())
	{
		if (bCurrentFrameExceedNotifyEnd)
		{
			return false;
		}
		
		if (!ProcessRealTimeTickPose(CurrentFrame, TotalHitResults, QueryParams))
		{
			return false;
		}
		PreviousDetectionFrame = CurrentFrame;
		return true;
	}

	// check if miss sample points
	const double DetectionInterval = CurrentFrame.MontageTime - PreviousDetectionFrame.MontageTime;
	if (DetectionInterval < -Tolerance)
	{
		return false;
	}	

	// Has missed sample point
	const double SampleInterval = 1.f / static_cast<double>(SampleCountPerSecond);
	const bool bHasMissedSamplePoint = DetectionInterval > SampleInterval + Tolerance;
	if (bHasMissedSamplePoint || bCurrentFrameExceedNotifyEnd)
	{
		if (!ProcessMissedSamples(PreviousDetectionFrame, CurrentFrame, TotalHitResults, QueryParams))
		{
			return false;
		}
	}

	if (!bCurrentFrameExceedNotifyEnd)
	{
		if (!ProcessRealTimeTickPose(CurrentFrame, TotalHitResults, QueryParams))
		{
			return false;
		}	
	}
	
	PreviousDetectionFrame = CurrentFrame;
	return true;
}

bool UHitDetectionComponent::ProcessWeaponCollisionData(TArray<FHitResult>& TotalHitResults, FDetectionCollisionShapeData& ShapeData,
		const FTransform& ShapeWorldTransform, const FCollisionQueryParams& QueryParams)
{
	UWorld* World = GetWorld();
	if (!OwnerMesh || !World || TraceChannel == ECC_MAX)	
	{
		return false;
	}

	checkf(CollisionShapes.Num() <= 1, TEXT("Only Support one Collision Shape for now."));
	if (!ensureMsgf(ShapeData.BodyName == WeaponBoneName, TEXT("Collision Body name doesn't match Sample Bone Name")))
	{
		return false;
	}
	
	if (ShapeWorldTransform.ContainsNaN())
	{
		return false;
	}

	const FVector CurrentLocation = ShapeWorldTransform.GetLocation();
	const FQuat CurrentRotation = ShapeWorldTransform.GetRotation();
	
	if (!ShapeData.bValidLastLocation)
	{
		TArray<FOverlapResult> InitialOverlapResults;
		
		World->OverlapMultiByChannel(
			InitialOverlapResults,
			CurrentLocation,
			CurrentRotation,
			TraceChannel,
			ShapeData.CollisionShape,
			QueryParams
		);
		
		ShapeData.bValidLastLocation = true;
		ShapeData.LastLocation = CurrentLocation;
		
		return true;
	}
	
	TArray<FHitResult> SweepResults;
	World->SweepMultiByChannel(
		SweepResults,
		ShapeData.LastLocation,
		CurrentLocation,
		CurrentRotation,
		TraceChannel,
		ShapeData.CollisionShape,
		QueryParams
	);

	// Update
	ShapeData.LastLocation = CurrentLocation;
	return true;
}

void UHitDetectionComponent::BuildCollisionData()
{
	CollisionShapes.Reset();

	if (!IsValid(OwnerMesh) || WeaponBoneName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid OwnerMesh, Build Collision Shape Data failed."));
		return;
	}

	const UPhysicsAsset* PhysicsAsset = OwnerMesh->GetPhysicsAsset();
	if (!PhysicsAsset)
	{
		return;
	}
	
	const int32 BodyIndex = PhysicsAsset->FindBodyIndex(WeaponBoneName);
	if (!PhysicsAsset->SkeletalBodySetups.IsValidIndex(BodyIndex))
	{
		return;
	}
	const USkeletalBodySetup* SkeletalBodySetup = PhysicsAsset->SkeletalBodySetups[BodyIndex].Get();
	if (!SkeletalBodySetup)
	{
		return;
	}

	if (SkeletalBodySetup->AggGeom.SphylElems.Num() != 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Only support one collision shape for now, and shape has to be capsule"));
		return;
	}
	
	const FTransform BoneComponentSpace = OwnerMesh->GetBoneTransform(WeaponBoneName, RTS_Component);
	const FVector Scale = BoneComponentSpace.GetScale3D().GetAbs();

	// Only Support Capsule for now.
	CollisionShapes.Reserve(SkeletalBodySetup->AggGeom.SphylElems.Num());
	if (SkeletalBodySetup->AggGeom.GetElementCount() != SkeletalBodySetup->AggGeom.SphylElems.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Body contains unsupported non-capsule shapes."));
	}
	
	for (const auto& Elem : SkeletalBodySetup->AggGeom.SphylElems)
	{
		const float Radius = Elem.GetScaledRadius(Scale);
		const float HalfHeight = Elem.GetScaledHalfLength(Scale);
		if (!FMath::IsFinite(Radius) || !FMath::IsFinite(HalfHeight) || Radius <= 0.f || Radius > HalfHeight)
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid Capsule in body %s"), *SkeletalBodySetup->BoneName.ToString());
			continue;
		}
		
		FDetectionCollisionShapeData& ShapeData = CollisionShapes.Add_GetRef(FDetectionCollisionShapeData{});
		ShapeData.BodyName = SkeletalBodySetup->BoneName;
		ShapeData.CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
		ShapeData.RelativeLocation = Elem.Center * Scale;		// 保持缩放一致
		ShapeData.RelativeRotation = Elem.Rotation.Quaternion().GetNormalized();
		ShapeData.LastLocation = FVector::ZeroVector;
	}
}

void UHitDetectionComponent::ResetHitDetectionStatus()
{
	bPendingDisable = false;
	ActiveMontageInstanceID = INDEX_NONE;
	ActiveMontageSlotIndex = INDEX_NONE;
	
	WeaponSocketDamageStateData.Reset();
	PreviousDetectionFrame.Reset();
	

	for (auto& ShapeDate : CollisionShapes)
	{
		ShapeDate.bValidLastLocation = false;
		ShapeDate.LastLocation = FVector::ZeroVector;
	}
}

FTransform UHitDetectionComponent::CalculateCollisionShapeWorldTransform(const FDetectionCollisionShapeData& ShapeData,
	const FTransform& BoneToMesh, const FTransform& MeshToWorld) const
{
	FTransform NormalizedBoneToMesh = BoneToMesh;
	NormalizedBoneToMesh.SetScale3D(FVector::OneVector);
	NormalizedBoneToMesh.SetRotation(NormalizedBoneToMesh.GetRotation().GetNormalized());

	const FTransform ShapeLocalToBone(ShapeData.RelativeRotation, ShapeData.RelativeLocation, FVector::OneVector);

	return ShapeLocalToBone * NormalizedBoneToMesh * MeshToWorld;
}

void UHitDetectionComponent::DrawCollisionShapeDebug(const FDetectionCollisionShapeData& ShapeData,
	const FTransform& WorldTransform, const FColor& Color, uint8 DepthPriority, float Thickness) const
{
	UWorld* World = GetWorld();
	if (!World || WorldTransform.ContainsNaN())
	{
		return;
	}

	// only support capsule
	if (ShapeData.CollisionShape.ShapeType != ECollisionShape::Capsule)
	{
		return;
	}

	const float Radius = ShapeData.CollisionShape.GetCapsuleRadius();
	const float HalfHeight = ShapeData.CollisionShape.GetCapsuleHalfHeight();
	if (Radius <= 0.f || Radius > HalfHeight)
	{
		return;
	}

	constexpr  float DrawTime = 10.f;
	DrawDebugCapsule(
		World,
		WorldTransform.GetLocation(),
		HalfHeight,
		Radius,
		WorldTransform.GetRotation(),
		Color,
		false,
		DrawTime,
		DepthPriority,
		Thickness
	);

	DrawDebugPoint(
		World,
		WorldTransform.GetLocation(),
		5.f,
		Color,
		false,
		DrawTime,
		DepthPriority
	);
}

bool UHitDetectionComponent::CacheDamageStateData(TArray<FDamageStateData>& OutDamageStateData, const FName& BoneOrSocketName,
                                                  const FAnimNotifyEventReference& EventReference) const
{
	OutDamageStateData.Reset();
	
	if (!OwnerMesh || BoneOrSocketName.IsNone())
	{
		return false;
	}
	
	USkeletalMesh* SkeletalMesh = OwnerMesh->GetSkeletalMeshAsset();
	if (!SkeletalMesh)
	{
		return false;
	}
	
	FTransform ShapeLocalToBone = FTransform::Identity;
	int32 BoneOrSocketMeshBoneIndex = INDEX_NONE;
	if (OwnerMesh->GetSocketInfoByName(BoneOrSocketName, ShapeLocalToBone, BoneOrSocketMeshBoneIndex))
	{
		if (BoneOrSocketMeshBoneIndex == INDEX_NONE)
		{
			return false;
		}
	} else
	{
		BoneOrSocketMeshBoneIndex = OwnerMesh->GetBoneIndex(BoneOrSocketName);
		if (BoneOrSocketMeshBoneIndex == INDEX_NONE)
		{
			return false;
		}
		ShapeLocalToBone = FTransform::Identity;
	}

	// Get AnimMontage
	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	const FAnimNotifyEvent* Event = EventReference.GetNotify();
	const UE::Anim::FAnimNotifyMontageInstanceContext* MontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	if (!AnimInstance || !Event || !MontageContext)
	{
		return false;
	}

	const FAnimMontageInstance* MontageInstance = AnimInstance->GetMontageInstanceForID(MontageContext->MontageInstanceID);
	UAnimMontage* Montage = MontageInstance ? MontageInstance->Montage : nullptr;
	if (!Montage)
	{
		return false;
	}

	// Make sure NotifyState comes from Montage 
	if (EventReference.GetSourceObject() != Montage)
	{
		UE_LOG(LogTemp, Error, TEXT("NotifyState is not from Montage"));
		return false;
	}
	
	const int32 SlotIndex = Event->GetSlotIndex();
	if (!Montage->SlotAnimTracks.IsValidIndex(SlotIndex))
	{
		return false;
	}
	
	const FAnimTrack& AnimTrack = Montage->SlotAnimTracks[SlotIndex].AnimTrack;
	if (AnimTrack.AnimSegments.IsEmpty())
	{
		return false;
	}

	const FReferenceSkeleton& ReferenceSkeleton = SkeletalMesh->GetRefSkeleton();
		
	// Initialize BoneIndexArray in FBoneIndexType
	TArray<FBoneIndexType> RequiredBoneIndices;
	RequiredBoneIndices.SetNumUninitialized(ReferenceSkeleton.GetNum());
	
	for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
	{
		RequiredBoneIndices[BoneIndex] = StaticCast<FBoneIndexType>(BoneIndex);
	}
		
	// Init Base Pose Data
	FBoneContainer BoneContainer;
	BoneContainer.InitializeTo(RequiredBoneIndices, UE::Anim::FCurveFilterSettings{}, *SkeletalMesh);
	BoneContainer.SetUseRAWData(false);		// Raw data or Source Data is invalid in Shipping 
	BoneContainer.SetUseSourceData(false);
	
	// convert SocketMeshBondIndex to CompactBoneIndex	
	const FCompactPoseBoneIndex BoneOrSocketCompactBoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(BoneOrSocketMeshBoneIndex));
	if (!BoneOrSocketCompactBoneIndex.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid SocketCompactBoneIndex"));
		return false;
	}

	if (AnimTrack.IsAdditive())
	{
		UE_LOG(LogTemp, Error, TEXT("Additive Animation is not supported for now."));
		return false;
	}

	FCompactPose CompactPose;
	CompactPose.SetBoneContainer(&BoneContainer);
	FBlendedCurve Curve;
	Curve.InitFrom(BoneContainer);
	
	UE::Anim::FStackAttributeContainer AttributeContainer;
	FCSPose<FCompactPose> ComponentSpacePose;
	
	// Start Sampling Animation Data
	const double StartTimeInMontage = Event->GetTriggerTime();
	const double EndTimeInMontage = Event->GetEndTriggerTime();
	if (EndTimeInMontage <= StartTimeInMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid NotifyState time range."));
		return false;
	}

	// Check SampleInterval
	const double SampleInterval = 1.f / SampleCountPerSecond;
	if (!FMath::IsFinite(SampleInterval) || SampleInterval <= Tolerance)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid SampleInterval"));
		return false;
	}
	const int32 SampleCount = FMath::CeilToInt((EndTimeInMontage - StartTimeInMontage) / SampleInterval) + 1;
	OutDamageStateData.Reserve(SampleCount);

	auto ExtractPoseInAnim = [&] (const double FrameTime) -> bool
	{
		CompactPose.ResetToRefPose();
		Curve.InitFrom(BoneContainer);
		AttributeContainer.Empty();
		
		FAnimationPoseData AnimationPoseData{CompactPose, Curve, AttributeContainer};
		FAnimExtractContext ExtractionContext(FrameTime, true);	// todo:
		AnimTrack.GetAnimationPose(AnimationPoseData, ExtractionContext);

		if (!CompactPose.IsValid() || !CompactPose.IsValidIndex(BoneOrSocketCompactBoneIndex) || CompactPose.ContainsNaN())
		{
			return false;
		}
		
		ComponentSpacePose.InitPose(CompactPose);
		
		const FTransform BoneToComponent = ComponentSpacePose.GetComponentSpaceTransform(BoneOrSocketCompactBoneIndex);
		// SocketCS = SocketLocal * BoneCS
		const FTransform SocketTransformComponentSpace = ShapeLocalToBone * BoneToComponent;
		if (SocketTransformComponentSpace.ContainsNaN())
		{
			return false;
		}

		FDamageStateData& DamageStateData = OutDamageStateData.Add_GetRef(FDamageStateData{});
		DamageStateData.FrameTimeInMontage = FrameTime;
		DamageStateData.bTriggered = false;
		DamageStateData.MeshComponentRelativeLocation = SocketTransformComponentSpace.GetLocation();
		DamageStateData.MeshComponentRelativeRotation = SocketTransformComponentSpace.GetRotation().GetNormalized();
		return true;
	};

	for (int32 SampleIndex = 0 ; ; ++SampleIndex)
	{
		const double FrameTime = StartTimeInMontage + static_cast<double>(SampleIndex) * SampleInterval;
		if (FrameTime >= EndTimeInMontage - Tolerance)
		{
			break;
		}

		if (!ExtractPoseInAnim(FrameTime))
		{
			OutDamageStateData.Reset();
			return false;
		}
	}

	if (!ExtractPoseInAnim(EndTimeInMontage))
	{
		OutDamageStateData.Reset();
	}

	return !OutDamageStateData.IsEmpty();
}

void UHitDetectionComponent::GetCapsuleAxisEndPoints(const FTransform& Transform, const FCollisionShape& Capsule,
	FVector& OutA, FVector& OutB) const
{
	const double AxisHalfLength = Capsule.GetCapsuleAxisHalfLength();
	const FVector Axis = Transform.GetRotation().GetAxisZ();
	const FVector Offset = Axis * AxisHalfLength;
	OutA = Transform.GetLocation() - Offset;
	OutB = Transform.GetLocation() + Offset;
}

void UHitDetectionComponent::ToggleSweepTrajectoryVisibility()
{
	bShowSweepTrajectory = !bShowSweepTrajectory;

	DrawCompleteTrajectoryView();
}

void UHitDetectionComponent::ToggleReferenceTrajectoryVisibility()
{
	bShowReferenceTrajectory = !bShowReferenceTrajectory;
	DrawCompleteTrajectoryView();
}

void UHitDetectionComponent::ToggleCompensatedSweepTrajectoryVisibility()
{
	bShowCompensatedSweepTrajectory = !bShowCompensatedSweepTrajectory;
	DrawCompleteTrajectoryView();
}

void UHitDetectionComponent::ToggleUnCompensatedSweepTrajectoryVisibility()
{
	bShowUnCompensatedSweepTrajectory = !bShowUnCompensatedSweepTrajectory;
	DrawCompleteTrajectoryView();
}

FTransform UHitDetectionComponent::InterpTransform(const FTransform& From, const FTransform& To, const double Alpha) const
{
	return FTransform(
		FQuat::Slerp(From.GetRotation(), To.GetRotation(), Alpha),
			FMath::Lerp(From.GetLocation(), To.GetLocation(), Alpha),
			FMath::Lerp(From.GetScale3D(), To.GetScale3D(), Alpha));
}

double UHitDetectionComponent::GetCurrentMontageTime() const
{
	if (!OwnerMesh)
	{
		return -1.f;
	}
	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	const FAnimMontageInstance* MontageInstance = AnimInstance ? AnimInstance->GetMontageInstanceForID(ActiveMontageInstanceID) : nullptr;
	if (!AnimInstance || !MontageInstance)
	{
		return -1.f;
	}
	
	return MontageInstance->GetPosition();
}

FTransform UHitDetectionComponent::ReconstructMeshToWorldAtSampleTime(const double SampleTime,
	const double PreviousMontageTime, const double CurrentMontageTime, const FTransform& PreviousMeshToWorld,
	const FTransform& CurrentMeshToWorld) const
{
	const double Interval = CurrentMontageTime - PreviousMontageTime;
	if (Interval < Tolerance)
	{
		return CurrentMeshToWorld;
	}

	const double Alpha = FMath::Clamp((SampleTime - PreviousMontageTime) / Interval, 0.f, 1.f);
	return InterpTransform(PreviousMeshToWorld, CurrentMeshToWorld, Alpha);
}

void UHitDetectionComponent::CaptureRealTimeReferenceCollisionShape(const FHitDetectionFrameData& CurrentFrame)
{
	if (!bEnableTrajectoryVisualization && !bEnableTrajectoryValidation)
	{
		return;
	}

	if (!CurrentFrame.IsValid() || !CollisionShapes.IsValidIndex(0) || !OwnerMesh)
	{
		return;
	}

	const auto& ShapeData = CollisionShapes[0];
	const FTransform ShapeWorldTransform = CalculateCollisionShapeWorldTransform(ShapeData, CurrentFrame.BoneToMesh, CurrentFrame.MeshToWorld);
	if (ShapeWorldTransform.ContainsNaN())
	{
		return;
	}

	if (!ReferenceShapeRecord.IsEmpty())
	{
		const double ReferenceInterval = CurrentFrame.MontageTime - ReferenceShapeRecord.Last().MontageTime;
		if (ReferenceInterval > Tolerance)
		{
			ValidationState.MaxReferenceInterval = FMath::Max(ValidationState.MaxReferenceInterval, ReferenceInterval);
		}
	}

	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	FAnimMontageInstance* MontageInstance = AnimInstance ? AnimInstance->GetMontageInstanceForID(ActiveMontageInstanceID) : nullptr;
	
	FHitDetectionReferencePose& Record = ReferenceShapeRecord.Add_GetRef(FHitDetectionReferencePose());
	Record.MontageTime = CurrentFrame.MontageTime;
	Record.ShapeWorldTransform = ShapeWorldTransform;
	Record.MontageWeight = MontageInstance ? MontageInstance->GetWeight() : -1.f;
}

void UHitDetectionComponent::CapturePreviousObservedFrame()
{
	if (!OwnerMesh || WeaponBoneName.IsNone() || PreviousObservedMontageInstanceID == INDEX_NONE)
	{
		PreviousObservedFrame.Reset();
		PreviousObservedMontageInstanceID = INDEX_NONE;
		return;
	}

	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	FAnimMontageInstance* MontageInstance = AnimInstance ? AnimInstance->GetMontageInstanceForID(PreviousObservedMontageInstanceID) : nullptr;

	if (!MontageInstance)
	{
		PreviousObservedFrame.Reset();
		PreviousObservedMontageInstanceID = INDEX_NONE;
		return;
	}

	FTransform BoneToMesh = OwnerMesh->GetBoneTransform(WeaponBoneName, RTS_Component);
	BoneToMesh.SetScale3D(FVector::OneVector);

	PreviousObservedFrame.MontageTime = MontageInstance->GetPosition();
	PreviousObservedFrame.BoneToMesh = BoneToMesh;
	PreviousObservedFrame.MeshToWorld = OwnerMesh->GetComponentTransform();
}

void UHitDetectionComponent::DrawCompleteTrajectoryView()
{
	ULineBatchComponent* LineBatcher = GetTrajectoryLineBatcher();

	if (!LineBatcher)
	{
		return;
	}

	// 避免同一组轨迹被重复绘制。
	LineBatcher->ClearBatch(SweepTrajectoryBatchID);
	LineBatcher->ClearBatch(ReferenceTrajectoryBatchID);
	LineBatcher->ClearBatch(UnCompensatedSweepTrajectoryBatchID);
	LineBatcher->ClearBatch(CompensatedSweepTrajectoryBatchID);

	if (!bEnableTrajectoryVisualization)
	{
		return;
	}

	
	if (bShowReferenceTrajectory)
	{
		DrawReferenceTrajectory(*LineBatcher);
	}
	
	if (bShowSweepTrajectory)
	{
		DrawSweepTrajectory(*LineBatcher, SweepDebugRecords, SweepTrajectoryBatchID, FLinearColor::Green, FLinearColor::Yellow);
	}

	if (bShowUnCompensatedSweepTrajectory)
	{
		DrawSweepTrajectory(*LineBatcher, ValidationState.UnCompensatedSweepDebugRecords, UnCompensatedSweepTrajectoryBatchID, FLinearColor::Red, FLinearColor::Red);
	}

	if (bShowCompensatedSweepTrajectory)
	{
		DrawSweepTrajectory(*LineBatcher, ValidationState.CompensatedSweepDebugRecords, CompensatedSweepTrajectoryBatchID, FLinearColor::Blue, FLinearColor(0.f, 1.f, 1.f));
	}
}

ULineBatchComponent* UHitDetectionComponent::GetTrajectoryLineBatcher() const
{
	UWorld* World = GetWorld();
	return World ? World->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent) : nullptr;
}

void UHitDetectionComponent::DrawSweepTrajectory(ULineBatchComponent& LineBatcher, const TArray<FHitDetectionSweepDebugRecord>& Records, uint32 BatchID,
	const FLinearColor& RealTimeColor, const FLinearColor& ReconstructedColor) const
{
	if (!CollisionShapes.IsValidIndex(0) || Records.IsEmpty())
    {
        return;
    }

    constexpr float LifeTime = -1.0f;
    constexpr uint8 DepthPriority = 0;
    constexpr float PathThickness = 2.0f;
    constexpr float CapsuleThickness = 0.75f;


    const FCollisionShape& Capsule = CollisionShapes[0].CollisionShape;
	const int32 DesiredDrawCount = FMath::Max(1, DrawCount);
	const int32 CapsuleStride = FMath::Max(1, FMath::DivideAndRoundUp(Records.Num(), DesiredDrawCount));

    for (int32 Index = 0; Index < Records.Num(); ++Index)
    {
    	const auto& Record = Records[Index];
        if (Record.StartLocation.ContainsNaN() || Record.EndLocation.ContainsNaN() || Record.SweepRotation.ContainsNaN())
        {
            continue;
        }

        const FQuat SweepRotation = Record.SweepRotation.GetNormalized();
    	const FLinearColor SweepColor = Record.EndPoseSource == EHitDetectionPoseSource::ReconstructedSample ? ReconstructedColor : RealTimeColor;

        if (Record.bInitialOverlap)
        {
            LineBatcher.DrawCapsule(
                Record.EndLocation,
                Capsule.GetCapsuleHalfHeight(),
                Capsule.GetCapsuleRadius(),
                SweepRotation,
                FLinearColor(1.0f, 0.0f, 1.0f),
                LifeTime,
                DepthPriority,
                CapsuleThickness,
                BatchID);

            continue;
        }

        LineBatcher.DrawLine(
            Record.StartLocation,
            Record.EndLocation,
            SweepColor,
            DepthPriority,
            PathThickness,
            LifeTime,
            BatchID
        );

    	
    	const bool bDrawCapsule = Index == 0 || Index == Records.Num() - 1 || Index % CapsuleStride == 0;
    	if (!bDrawCapsule)
    	{
    		continue;
    	}

		if (Index == 0)
		{
			LineBatcher.DrawCapsule(
				Record.StartLocation,
				Capsule.GetCapsuleHalfHeight(),
				Capsule.GetCapsuleRadius(),
				SweepRotation,
				SweepColor,
				LifeTime,
				DepthPriority,
				CapsuleThickness,
				BatchID
			);
		}
    	
    	LineBatcher.DrawCapsule(
				Record.EndLocation,
				Capsule.GetCapsuleHalfHeight(),
				Capsule.GetCapsuleRadius(),
				SweepRotation,
				SweepColor,
				LifeTime,
				DepthPriority,
				CapsuleThickness,
				BatchID
		);
    }
}

void UHitDetectionComponent::DrawReferenceTrajectory(ULineBatchComponent& LineBatcher) const
{
	if (!CollisionShapes.IsValidIndex(0) || ReferenceShapeRecord.IsEmpty())
    {
        return;
    }

    constexpr float LifeTime = -1.0f;
    constexpr uint8 DepthPriority = 0;
    constexpr float PathThickness = 1.5f;
    constexpr float CapsuleThickness = 0.75f;

    const FCollisionShape& Capsule = CollisionShapes[0].CollisionShape;
    const int32 DesiredCapsuleCount = FMath::Max(1, DrawCount);

    const int32 CapsuleStride = FMath::Max(1,FMath::DivideAndRoundUp(ReferenceShapeRecord.Num(),DesiredCapsuleCount));

    bool bHasPreviousLocation = false;
    FVector PreviousLocation = FVector::ZeroVector;

    for (int32 Index = 0; Index < ReferenceShapeRecord.Num(); ++Index)
    {
        const FHitDetectionReferencePose& Record = ReferenceShapeRecord[Index];
        const FTransform& ShapeTransform = Record.ShapeWorldTransform;

        if (ShapeTransform.ContainsNaN())
        {
            continue;
        }

        const FVector CurrentLocation = ShapeTransform.GetLocation();

        if (bHasPreviousLocation)
        {
            LineBatcher.DrawLine(
                PreviousLocation,
                CurrentLocation,
                FLinearColor::White,
                DepthPriority,
                PathThickness,
                LifeTime,
                ReferenceTrajectoryBatchID
            );
        }

        const bool bDrawCapsule =
            Index == 0 ||
            Index == ReferenceShapeRecord.Num() - 1 ||
            Index % CapsuleStride == 0;

        if (bDrawCapsule)
        {
            LineBatcher.DrawCapsule(
                CurrentLocation,
                Capsule.GetCapsuleHalfHeight(),
                Capsule.GetCapsuleRadius(),
                ShapeTransform.GetRotation().GetNormalized(),
                FLinearColor::White,
                LifeTime,
                DepthPriority,
                CapsuleThickness,
                ReferenceTrajectoryBatchID
            );
        }

        PreviousLocation = CurrentLocation;
        bHasPreviousLocation = true;
    }
}

bool UHitDetectionComponent::ExtractMontageBoneComponentSpaceTransforms(double PreviousMontageTime, double CurrentMontageTime,
	FTransform& OutPreviousMontageBoneComponentSpace, FTransform& OutCurrentMontageBoneComponentSpace) const
{
	OutPreviousMontageBoneComponentSpace = FTransform::Identity;
	OutCurrentMontageBoneComponentSpace = FTransform::Identity;

	if (!OwnerMesh || WeaponBoneName.IsNone() || ActiveMontageSlotIndex == INDEX_NONE || ActiveMontageInstanceID == INDEX_NONE)
	{
		return false;
	}

	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	USkeletalMesh* SkeletalMesh = OwnerMesh->GetSkeletalMeshAsset();
	if (!SkeletalMesh || !AnimInstance)
	{
		return false;
	}

	const FAnimMontageInstance* MontageInstance = AnimInstance->GetMontageInstanceForID(ActiveMontageInstanceID);
	const UAnimMontage* Montage = MontageInstance ? MontageInstance->Montage : nullptr;
	if (!Montage || !Montage->SlotAnimTracks.IsValidIndex(ActiveMontageSlotIndex))
	{
		return false;
	}

	const FAnimTrack& AnimTrack = Montage->SlotAnimTracks[ActiveMontageSlotIndex].AnimTrack;
	if (AnimTrack.AnimSegments.IsEmpty() || AnimTrack.IsAdditive())
	{
		return false;
	}

	const int32 MeshBoneIndex = OwnerMesh->GetBoneIndex(WeaponBoneName);
	if (MeshBoneIndex == INDEX_NONE)
	{
		return false;
	}

	const FReferenceSkeleton& ReferenceSkeleton = SkeletalMesh->GetRefSkeleton();
	TArray<FBoneIndexType> RequiredBoneIndices;
	RequiredBoneIndices.SetNumUninitialized(ReferenceSkeleton.GetNum());

	for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
	{
		RequiredBoneIndices[BoneIndex] = static_cast<FBoneIndexType>(BoneIndex);
	}

	FBoneContainer BoneContainer;
	BoneContainer.InitializeTo(RequiredBoneIndices, UE::Anim::FCurveFilterSettings{}, *SkeletalMesh);
	BoneContainer.SetUseRAWData(false);
	BoneContainer.SetUseSourceData(false);	
	
	const FCompactPoseBoneIndex CompactPoseBoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshBoneIndex));
	if (!CompactPoseBoneIndex.IsValid())
	{
		return false;
	}
	
	FCompactPose CompactPose;
	CompactPose.SetBoneContainer(&BoneContainer);
	FBlendedCurve BlendedCurve;
	BlendedCurve.InitFrom(BoneContainer);
	UE::Anim::FStackAttributeContainer AttributeContainer;

	FCSPose<FCompactPose> ComponentSpacePose;
	
	auto ExtractBoneTransforms = [&] (const double MontageTime, FTransform& OutBoneComponentTransform)-> bool
	{
		CompactPose.ResetToRefPose();
		BlendedCurve.InitFrom(BoneContainer);
		AttributeContainer.Empty();

		FAnimationPoseData AnimationPoseData{CompactPose, BlendedCurve, AttributeContainer};
		FAnimExtractContext Context{MontageTime, true};
		AnimTrack.GetAnimationPose(AnimationPoseData, Context);

		if (!CompactPose.IsValid() || !CompactPose.IsValidIndex(CompactPoseBoneIndex) || CompactPose.ContainsNaN())
		{
			return false;
		}

		ComponentSpacePose.InitPose(CompactPose);
		const FTransform BoneComponentSpace = ComponentSpacePose.GetComponentSpaceTransform(CompactPoseBoneIndex); 
		if (BoneComponentSpace.ContainsNaN())
		{
			return false;
		}

		OutBoneComponentTransform = FTransform(BoneComponentSpace.GetRotation().GetNormalized(), BoneComponentSpace.GetLocation(), FVector::OneVector);
		return true;
	};

	return ExtractBoneTransforms(PreviousMontageTime, OutPreviousMontageBoneComponentSpace)
		&& ExtractBoneTransforms(CurrentMontageTime, OutCurrentMontageBoneComponentSpace);
}

FTransform UHitDetectionComponent::ApplyInterpolatedBonePoseCorrection(
	const FTransform& PreviousMontageBoneComponentSpace, const FTransform& CurrentMontageBoneComponentSpace,
	const FTransform& PreviousFinalBoneComponentSpace, const FTransform& CurrentFinalBoneComponentSpace,
	const FTransform& SampleMontageBoneComponentSpace, double Alpha) const
{
	const double ClampedAlpha = FMath::Clamp(Alpha, 0.f, 1.f);

	const FVector PreviousLocationCorrection = PreviousFinalBoneComponentSpace.GetLocation() - PreviousMontageBoneComponentSpace.GetLocation();
	const FVector CurrentLocationCorrection = CurrentFinalBoneComponentSpace.GetLocation() - CurrentMontageBoneComponentSpace.GetLocation();
	const FVector InterpLocationCorrection = FMath::Lerp(PreviousLocationCorrection, CurrentLocationCorrection, ClampedAlpha);

	const FQuat PreviousRotationCorrection = PreviousFinalBoneComponentSpace.GetRotation().GetNormalized()
						* PreviousMontageBoneComponentSpace.GetRotation().Inverse().GetNormalized();
	const FQuat CurrentRotationCorrection = CurrentFinalBoneComponentSpace.GetRotation().GetNormalized()
						* CurrentMontageBoneComponentSpace.GetRotation().Inverse().GetNormalized();
	const FQuat InterpRotationCorrection = FQuat::Slerp(PreviousRotationCorrection, CurrentRotationCorrection, ClampedAlpha).GetNormalized();

	const FVector CorrectedLocation = SampleMontageBoneComponentSpace.GetLocation() + InterpLocationCorrection;
	const FQuat CorrectedRotation = (InterpRotationCorrection * SampleMontageBoneComponentSpace.GetRotation().GetNormalized()).GetNormalized();

	return FTransform(CorrectedRotation, CorrectedLocation, FVector::OneVector);
}

bool UHitDetectionComponent::BuildCurrentDetectionFrame(FHitDetectionFrameData& OutFrame) const
{
	OutFrame.Reset();
	if (!OwnerMesh || WeaponBoneName.IsNone() || OwnerMesh->GetBoneIndex(WeaponBoneName) == INDEX_NONE)
	{
		return false;
	}
	
	const double CurrentMontageTime = GetCurrentMontageTime();
	if (CurrentMontageTime < 0.f)
	{
		return false;
	}

	FTransform CurrentBoneToMesh = OwnerMesh->GetBoneTransform(WeaponBoneName, RTS_Component);
	CurrentBoneToMesh.SetScale3D(FVector::OneVector);

	const FTransform CurrentMeshToWorld = OwnerMesh->GetComponentTransform(); 
	
	OutFrame.MontageTime = CurrentMontageTime;
	OutFrame.BoneToMesh = CurrentBoneToMesh;
	OutFrame.MeshToWorld = CurrentMeshToWorld;

	return true;
}

bool UHitDetectionComponent::ProcessRealTimeTickPose(const FHitDetectionFrameData& CurrentFrame,
	TArray<FHitResult>& OutHitResult, const FCollisionQueryParams& QueryParams)
{
	for (auto& ShapeData : CollisionShapes)
	{
		// cache ShapeData.LastLocation before ProcessWeaponCollisionData
		const FTransform ShapeWorldTransform = CalculateCollisionShapeWorldTransform(ShapeData, CurrentFrame.BoneToMesh, CurrentFrame.MeshToWorld);
		const bool bInitialOverlap = !ShapeData.bValidLastLocation;
		const FVector PreviousShapeLocation = ShapeData.LastLocation;
		
		if (!ProcessWeaponCollisionData(OutHitResult, ShapeData, ShapeWorldTransform, QueryParams))
		{
			return false;
		}

		RecordExecutedCollisionQuery(CurrentFrame.MontageTime, ShapeWorldTransform, PreviousShapeLocation, bInitialOverlap, EHitDetectionPoseSource::RealTimeTick);
	}

	return true;
}

bool UHitDetectionComponent::ProcessMissedSamples(const FHitDetectionFrameData& PreviousFrame, const FHitDetectionFrameData& CurrentFrame,
	TArray<FHitResult>& OutHitResult, const FCollisionQueryParams& QueryParams)
{
	FHitDetectionReconstructionContext Context;
	BuildReconstructionContext(PreviousFrame, CurrentFrame, Context);
	
	for (const auto& StateData : WeaponSocketDamageStateData)
	{
		const double SampleTime = StateData.FrameTimeInMontage;

		const bool bHasExecutedCollisionQuery = CollisionShapes.IsValidIndex(0) && CollisionShapes[0].bValidLastLocation;
		
		if (SampleTime < PreviousFrame.MontageTime - Tolerance || (bHasExecutedCollisionQuery && SampleTime <= PreviousFrame.MontageTime + Tolerance))
		{
			continue;
		}

		if (SampleTime >= CurrentFrame.MontageTime - Tolerance)
		{
			break;
		}
		
		for (FDetectionCollisionShapeData& ShapeData : CollisionShapes)
		{
			FTransform ShapeWorldTransform;
			if (!BuildReconstructedCollisionShapeWorldTransform(StateData, ShapeData, Context, ShapeWorldTransform))
			{
				return false;
			}

			// cache before process collision data
			const FVector PreviousShapeLocation = ShapeData.LastLocation;
			const bool bInitialOverlap = !ShapeData.bValidLastLocation;
			
			if (!ProcessWeaponCollisionData(OutHitResult, ShapeData, ShapeWorldTransform, QueryParams))
			{
				return false;
			}
			
			RecordExecutedCollisionQuery(SampleTime, ShapeWorldTransform, PreviousShapeLocation, bInitialOverlap, EHitDetectionPoseSource::ReconstructedSample);
		}
	}
	
	return true;
}

void UHitDetectionComponent::RecordExecutedCollisionQuery(double MontageTime, const FTransform& ShapeWorldTransform,
	const FVector& PreviousShapeLocation, const bool bInitialOverlap, const EHitDetectionPoseSource EndPoseSource)
{
	if (!bEnableTrajectoryVisualization)
	{
		return;
	}

	if (!FMath::IsFinite(MontageTime) || ShapeWorldTransform.ContainsNaN())
	{
		return;
	}
	
	FHitDetectionSweepDebugRecord& Record = SweepDebugRecords.Add_GetRef(FHitDetectionSweepDebugRecord());
	Record.bInitialOverlap = bInitialOverlap;
	Record.EndTimeInMontage = MontageTime;
	Record.EndLocation = ShapeWorldTransform.GetLocation();
	Record.StartLocation = bInitialOverlap ? ShapeWorldTransform.GetLocation() : PreviousShapeLocation;
	Record.SweepRotation = ShapeWorldTransform.GetRotation().GetNormalized();
	Record.EndPoseSource = EndPoseSource;

	if (bInitialOverlap || SweepDebugRecords.Num() == 1)
	{
		Record.StartTimeInMontage = MontageTime;
	} else
	{
		Record.StartTimeInMontage = SweepDebugRecords[SweepDebugRecords.Num() - 2].EndTimeInMontage;
	}
}

void UHitDetectionComponent::UpdateTrajectoryValidation(const FHitDetectionFrameData& PreviousFrame, const FHitDetectionFrameData& CurrentFrame, const float DeltaTime)
{
	if (!CurrentFrame.IsValid() || WeaponSocketDamageStateData.IsEmpty())
	{
		return;
	}
	
	// first time
	if (!ValidationState.PreviousSimulatedDetectionFrame.IsValid())
	{
		if (!PreviousFrame.IsValid())
		{
			ValidationState.PreviousSimulatedDetectionFrame = CurrentFrame;
			return;
		}

		// if PreviousFrame ahead of NotifyBegin, interp NotifyBegin Frame
		const double NotifyBeginTime = WeaponSocketDamageStateData[0].FrameTimeInMontage;
		FHitDetectionFrameData InitialFrame = PreviousFrame;
		if (PreviousFrame.MontageTime < NotifyBeginTime - Tolerance)
		{
			const double Interval = CurrentFrame.MontageTime - PreviousFrame.MontageTime;
			if (Interval < Tolerance)
			{
				return;
			}

			const double Alpha = FMath::Clamp((NotifyBeginTime - PreviousFrame.MontageTime) / Interval, 0.f, 1.f);
			InitialFrame.MontageTime = NotifyBeginTime;
			InitialFrame.BoneToMesh = InterpTransform(PreviousFrame.BoneToMesh, CurrentFrame.BoneToMesh, Alpha);
			InitialFrame.MeshToWorld = InterpTransform(PreviousFrame.MeshToWorld, CurrentFrame.MeshToWorld, Alpha);
		}
		
		ValidationState.PreviousSimulatedDetectionFrame = InitialFrame;
	}

	ValidationState.DetectionAccumulator += DeltaTime;
	const double ValidationInterval = 1.f / static_cast<double>(ValidationDetectionFrameRate);

	// has't reached detection interval
	if (ValidationState.DetectionAccumulator < ValidationInterval && !bPendingDisable)
	{
		return;
	}

	// reached detection interval
	if (!ProcessValidationDetectionInterval(ValidationState.PreviousSimulatedDetectionFrame, CurrentFrame))
	{
		return;
	}

	// update ValidationState
	ValidationState.PreviousSimulatedDetectionFrame = CurrentFrame;
	ValidationState.DetectionAccumulator = FMath::Max(0.f, ValidationState.DetectionAccumulator - ValidationInterval);
}

bool UHitDetectionComponent::FindReferenceCollisionShapeAtMontageTime(const double MontageTime, FTransform& OutReferenceShapeWorldTransform, float* OutMontageWeight) const
{
	if (OutMontageWeight)
	{
		*OutMontageWeight = -1.f;
	}
	
	for (int32 Index = 0; Index < ReferenceShapeRecord.Num(); ++Index)
	{
		const auto& Record = ReferenceShapeRecord[Index];
		if (FMath::IsNearlyEqual(Record.MontageTime, MontageTime))
		{
			OutReferenceShapeWorldTransform = Record.ShapeWorldTransform;
			if (OutMontageWeight)
			{
				*OutMontageWeight = Record.MontageWeight;
			}
			return true;
		}
		
		if (Index > 0)
		{
			const auto& PreviousRecord = ReferenceShapeRecord[Index - 1];
			const auto& NextRecord = ReferenceShapeRecord[Index];
			const double Interval = NextRecord.MontageTime - PreviousRecord.MontageTime;
			if (Interval <= Tolerance)
			{
				continue;
			}
			
			if (MontageTime >= PreviousRecord.MontageTime && MontageTime <= NextRecord.MontageTime)
			{
				const double Alpha = FMath::Clamp((MontageTime - PreviousRecord.MontageTime) / Interval, 0.f, 1.f);
				OutReferenceShapeWorldTransform = InterpTransform(PreviousRecord.ShapeWorldTransform, NextRecord.ShapeWorldTransform, Alpha);

				if (OutMontageWeight && PreviousRecord.MontageWeight >= 0.f && NextRecord.MontageWeight >= 0.f)
				{
					*OutMontageWeight = FMath::Lerp(PreviousRecord.MontageWeight, NextRecord.MontageWeight, Alpha);
				}
				
				return true;
			}
		}
	}
	
	return false;
}

void UHitDetectionComponent::BuildReconstructionContext(const FHitDetectionFrameData& PreviousFrame, const FHitDetectionFrameData& CurrentFrame,
															FHitDetectionReconstructionContext& OutContext) const
{
	OutContext.PreviousFrame = PreviousFrame;
	OutContext.CurrentFrame = CurrentFrame;
	OutContext.bHasPoseCorrection = ExtractMontageBoneComponentSpaceTransforms(PreviousFrame.MontageTime, CurrentFrame.MontageTime, 
	OutContext.PreviousMontageBoneToMesh, OutContext.CurrentMontageBoneToMesh);
}

bool UHitDetectionComponent::BuildReconstructedCollisionShapeWorldTransform(const FDamageStateData& StateData, const FDetectionCollisionShapeData& ShapeData,
	const FHitDetectionReconstructionContext& Context, FTransform& OutShapeWorldTransform) const
{
	const FTransform SampleMontageMeshComponentTransform = FTransform(StateData.MeshComponentRelativeRotation, StateData.MeshComponentRelativeLocation, FVector::OneVector);
	FTransform ReconSampleBoneToMesh = SampleMontageMeshComponentTransform;
	const double SampleTime = StateData.FrameTimeInMontage;
	
	if (Context.bHasPoseCorrection)
	{
		const double DetectionInterval = Context.CurrentFrame.MontageTime - Context.PreviousFrame.MontageTime;
		if (DetectionInterval <= Tolerance)
		{
			return false;
		}
		
		const double Alpha = (SampleTime - Context.PreviousFrame.MontageTime) / static_cast<double>(DetectionInterval);
		ReconSampleBoneToMesh = ApplyInterpolatedBonePoseCorrection(Context.PreviousMontageBoneToMesh, Context.CurrentMontageBoneToMesh,
				Context.PreviousFrame.BoneToMesh, Context.CurrentFrame.BoneToMesh, SampleMontageMeshComponentTransform, Alpha);
	}

	const FTransform ReconSampleMeshToWorld =
		ReconstructMeshToWorldAtSampleTime(SampleTime, Context.PreviousFrame.MontageTime, Context.CurrentFrame.MontageTime,
					Context.PreviousFrame.MeshToWorld, Context.CurrentFrame.MeshToWorld);

	OutShapeWorldTransform = CalculateCollisionShapeWorldTransform(ShapeData, ReconSampleBoneToMesh, ReconSampleMeshToWorld);

	return !OutShapeWorldTransform.ContainsNaN();
}

bool UHitDetectionComponent::ProcessValidationDetectionInterval(const FHitDetectionFrameData& PreviousFrame, const FHitDetectionFrameData& CurrentFrame)
{
	for (const auto& ShapeData : CollisionShapes)
	{
		const FTransform PreviousShapeWorldTransform = CalculateCollisionShapeWorldTransform(ShapeData, PreviousFrame.BoneToMesh, PreviousFrame.MeshToWorld);
		const FTransform CurrentShapeWorldTransform = CalculateCollisionShapeWorldTransform(ShapeData, CurrentFrame.BoneToMesh, CurrentFrame.MeshToWorld);

		// 不补帧时低帧率检测
		FHitDetectionSweepDebugRecord& Record = ValidationState.UnCompensatedSweepDebugRecords.Add_GetRef(FHitDetectionSweepDebugRecord());
		Record.bInitialOverlap = false;
		Record.StartTimeInMontage = PreviousFrame.MontageTime;
		Record.EndTimeInMontage = CurrentFrame.MontageTime;
		Record.StartLocation = PreviousShapeWorldTransform.GetLocation();
		Record.EndLocation = CurrentShapeWorldTransform.GetLocation();
		Record.SweepRotation = CurrentShapeWorldTransform.GetRotation().GetNormalized();
		Record.EndPoseSource = EHitDetectionPoseSource::RealTimeTick;

		// 补帧Sweep
		FHitDetectionReconstructionContext Context;
		BuildReconstructionContext(PreviousFrame, CurrentFrame, Context);

		double PreviousSweepTime = PreviousFrame.MontageTime;
		FVector PreviousSweepLocation = PreviousShapeWorldTransform.GetLocation();
		
		for (const auto& StateData : WeaponSocketDamageStateData)
		{
			const double SampleTime = StateData.FrameTimeInMontage;
			if (SampleTime > PreviousSweepTime + Tolerance && SampleTime < CurrentFrame.MontageTime - Tolerance)
			{
				FTransform ReconShapeWorldTransform = FTransform::Identity;
				if (!BuildReconstructedCollisionShapeWorldTransform(StateData, ShapeData, Context, ReconShapeWorldTransform))
				{
					return false;
				}

				// Find ReferenceShapeWorldTransform
				FTransform ReferenceShapeWorldTransform = FTransform::Identity;
				float ReferenceMontageWeight = -1.f;
				if (FindReferenceCollisionShapeAtMontageTime(SampleTime, ReferenceShapeWorldTransform, &ReferenceMontageWeight))
				{
					double CenterError = 0.f;
					double EndPointError = 0.f;
					CalculateCollisionShapeError(ReconShapeWorldTransform, ReferenceShapeWorldTransform, ShapeData.CollisionShape, CenterError, EndPointError);
					ValidationState.ReconSampleStatistics.AddSample(CenterError, EndPointError);

					UE_LOG(LogTemp, Display, TEXT(
					"[HitDetectionValidation][ReconSample] "
						"Time= %.4f "
						"Weight=%.3f "
						"CenterError=%.3f cm "
						"EndPointError=%.3f cm"),
						SampleTime,
						ReferenceMontageWeight,
						CenterError,
						EndPointError
					);
				}
				
				FHitDetectionSweepDebugRecord& ReconRecord = ValidationState.CompensatedSweepDebugRecords.Add_GetRef(FHitDetectionSweepDebugRecord());
				ReconRecord.StartTimeInMontage = PreviousSweepTime;
				ReconRecord.EndTimeInMontage = SampleTime;
				ReconRecord.StartLocation = PreviousSweepLocation;
				ReconRecord.EndLocation = ReconShapeWorldTransform.GetLocation();
				ReconRecord.SweepRotation = ReconShapeWorldTransform.GetRotation().GetNormalized();
				ReconRecord.EndPoseSource = EHitDetectionPoseSource::ReconstructedSample;

				// update previous time
				PreviousSweepTime = SampleTime;
				PreviousSweepLocation = ReconShapeWorldTransform.GetLocation();
			}
		}

		// 补充最后一个重建采样点到当前tick帧的 sweep
		FHitDetectionSweepDebugRecord& LastRecord = ValidationState.CompensatedSweepDebugRecords.Add_GetRef(FHitDetectionSweepDebugRecord());
		LastRecord.bInitialOverlap = false;
		LastRecord.StartTimeInMontage = PreviousSweepTime;
		LastRecord.EndTimeInMontage = CurrentFrame.MontageTime;
		LastRecord.StartLocation = PreviousSweepLocation;
		LastRecord.EndLocation = CurrentShapeWorldTransform.GetLocation();
		LastRecord.SweepRotation = CurrentShapeWorldTransform.GetRotation().GetNormalized();
		LastRecord.EndPoseSource = EHitDetectionPoseSource::RealTimeTick;
	}

	return true;
}

void UHitDetectionComponent::CalculateCollisionShapeError(const FTransform& ReconShapeWorldTransform, const FTransform& ReferenceShapeWorldTransform,
	const FCollisionShape& CollisionShape, double& OutCenterError, double& OutEndPointError) const
{
	OutCenterError = FVector::Distance(ReconShapeWorldTransform.GetLocation(), ReferenceShapeWorldTransform.GetLocation());

	FVector ReconEndPointA{FVector::ZeroVector};
	FVector ReconEndPointB{FVector::ZeroVector};
	GetCapsuleAxisEndPoints(ReconShapeWorldTransform, CollisionShape, ReconEndPointA, ReconEndPointB);

	FVector RefEndPointA{FVector::ZeroVector};
	FVector RefEndPointB{FVector::ZeroVector};
	GetCapsuleAxisEndPoints(ReferenceShapeWorldTransform, CollisionShape, RefEndPointA, RefEndPointB);

	const double DirectError = FMath::Max(FVector::Distance(ReconEndPointA, RefEndPointA), FVector::Distance(ReconEndPointB, RefEndPointB));
	const double SwappedError = FMath::Max(FVector::Distance(ReconEndPointA, RefEndPointB), FVector::Distance(ReconEndPointB, RefEndPointA));
	OutEndPointError = FMath::Min(DirectError, SwappedError);
}

void UHitDetectionComponent::ValidationSweepTrajectory(const FHitDetectionSweepDebugRecord& SweepRecord, const FCollisionShape& CollisionShape, FHitDetectionValidationStatistics& OutStatistics) const
{
	for (const auto& RefRecord : ReferenceShapeRecord)
	{
		if (RefRecord.MontageTime > SweepRecord.StartTimeInMontage + Tolerance && RefRecord.MontageTime <= SweepRecord.EndTimeInMontage + Tolerance)
		{
			FVector ClosestSweepLocation = FMath::ClosestPointOnSegment(RefRecord.ShapeWorldTransform.GetLocation(), SweepRecord.StartLocation, SweepRecord.EndLocation);
			const FTransform SweepShapeWorldTransform = FTransform(SweepRecord.SweepRotation, ClosestSweepLocation, FVector::OneVector);

			double CenterError = 0.f;
			double EndPointError = 0.f;
			CalculateCollisionShapeError(SweepShapeWorldTransform, RefRecord.ShapeWorldTransform,
				CollisionShape, CenterError, EndPointError);
			OutStatistics.AddSample(CenterError, EndPointError);
		}
	}
}

void UHitDetectionComponent::FinalizeTrajectoryValidation()
{
	for (auto& ShapeData : CollisionShapes)
	{
		for (auto& SweepRecord : ValidationState.UnCompensatedSweepDebugRecords)
		{
			ValidationSweepTrajectory(SweepRecord, ShapeData.CollisionShape, ValidationState.UnCompensatedSweepStatistics);
		}

		for (auto& SweepRecord : ValidationState.CompensatedSweepDebugRecords)
		{
			ValidationSweepTrajectory(SweepRecord, ShapeData.CollisionShape, ValidationState.CompensatedSweepStatistics);
		}
	}

	// Output Validation Error
	UE_LOG(LogTemp, Display,
	    TEXT(
	        " \n"
	        "══════════════════════════════════════════════════════════════\n"
	        "              [Validation] 统计报告							   \n"
	        "══════════════════════════════════════════════════════════════\n"
	        " 参考数据:                                                     \n"
	        "   ReferenceCount       : %6d								   \n"
	        "   MaxReferenceInterval : %10.6f s							   \n"
	        "══════════════════════════════════════════════════════════════\n"
	        " 重建采样:                                                      \n"
	        "   SampleCount          : %6d                                 \n"
	        "   AvgCenterError       : %10.3f cm                           \n"
	        "   MaxCenterError       : %10.3f cm                           \n"
	        "   AvgEndPointError     : %10.3f cm                           \n"
	        "   MaxEndPointError     : %10.3f cm                           \n"
	        "══════════════════════════════════════════════════════════════\n"
	        " 未补偿扫描:                                                    \n"
	        "   SweepCount           : %6d                                 \n"
	        "   ComparedSamples      : %6d                                 \n"
	        "   AvgCenterError       : %10.3f cm                           \n"
	        "   MaxCenterError       : %10.3f cm                           \n"
	        "   AvgEndPointError     : %10.3f cm                           \n"
	        "   MaxEndPointError     : %10.3f cm                           \n"
	        "══════════════════════════════════════════════════════════════\n"
	        " 已补偿扫描:                                                    \n"
	        "   SweepCount           : %6d                                 \n"
	        "   ComparedSamples      : %6d                                 \n"
	        "   AvgCenterError       : %10.3f cm                           \n"
	        "   MaxCenterError       : %10.3f cm                           \n"
	        "   AvgEndPointError     : %10.3f cm                           \n"
	        "   MaxEndPointError     : %10.3f cm                           \n"
	        "══════════════════════════════════════════════════════════════"
	    ),

	    ReferenceShapeRecord.Num(),
	    ValidationState.MaxReferenceInterval,

	    ValidationState.ReconSampleStatistics.SampleCount,
	    ValidationState.ReconSampleStatistics.GetAverageCenterError(),
	    ValidationState.ReconSampleStatistics.MaxCenterError,
	    ValidationState.ReconSampleStatistics.GetAverageEndPointError(),
	    ValidationState.ReconSampleStatistics.MaxEndPointError,

	    ValidationState.UnCompensatedSweepDebugRecords.Num(),
	    ValidationState.UnCompensatedSweepStatistics.SampleCount,
	    ValidationState.UnCompensatedSweepStatistics.GetAverageCenterError(),
	    ValidationState.UnCompensatedSweepStatistics.MaxCenterError,
	    ValidationState.UnCompensatedSweepStatistics.GetAverageEndPointError(),
	    ValidationState.UnCompensatedSweepStatistics.MaxEndPointError,

	    ValidationState.CompensatedSweepDebugRecords.Num(),
	    ValidationState.CompensatedSweepStatistics.SampleCount,
	    ValidationState.CompensatedSweepStatistics.GetAverageCenterError(),
	    ValidationState.CompensatedSweepStatistics.MaxCenterError,
	    ValidationState.CompensatedSweepStatistics.GetAverageEndPointError(),
	    ValidationState.CompensatedSweepStatistics.MaxEndPointError
	);
}

bool UHitDetectionComponent::PrepareHitDetection(int32 MontageInstanceID)
{
	if (!OwnerMesh || MontageInstanceID == INDEX_NONE)
	{
		return false;
	}

	if (PreviousObservedMontageInstanceID == MontageInstanceID && PreviousObservedFrame.IsValid())
	{
		return true;
	}
	
	UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance();
	FAnimMontageInstance* MontageInstance = AnimInstance ? AnimInstance->GetMontageInstanceForID(MontageInstanceID) : nullptr;
	if (!MontageInstance)
	{
		return false;
	}

	PreviousObservedMontageInstanceID = MontageInstance->GetInstanceID();

	CapturePreviousObservedFrame();

	return PreviousObservedFrame.IsValid();
}
