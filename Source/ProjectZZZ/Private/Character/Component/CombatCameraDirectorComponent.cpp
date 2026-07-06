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
	if (CalculateCurrentCameraTransform(CameraTransform, DeltaTime))
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
	case ECombatCameraMode::ForwardDashFollowView:
			return ResolveForwardDashFollowViewRequest(Context, OutRequest);
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
	OutRequest.Enemy = Context.Enemy.Get();
	OutRequest.Config = Context.CameraConfig;
	OutRequest.AnchorLocation = Context.AgentSectionTransform.GetLocation();	// Use Agent Current Location as Anchor
	OutRequest.BasisForward = BasisForward;
	OutRequest.SideSign = 1.f;
	return true;
}

bool UCombatCameraDirectorComponent::ResolveParryAssistRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest)
{
	if (!Context.Agent.IsValid() || !Context.Enemy.IsValid() || !Context.bHasAnchorLocation)
	{
		return false;
	}
	
	const FVector AgentLocation{Context.AgentSectionTransform.GetLocation()};
	const FVector EnemyLocation{Context.Enemy->GetActorLocation()};

	FVector BasisForward{FVector::VectorPlaneProject(EnemyLocation - AgentLocation, FVector::UpVector).GetSafeNormal()};
	if (BasisForward.IsNearlyZero())
	{
		return false;
	}

	OutRequest.Reset();
	OutRequest.Agent = Context.Agent;
	OutRequest.Enemy = Context.Enemy.Get();
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

	OutRequest.Reset();
	OutRequest.Agent = Context.Agent;
	OutRequest.Enemy = Context.Enemy.Get();
	OutRequest.Config = Context.CameraConfig;
	OutRequest.AnchorLocation = Context.AgentSectionTransform.GetLocation();	// Use Agent Current Location as Anchor
	OutRequest.SideSign = Context.SideSign;
	
	return true;
}

bool UCombatCameraDirectorComponent::ResolveForwardDashFollowViewRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest)
{
	if (!Context.Agent.IsValid() || Context.CameraMode != ECombatCameraMode::ForwardDashFollowView)
	{
		return false;
	}

	const FVector AgentLocation{Context.AgentSectionTransform.GetLocation()};
	FVector BasisForward{FVector::ZeroVector};

	if (Context.Enemy.IsValid())
	{
		BasisForward = Context.Enemy->GetActorLocation() - AgentLocation;
	} else
	{
		BasisForward = Context.AgentSectionTransform.GetRotation().GetForwardVector();
	}

	BasisForward = FVector::VectorPlaneProject(BasisForward, FVector::UpVector).GetSafeNormal();
	if (BasisForward.IsNearlyZero())
	{
		return false;
	}

	OutRequest.Reset();

	OutRequest.Agent = Context.Agent;
	OutRequest.Enemy = Context.Enemy;
	OutRequest.Config = Context.CameraConfig;
	OutRequest.AnchorLocation = Context.AgentSectionTransform.GetLocation();
	OutRequest.BasisForward = BasisForward;
	OutRequest.SideSign = Context.SideSign;
	
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
		ClearActiveCameraState();
		return false;
	}

	FTransform CameraTransform;
	// todo: DeltaTime
	if (CalculateCurrentCameraTransform(CameraTransform, 0.f))
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

bool UCombatCameraDirectorComponent::CalculateCurrentCameraTransform(FTransform& OutTransform, const float DeltaTime)
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
		case ECombatCameraMode::ActionFocusView:
			return CalculateActionFocusViewCameraTransform(OutTransform, DeltaTime);
		case ECombatCameraMode::ForwardDashFollowView:
			return CalculateForwardDashFollowViewCameraTransform(OutTransform, DeltaTime);
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
		case ECombatCameraMode::ActionFocusView:
			return true;										// ActionFocusView doest not lock Camera
		case ECombatCameraMode::ForwardDashFollowView:			// ForwardDashFollowView doest not lock Camera
			return true;
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

