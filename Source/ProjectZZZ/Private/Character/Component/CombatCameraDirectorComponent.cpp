#include "Character/Component/CombatCameraDirectorComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/PlayerCharacter.h"

UCombatCameraDirectorComponent::UCombatCameraDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatCameraDirectorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCombatCameraDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatCameraDirectorComponent::UpdateCamera(const float DeltaTime)
{
	if (!ActiveCameraState.bActive)
	{
		return;
	}

	FTransform CameraTransform;
	if (CalculateCurrentCameraTransform(CameraTransform))
	{
		OnUpdateCameraTransform.Broadcast(CameraTransform);
	}
}

void UCombatCameraDirectorComponent::PrepareCameraContext(ECombatCameraMode CameraMode, APlayerCharacter* Agent, const FCombatCameraContext& Context)
{
	if (CameraMode == ECombatCameraMode::None || !Agent)
	{
		PreparedCameraContext.Reset();
		return;
	}

	PreparedCameraContext.bValid = true;
	PreparedCameraContext.CameraMode = CameraMode;
	PreparedCameraContext.Agent = Agent;
	PreparedCameraContext.CombatCameraContext = Context;
}

bool UCombatCameraDirectorComponent::ResolveCameraRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest)
{
	switch (Context.CameraMode)
	{
		case ECombatCameraMode::ParryAssist:
			return ResolveParryAssistRequest(Context, OutRequest);
		case ECombatCameraMode::FixedActionView:
			return ResolveFixedActionViewRequest(Context, OutRequest);
		case ECombatCameraMode::ActionFocusView:
			return ResolveActionFocusRequest(Context, OutRequest);
		default:
			return false;
	}
}

bool UCombatCameraDirectorComponent::ResolveFixedActionViewRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest)
{
	if (!Context.Agent.IsValid() || Context.CameraMode != ECombatCameraMode::FixedActionView)
	{
		return false;
	}

	FVector BasisForward{FVector::VectorPlaneProject(Context.AgentSectionTransform.GetRotation().GetForwardVector(), FVector::UpVector).GetSafeNormal()};
	if (BasisForward.IsNearlyZero())
	{
		return false;
	}

	OutRequest.Reset();
	OutRequest.Agent = Context.Agent;
	OutRequest.Config = Context.CameraConfig;
	OutRequest.AnchorLocation = Context.AgentSectionTransform.GetLocation();	// Use Agent Current Location as Anchor
	OutRequest.BasisForward = BasisForward;
	OutRequest.SideSign = 1.f;
	return true;
}

bool UCombatCameraDirectorComponent::ResolveParryAssistRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest)
{
	if (!Context.Agent.IsValid() || !Context.Enemy.IsValid())
	{
		return false;
	}
	
	const FVector AgentLocation{Context.AgentSectionTransform.GetLocation()};
	const FVector EnemyLocation{Context.Enemy->GetActorLocation()};
	// BasisForward: AgentLocation - EnemyLocation
	FVector BasisForward{FVector::VectorPlaneProject(EnemyLocation - AgentLocation, FVector::UpVector).GetSafeNormal()};
	if (BasisForward.IsNearlyZero())
	{
		return false;
	}

	OutRequest.Reset();
	OutRequest.Agent = Context.Agent;
	OutRequest.Config = Context.CameraConfig;
	OutRequest.AnchorLocation = Context.AnchorLocation;		// Use Context AnchorLocation. Calculate in SquadManagerComponent
	OutRequest.BasisForward = BasisForward;
	OutRequest.SideSign = Context.SideSign;
	
	return true;
}

bool UCombatCameraDirectorComponent::ResolveActionFocusRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest)
{
	if (!Context.Agent.IsValid() || Context.CameraMode != ECombatCameraMode::ActionFocusView)
	{
		return false;
	}

	// not necessary
	FVector BasisForward{FVector::VectorPlaneProject(Context.AgentSectionTransform.GetRotation().GetForwardVector(), FVector::UpVector).GetSafeNormal()};
	if (BasisForward.IsNearlyZero())
	{
		return false;
	}

	OutRequest.Reset();
	OutRequest.Agent = Context.Agent;
	OutRequest.Config = Context.CameraConfig;
	OutRequest.AnchorLocation = Context.AgentSectionTransform.GetLocation();	// Use Agent Current Location as Anchor
	OutRequest.BasisForward = BasisForward;
	OutRequest.SideSign = 1.f;
	return true;
}

bool UCombatCameraDirectorComponent::ActivateCameraSection(const FCombatCameraSectionContext& InContext)
{
	if (!InContext.Agent.IsValid() ||
		InContext.CameraMode == ECombatCameraMode::None ||
		InContext.CameraMode != InContext.CameraConfig.CameraMode)
	{
		return false;
	}

	FCombatCameraSectionContext Context{InContext};
	ApplyPreparedContextIfMatched(Context);
	
	FCombatCameraRequest Request;
	if (!ResolveCameraRequest(Context, Request))
	{
		return false;
	}
	
	SetActiveCameraState(Request);

	if (!InitializeActiveCameraState())
	{
		ActiveCameraState.Reset();
		return false;
	}

	FTransform CameraTransform;
	if (CalculateCurrentCameraTransform(CameraTransform))
	{
		OnUpdateCameraTransform.Broadcast(CameraTransform);
	}
	return true;
}

