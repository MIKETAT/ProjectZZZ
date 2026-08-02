#include "FCombatActionStepEditorViewportClient.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Character/Combat/AttackDetectionGeometry.h"

FCombatActionStepEditorViewportClient::FCombatActionStepEditorViewportClient(FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget)
	: FEditorViewportClient(nullptr, InPreviewScene, InEditorViewportWidget)
{
	check(InPreviewScene != nullptr);
	
	SetViewLocation(FVector(-300.f, 0.f, 150.f));
	SetViewRotation(FRotator(-10.f, 0.f, 0.f));
	SetViewportType(ELevelViewportType::LVT_Perspective);
	SetRealtime(true);

	EngineShowFlags.SetMotionBlur(false);
	EngineShowFlags.SetEyeAdaptation(false);
}

void FCombatActionStepEditorViewportClient::Tick(float InDeltaTime)
{
	FEditorViewportClient::Tick(InDeltaTime);

	TickPreviewScene(InDeltaTime);

	UpdateAttackDetectionVisualizationCache();
}

void FCombatActionStepEditorViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);
	DrawVisualizationCache(PDI);
}

void FCombatActionStepEditorViewportClient::SetShapeQueryVisualizationRequest(const FAttackDetectionVisualizationRequest& InVisualizationRequest)
{
	VisualizationRequest = InVisualizationRequest;
	VisualizationFrame.Reset();
}

void FCombatActionStepEditorViewportClient::DrawVisualizationCache(FPrimitiveDrawInterface* PDI)
{
	if (!PDI)
	{
		return;
	}

	for (const FAttackShapeQueryGeometry& Geometry : VisualizationFrame.Shapes)
	{
		DrawShapeGeometry(PDI, Geometry);
	}

	for (const FAttackSweepGeometry& Geometry : VisualizationFrame.Sweeps)
	{
		DrawSweepGeometry(PDI, Geometry);
	}
}

bool FCombatActionStepEditorViewportClient::IsVisualizationActiveAtTime(const float CurrentTime) const
{
	if (!VisualizationRequest.bEnable)
	{
		return false;
	}

	switch (VisualizationRequest.Spec.DetectionMode)
	{
		case EAttackDetectionMode::ShapeQueryInstant:
			return FMath::Abs(CurrentTime - VisualizationRequest.StartTime) <= PreviewInstantHalfWindow;
		case EAttackDetectionMode::ActorPathSweep:
		case EAttackDetectionMode::WeaponSweep:
		case EAttackDetectionMode::ShapeQueryContinuous:
			return CurrentTime >= VisualizationRequest.StartTime && CurrentTime <= VisualizationRequest.EndTime;
		default:
			return false;
	}
}

void FCombatActionStepEditorViewportClient::DrawShapeGeometry(FPrimitiveDrawInterface* PDI, const FAttackShapeQueryGeometry& Geometry)
{
	if (!PDI)
	{
		return;
	}

	const FCollisionShape& Shape = Geometry.CollisionShape;
	const FTransform& Transform = Geometry.WorldTransform;

	DrawCollisionShapePDI(PDI, Transform.GetLocation(), Transform.GetRotation(), Shape);
}

void FCombatActionStepEditorViewportClient::DrawSweepGeometry(FPrimitiveDrawInterface* PDI, const FAttackSweepGeometry& Geometry)
{
	if (!PDI)
	{
		return;
	}
	
	const FCollisionShape& Shape = Geometry.CollisionShape;
	const FQuat& Rotation = Geometry.Rotation;
	FVector Start = Geometry.Start;
	FVector End = Geometry.End;

	if (Shape.IsBox())
	{
		DrawSweepBoxPDI(PDI, Start, End, Rotation, Shape.GetBox());
	}
	else if (Shape.IsSphere())
	{
		DrawSweepSpherePDI(PDI, Start, End, Shape.GetSphereRadius());
	}
	else if (Shape.IsCapsule())
	{
		DrawSweepCapsulePDI(PDI, Start, End, Rotation, Shape.GetCapsuleRadius(), Shape.GetCapsuleHalfHeight());
	}
}

float FCombatActionStepEditorViewportClient::GetPreviewCurrentTime() const
{
	USkeletalMeshComponent* Mesh = VisualizationRequest.PreviewMesh.Get();
	if (!Mesh)
	{
		return 0.f;
	}

	UAnimSingleNodeInstance* Instance = Mesh->GetSingleNodeInstance();
	return Instance ? Instance->GetCurrentTime() : 0.f;
}

void FCombatActionStepEditorViewportClient::TickPreviewScene(float InDeltaTime)
{
	if (!PreviewScene)
	{
		return;
	}

	UWorld* World = PreviewScene->GetWorld();
	if (World)
	{
		World->Tick(LEVELTICK_All, InDeltaTime);
	}
}