bool UCombatCameraDirectorComponent::CalculateActionFocusViewCameraTransform(FTransform& OutTransform, const float DeltaTime)
{
	const FCombatCameraRequest& Request = ActiveCameraState.Request;

	if (!Request.Agent.IsValid())
	{
		return false;
	}

	FVector BasisForward{Request.Agent->GetActorForwardVector()};
	FVector AgentLocation{Request.Agent->GetActorLocation()};
	FVector EnemyLocation{AgentLocation};		// 如果没有有效敌人, 将敌人位置设置为角色位置, 相机看向角色(等价于AnchorAlpha/LookAtAlpha为0)
	if (Request.Enemy.IsValid())
	{
		EnemyLocation = Request.Enemy->GetActorLocation();
		BasisForward = FVector::VectorPlaneProject(EnemyLocation - AgentLocation, FVector::UpVector).GetSafeNormal();
	}

	FVector Forward{FVector::ZeroVector};
	FVector Right{FVector::ZeroVector};
	
	if (!BuildCameraBasis(BasisForward, Forward, Right))
	{
		return false;
	}

	const FCombatCameraConfig& Config{Request.Config};

	const float AnchorAlpha{FMath::Clamp(Config.AnchorTargetWeight, 0.f, 1.f)};
	const float LookAtAlpha{FMath::Clamp(Config.LookAtTargetWeight, 0.f, 1.f)};

	const FVector AnchorLocation{FMath::Lerp(AgentLocation, EnemyLocation, AnchorAlpha)};
	const FVector LookAtBase{FMath::Lerp(AgentLocation, EnemyLocation, LookAtAlpha)};

	const FVector CameraLocation = AnchorLocation
									+ Forward * Config.LocalCameraOffset.X
									+ Right * Config.LocalCameraOffset.Y * Request.SideSign
									+ FVector::UpVector * Config.LocalCameraOffset.Z;

	const FVector LookAtLocation = LookAtBase
									+ Forward * Config.LocalLookAtOffset.X
									+ Right * Config.LocalLookAtOffset.Y * Request.SideSign
									+ FVector::UpVector * Config.LocalLookAtOffset.Z;

	const FRotator CameraRotation = UKismetMathLibrary::FindLookAtRotation(CameraLocation, LookAtLocation);


	// Smooth
	SmoothCameraTransform(FTransform(CameraRotation, CameraLocation), OutTransform,
							Config.LocationInterpSpeed, Config.RotationInterpSpeed, DeltaTime);
	return true;
}

bool UCombatCameraDirectorComponent::CalculateForwardDashFollowViewCameraTransform(FTransform& OutTransform, const float DeltaTime)
{
	const FCombatCameraRequest& Request{ActiveCameraState.Request};

	if (!Request.Agent.IsValid())
	{
		return false;
	}

	FVector Forward{FVector::ZeroVector};
	FVector Right{FVector::ZeroVector};
	const FVector Up{FVector::UpVector};
	if (!BuildCameraBasis(Request.BasisForward, Forward, Right))
	{
		return false;
	}

	const FVector AgentLocation{Request.Agent->GetActorLocation()};
	const FVector Origin{Request.AnchorLocation};
	const FVector Delta{AgentLocation - Origin};

	const FVector ForwardAmount{FVector::DotProduct(Delta, Forward)};
	const FVector RightAmount{FVector::DotProduct(Delta, Right)};
	const FVector UpAmount{FVector::DotProduct(Delta, Up)};

	const FCombatCameraConfig& Config{Request.Config};

	const FVector TargetAnchor =
								Origin
								+ Forward * ForwardAmount * Config.ForwardFollowWeight
								+ Right * RightAmount * Config.LateralFollowWeight
								+ Up * UpAmount * Config.VerticalFollowWeight;

	const FVector TargetCameraLocation =
								TargetAnchor
								+ Forward * Config.LocalCameraOffset.X
								+ Right * Config.LocalCameraOffset.Y
								+ Up * Config.LocalCameraOffset.Z;

	const FVector TargetLookAtLocation =
								TargetAnchor
								+ Forward * Config.LocalLookAtOffset.X
								+ Right * Config.LocalLookAtOffset.Y
								+ Up * Config.LocalLookAtOffset.Z;

	const FRotator CameraRotation{UKismetMathLibrary::FindLookAtRotation(TargetCameraLocation, TargetLookAtLocation)};

	const FTransform TargetTransform(CameraRotation, TargetCameraLocation);
	
	SmoothCameraTransform(TargetTransform, OutTransform, Config.LocationInterpSpeed, Config.RotationInterpSpeed, DeltaTime);				
	
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

void UCombatCameraDirectorComponent::SmoothCameraTransform(const FTransform& TargetTransform, FTransform& OutTransform,
														const float InterpLocationSpeed, const float InterpRotationSpeed, const float DeltaTime)
{
	if (DeltaTime <= 0.f)
	{
		return;
	}

	const FVector SmoothedLocation{FMath::VInterpTo(ActiveCameraState.SmoothedTransform.GetLocation(),
													TargetTransform.GetLocation(), DeltaTime, InterpLocationSpeed)};
	const FRotator SmoothedRotation{FMath::RInterpTo(ActiveCameraState.SmoothedTransform.GetRotation().Rotator(),
													TargetTransform.GetRotation().Rotator(), DeltaTime, InterpRotationSpeed)};

	ActiveCameraState.SmoothedTransform = FTransform(SmoothedRotation, SmoothedLocation);

	OutTransform = ActiveCameraState.SmoothedTransform;
}