void UCombatCameraDirectorComponent::DeactivateCameraSection(const ECombatCameraMode CameraMode, APlayerCharacter* Agent)
{
	if (!ActiveCameraState.bActive
	|| !Agent
	|| !ActiveCameraState.Request.Agent.IsValid()
	|| ActiveCameraState.Request.Agent.Get() != Agent
	|| ActiveCameraState.Request.Config.CameraMode != CameraMode)
	{
		return;
	}

	ClearActiveCameraState();
}

bool UCombatCameraDirectorComponent::InitializeStaticActionCameraState()
{
	FVector Forward{FVector::ZeroVector};
	FVector Right{FVector::ZeroVector};
	if (!BuildCameraBasis(ActiveCameraState.Request.BasisForward, Forward, Right))
	{
		return false;
	}

	const FVector& AnchorLocation{ActiveCameraState.Request.AnchorLocation};
	const FVector& Offset{ActiveCameraState.Request.Config.LocalCameraOffset};
	const float SideSign{ActiveCameraState.Request.SideSign};

	ActiveCameraState.LockedCameraLocation = AnchorLocation
		+ Forward * Offset.X
		+ Right * Offset.Y * SideSign
		+ FVector::UpVector * Offset.Z;
	return true;
}

bool UCombatCameraDirectorComponent::CalculateCurrentCameraTransform(FTransform& OutTransform)
{
	if (!ActiveCameraState.bActive)
	{
		return false;
	}
	
	switch (ActiveCameraState.Request.Config.CameraMode)
	{
		case ECombatCameraMode::ParryAssist:
		case ECombatCameraMode::FixedActionView:
			return CalculateFixedPointCameraTransform(OutTransform);
		
		default:
			return false;
	}
}

bool UCombatCameraDirectorComponent::InitializeActiveCameraState()
{
	if (!ActiveCameraState.bActive)
	{
		return false;
	}

	switch (ActiveCameraState.Request.Config.CameraMode)
	{
		case ECombatCameraMode::FixedActionView:	// todo
		case ECombatCameraMode::ParryAssist:
			return InitializeStaticActionCameraState();
		default:
			return false;
	}
}

bool UCombatCameraDirectorComponent::BuildCameraBasis(const FVector& BasisForward, FVector& OutForward, FVector& OutRight)
{
	OutForward = FVector::VectorPlaneProject(BasisForward, FVector::UpVector).GetSafeNormal();

	if (OutForward.IsNearlyZero())
	{
		return false;
	}

	OutRight = FVector::CrossProduct(FVector::UpVector, OutForward).GetSafeNormal();	// Left Handed
	if (OutRight.IsNearlyZero())
	{
		return false;
	}

	return true;
}

bool UCombatCameraDirectorComponent::CalculateFixedPointCameraTransform(FTransform& OutTransform)
{
	if (!ActiveCameraState.bActive)
	{
		return false;
	}

	FVector Forward{FVector::ZeroVector};
	FVector Right{FVector::ZeroVector};
	if (!BuildCameraBasis(ActiveCameraState.Request.BasisForward, Forward, Right))
	{
		return false;
	}

	const FVector& AnchorLocation{ActiveCameraState.Request.AnchorLocation};
	const FVector& LookAtOffset{ActiveCameraState.Request.Config.LocalLookAtOffset};
	const float SideSign{ActiveCameraState.Request.SideSign};

	const FVector CameraLocation{ActiveCameraState.LockedCameraLocation};
	const FVector LookAt = AnchorLocation
		+ Forward * LookAtOffset.X
		+ Right * LookAtOffset.Y * SideSign
		+ FVector::UpVector * LookAtOffset.Z;

	const FRotator CameraRotation{UKismetMathLibrary::FindLookAtRotation(CameraLocation, LookAt)};
	OutTransform = FTransform::Identity;
	OutTransform.SetLocation(CameraLocation);
	OutTransform.SetRotation(CameraRotation.Quaternion());
	OutTransform.SetScale3D(FVector::OneVector);

	return true;
}

void UCombatCameraDirectorComponent::SetActiveCameraState(const FCombatCameraRequest& Request)
{
	ActiveCameraState.Reset();
	ActiveCameraState.bActive = true;
	ActiveCameraState.Request = Request;

	CurrentCameraMode = Request.Config.CameraMode;
}

void UCombatCameraDirectorComponent::ClearActiveCameraState()
{
	ActiveCameraState.Reset();

	CurrentCameraMode = ECombatCameraMode::CombatFollow;
}

void UCombatCameraDirectorComponent::ApplyPreparedContextIfMatched(FCombatCameraSectionContext& OutContext)
{
	if (!PreparedCameraContext.bValid ||
		PreparedCameraContext.CameraMode != OutContext.CameraMode ||
		PreparedCameraContext.Agent != OutContext.Agent)
	{
		return;
	}

	const FCombatCameraContext& PreparedContext{PreparedCameraContext.CombatCameraContext};
	if (PreparedContext.Enemy.IsValid())
	{
		OutContext.Enemy = PreparedContext.Enemy;
	}

	if (PreparedContext.bHasAnchorLocation)
	{
		OutContext.bHasAnchorLocation = true;
		OutContext.AnchorLocation = PreparedContext.AnchorLocation;
	}

	OutContext.SideSign = PreparedContext.SideSign;

	PreparedCameraContext.Reset();
}