void FCombatActionStepEditorViewportClient::UpdateAttackDetectionVisualizationCache()
{
	VisualizationFrame.Reset();

	if (!VisualizationRequest.bEnable)
	{
		return;
	}

	USkeletalMeshComponent* PreviewMesh = VisualizationRequest.PreviewMesh.Get();
	if (!PreviewMesh)
	{
		return;
	}
	
	UAnimSingleNodeInstance* PreviewInstance = PreviewMesh->GetSingleNodeInstance();
	if (!PreviewInstance)
	{
		return;
	}

	const float CurrentTime = GetPreviewCurrentTime();
	if (!IsVisualizationActiveAtTime(CurrentTime))
	{
		return;
	}

	switch (VisualizationRequest.Spec.DetectionMode)
	{
		case EAttackDetectionMode::ShapeQueryInstant:
			UpdateShapeQueryCache();
			break;
		case EAttackDetectionMode::WeaponSweep:
			UpdateWeaponSweepCache();
			break;
		case EAttackDetectionMode::ActorPathSweep:
			UpdateActorPathSweepCache();
			break;
		case EAttackDetectionMode::ShapeQueryContinuous:
			
			break;
		
		default:
			break;
	}
}

void FCombatActionStepEditorViewportClient::UpdateShapeQueryCache()
{
	USkeletalMeshComponent* PreviewMesh = VisualizationRequest.PreviewMesh.Get();
	if (!PreviewMesh)
	{
		return;
	}

	const FAttackDetectionSpec& Spec = VisualizationRequest.Spec;
	FTransform ReferenceTransform;
	switch (Spec.ReferenceType)
	{
		case EAttackQueryReference::Owner:
			ReferenceTransform = PreviewMesh->GetComponentTransform();
			break;
		case EAttackQueryReference::OwnerSocket:
			{
				if (!PreviewMesh->DoesSocketExist(Spec.ReferenceSocketName))
				{
					return;
				}
				ReferenceTransform = PreviewMesh->GetSocketTransform(Spec.ReferenceSocketName, RTS_World);
			}
			break;
		default:
			return;
	}

	FAttackShapeQueryGeometry Geometry;
	if (!AttackDetectionGeometry::BuildShapeQueryGeometry(Spec, ReferenceTransform, Geometry))
	{
		return;
	}

	VisualizationFrame.Shapes.Add(MoveTemp(Geometry));
}

void FCombatActionStepEditorViewportClient::UpdateWeaponSweepCache()
{
	const FAttackDetectionSpec& Spec = VisualizationRequest.Spec;
	
	USkeletalMeshComponent* PreviewMesh = VisualizationRequest.PreviewMesh.Get();
	USkeletalMeshComponent* SamplingMesh = VisualizationRequest.SamplingMesh.Get();
	if (!PreviewMesh || !SamplingMesh
		|| !PreviewMesh->DoesSocketExist(Spec.WeaponRootSocketName)
		|| !PreviewMesh->DoesSocketExist(Spec.WeaponTipSocketName)
		|| !SamplingMesh->DoesSocketExist(Spec.WeaponRootSocketName)
		|| !SamplingMesh->DoesSocketExist(Spec.WeaponTipSocketName))
	{
		return;
	}

	UAnimSingleNodeInstance* PreviewInstance = PreviewMesh->GetSingleNodeInstance();
	UAnimSingleNodeInstance* SamplingInstance = SamplingMesh->GetSingleNodeInstance();
	if (!PreviewInstance || !SamplingInstance)
	{
		return;
	}

	const float CurrentTime = GetPreviewCurrentTime();
	const float PreviousTime = FMath::Max(CurrentTime - PreviewSampleInterval, VisualizationRequest.StartTime);
	
	SamplingInstance->SetPlaying(false);
	SamplingInstance->SetPosition(PreviousTime, false);
	SamplingMesh->SetWorldTransform(PreviewMesh->GetComponentTransform());
	SamplingMesh->TickAnimation(0.f, false);
	SamplingMesh->RefreshBoneTransforms(nullptr);
	
	FTransform PreviousRootTransform = SamplingMesh->GetSocketTransform(Spec.WeaponRootSocketName, RTS_World);
	FTransform PreviousTipTransform = SamplingMesh->GetSocketTransform(Spec.WeaponTipSocketName, RTS_World);
	FTransform CurrentRootTransform = PreviewMesh->GetSocketTransform(Spec.WeaponRootSocketName, RTS_World);
	FTransform CurrentTipTransform = PreviewMesh->GetSocketTransform(Spec.WeaponTipSocketName, RTS_World);

	TArray<FAttackSweepGeometry> Sweeps;
	if (!AttackDetectionGeometry::BuildWeaponSweepGeometry(Spec, CurrentTime - PreviousTime,
		PreviousRootTransform, PreviousTipTransform,
		CurrentRootTransform, CurrentTipTransform,
		Sweeps))
	{
		return;
	}
	
	VisualizationFrame.Sweeps.Append(Sweeps);
}

