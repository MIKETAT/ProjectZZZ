#pragma once

#include "CoreMinimal.h"
#include "Character/Combat/CombatStep.h"
#include "Components/ActorComponent.h"
#include "CombatCameraDirectorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdateCameraTransform, const FTransform&, CameraTransform);

USTRUCT()
struct FActiveCombatCameraState
{
	GENERATED_BODY()

public:
	void Reset()
	{
		bActive = false;
		Request.Reset();
		LockedCameraLocation = FVector::ZeroVector;
		SmoothedTransform = FTransform::Identity;
	}
	
	bool bActive{false};

	FCombatCameraRequest Request;

	FVector LockedCameraLocation{FVector::ZeroVector};
	FTransform SmoothedTransform{FTransform::Identity};
};

USTRUCT()
struct FPreparedCombatCameraContext
{
	GENERATED_BODY()
	
public:
	void Reset()
	{
		bValid = false;
		CameraMode = ECombatCameraMode::None;
		Agent = nullptr;
		CombatCameraContext.Reset();
	}
	
	bool bValid{false};

	ECombatCameraMode CameraMode{ECombatCameraMode::None};

	TWeakObjectPtr<APlayerCharacter> Agent{nullptr};

	FCombatCameraContext CombatCameraContext;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API UCombatCameraDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatCameraDirectorComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void UpdateCamera(const float DeltaTime);

	UFUNCTION(BlueprintCallable)
	ECombatCameraMode GetActiveCombatCameraMode() const { return CurrentCameraMode; }

	void PrepareCameraContext(ECombatCameraMode CameraMode, APlayerCharacter* Agent, const FCombatCameraContext& Context);

	bool ResolveCameraRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest);
	
	// for FixedActionView
	bool ResolveFixedActionViewRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest);

	// for Parry
	bool ResolveParryAssistRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest);

	// for ActionFocusView
	bool ResolveActionFocusRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest);

	// for ForwardDashFollowView
	bool ResolveForwardDashFollowViewRequest(const FCombatCameraSectionContext& Context, FCombatCameraRequest& OutRequest);
	
	bool ActivateCameraSection(const FCombatCameraSectionContext& InContext);

	void DeactivateCameraSection(const ECombatCameraMode CameraMode, APlayerCharacter* Agent);
	
	// 初始化镜头动作镜头
	bool InitializeStaticActionCameraState();

	bool CalculateCurrentCameraTransform(FTransform& OutTransform, const float DeltaTime);
	
	bool InitializeActiveCameraState();

	bool BuildCameraBasis(const FVector& BasisForward, FVector& OutForward, FVector& OutRight);
	
	bool CalculateFixedPointCameraTransform(FTransform& OutTransform);

	bool CalculateActionFocusViewCameraTransform(FTransform& OutTransform, const float DeltaTime);

	bool CalculateForwardDashFollowViewCameraTransform(FTransform& OutTransform, const float DeltaTime);
	
private:
	void SetActiveCameraState(const FCombatCameraRequest& Request);

	void ClearActiveCameraState();

	void ApplyPreparedContextIfMatched(FCombatCameraSectionContext& OutContext);

	void SmoothCameraTransform(const FTransform& TargetTransform, FTransform& OutTransform,
		const float InterpLocationSpeed, const float InterpRotationSpeed, const float DeltaTime);
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnUpdateCameraTransform OnUpdateCameraTransform;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ECombatCameraMode CurrentCameraMode{ECombatCameraMode::CombatFollow};
	
private:
	FActiveCombatCameraState ActiveCameraState;

	FPreparedCombatCameraContext PreparedCameraContext;
};