void FCombatActionStepEditorViewportClient::UpdateActorPathSweepCache()
{
	const FAttackDetectionSpec& Spec = VisualizationRequest.Spec;
	USkeletalMeshComponent* PreviewMesh = VisualizationRequest.PreviewMesh.Get();
	USkeletalMeshComponent* SamplingMesh = VisualizationRequest.SamplingMesh.Get();
	if (!PreviewMesh || !SamplingMesh)
	{
		return;
	}

	UAnimSingleNodeInstance* PreviewInstance = PreviewMesh->GetSingleNodeInstance();
	UAnimSingleNodeInstance* SamplingInstance = SamplingMesh->GetSingleNodeInstance();
	if (!PreviewInstance || !SamplingInstance)
	{
		return;
	}
	
	const float CurrentTime = GetPreviewCurrentTime();
	const float PreviousTime = FMath::Max(CurrentTime - PreviewSampleInterval, VisualizationRequest.StartTime);
	
	FTransform CurrentReferenceTransform = PreviewMesh->GetComponentTransform();
	FTransform PreviousReferenceTransform;
	if (!BuildPreviewActorTransformAtTime(CurrentTime, CurrentReferenceTransform,
		PreviousTime, PreviousReferenceTransform))
	{
		return;
	}

	// Lock on Begin, Calc Rotation at Start Time
	if (VisualizationRequest.Spec.PathSweepRotationPolicy == EActorPathSweepRotationPolicy::LockOnBegin)
	{
		FTransform BeginReferenceTransform;
		if (!BuildPreviewActorTransformAtTime(CurrentTime, CurrentReferenceTransform,
			VisualizationRequest.StartTime, BeginReferenceTransform))
		{
			return;
		}

		const FQuat LockedRotation = BeginReferenceTransform.GetRotation();
		PreviousReferenceTransform.SetRotation(LockedRotation);
		CurrentReferenceTransform.SetRotation(LockedRotation);
	}
	
	FAttackSweepGeometry Geometry;
	if (!AttackDetectionGeometry::BuildActorPathSweepGeometry(Spec, PreviousReferenceTransform, CurrentReferenceTransform, Geometry))
	{
		return;
	}
	
	VisualizationFrame.Sweeps.Add(MoveTemp(Geometry));
}

void FCombatActionStepEditorViewportClient::DrawCollisionShapePDI(FPrimitiveDrawInterface* PDI, const FVector& Location,
	const FQuat& Rotation, const FCollisionShape& Shape)
{
	if (!PDI)
	{
		return;
	}

	// Capsule
	if (Shape.IsCapsule())
	{
		DrawWireCapsule(
			PDI,
			Location,
			Rotation.GetAxisX(),
			Rotation.GetAxisY(),
			Rotation.GetAxisZ(),
			TraceColor,
			Shape.GetCapsuleRadius(),
			Shape.GetCapsuleHalfHeight(),
			16,	// NumSides
			DepthPriority,
			Thickness);
	}
	// Box
	else if (Shape.IsBox())
	{
		const FVector HalfExtent = Shape.GetBox();
		const FTransform Transform(Rotation, Location, FVector::OneVector);
		
		DrawWireBox(
			PDI,
			Transform.ToMatrixWithScale(),
			FBox(-HalfExtent, HalfExtent),
			TraceColor,
			DepthPriority,
			Thickness);
	}
	// Sphere
	else if (Shape.IsSphere())
	{
		DrawWireSphere(
			PDI,
			Location,
			TraceColor,
			Shape.GetSphereRadius(),
			16,
			DepthPriority,
			Thickness);
	}
}

void FCombatActionStepEditorViewportClient::DrawSweepBoxPDI(FPrimitiveDrawInterface* PDI, const FVector& Start,
	const FVector& End, const FQuat& Rotation, const FVector& HalfExtent)
{
	if (!PDI)
	{
		return;
	}

	FCollisionShape BoxShape = FCollisionShape::MakeBox(HalfExtent);
	DrawCollisionShapePDI(PDI, Start, Rotation, BoxShape);

	if (Start.Equals(End, UE_KINDA_SMALL_NUMBER))
	{
		return;
	}

	DrawCollisionShapePDI(PDI, End, Rotation, BoxShape);

	for (int32 SX = -1; SX <= 1; SX += 2)
	{
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			for (int32 SZ = -1; SZ <= 1; SZ += 2)
			{
				const FVector CenterToCorner = FVector(HalfExtent.X * SX, HalfExtent.Y * SY, HalfExtent.Z * SZ);
				const FVector CornerOffset = Rotation.RotateVector(CenterToCorner);

				PDI->DrawLine(
					Start + CornerOffset,
					End + CornerOffset,
					TraceColor,
					DepthPriority,
					Thickness
				);
			}
		}	
	}
}

void FCombatActionStepEditorViewportClient::DrawSweepSpherePDI(FPrimitiveDrawInterface* PDI, const FVector& Start,
	const FVector& End, const float Radius)
{
	if (!PDI)
	{
		return;
	}

	const FVector Delta = End - Start;
	const float Distance = Delta.Size();

	if (Distance < UE_KINDA_SMALL_NUMBER)
	{
		DrawWireSphere(PDI, Start, TraceColor, Radius, 16, DepthPriority, Thickness);
		return;
	}

	FVector AxisX;
	FVector AxisY;
	const FVector AxisZ = Delta / Distance;
	AxisZ.FindBestAxisVectors(AxisX, AxisY);

	const FVector Center = (Start + End) * 0.5f;
	const float SweepHalfHeight = Distance * 0.5f + Radius;

	DrawWireCapsule(
		PDI,
		Center,
		AxisX,
		AxisY,
		AxisZ,
		TraceColor,
		Radius,
		SweepHalfHeight,
		16,
		DepthPriority,
		Thickness
	);
}

void FCombatActionStepEditorViewportClient::DrawSweepCapsulePDI(FPrimitiveDrawInterface* PDI, const FVector& Start,
	const FVector& End, const FQuat& Rotation, const float Radius, const float HalfHeight)
{
	if (!PDI)
	{
		return;
	}

	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	DrawCollisionShapePDI(PDI, Start, Rotation, CapsuleShape);

	if (Start.Equals(End, UE_KINDA_SMALL_NUMBER))
	{
		return;
	}

	DrawCollisionShapePDI(PDI, End, Rotation, CapsuleShape);

	const FVector AxisX = Rotation.GetAxisX();
	const FVector AxisY = Rotation.GetAxisY();
	const FVector AxisZ = Rotation.GetAxisZ();

	const float CylinderHalfHeight = FMath::Max(0.f, HalfHeight - Radius);
	const FVector RadialDirections[] = { AxisX, -AxisX, AxisY, -AxisY };

	// 圆柱体的起始连线
	for (const FVector& RadialDirection : RadialDirections)
	{
		const FVector RadialOffset = RadialDirection * Radius;
		const FVector TopOffset = AxisZ * CylinderHalfHeight + RadialOffset;
		const FVector BottomOffset = -AxisZ * CylinderHalfHeight + RadialOffset;

		PDI->DrawLine(
			Start + TopOffset,
			End + TopOffset,
			TraceColor,
			DepthPriority,
			Thickness
		);

		PDI->DrawLine(
			Start + BottomOffset,
			End + BottomOffset,
			TraceColor,
			DepthPriority,
			Thickness
		);
	}

	// 半球的连线
	const FVector TopPoleOffset = AxisZ * HalfHeight;
	const FVector BottomPoleOffset = -AxisZ * HalfHeight;

	PDI->DrawLine(
		Start + TopPoleOffset,
		End + TopPoleOffset,
		TraceColor,
		DepthPriority,
		Thickness
	);

	PDI->DrawLine(
		Start + BottomPoleOffset,
		End + BottomPoleOffset,
		TraceColor,
		DepthPriority,
		Thickness
	);
}

bool FCombatActionStepEditorViewportClient::BuildPreviewActorTransformAtTime(
		const float AnchorTime, const FTransform& AnchorWorldTransform,
		const float SampleTime, FTransform& OutSampleWorldTransform) const
{
	OutSampleWorldTransform = FTransform::Identity;

	UAnimMontage* Montage = VisualizationRequest.Montage.Get();
	
	if (!Montage || SampleTime < 0.f || AnchorTime < SampleTime)
	{
		return false;
	}

	FAnimExtractContext Context;
	const FTransform RootMotionDelta = Montage->ExtractRootMotionFromTrackRange(SampleTime, AnchorTime, Context); 

	FTransform AnchorWorldTransformNoScale = AnchorWorldTransform;
	AnchorWorldTransformNoScale.SetScale3D(FVector::OneVector);

	OutSampleWorldTransform = RootMotionDelta.Inverse() * AnchorWorldTransformNoScale;
	OutSampleWorldTransform.SetScale3D(FVector::OneVector);
	
	return true;
}
