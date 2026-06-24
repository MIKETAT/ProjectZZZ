#pragma once

#include "CoreMinimal.h"
#include "CombatCameraTypes.generated.h"

class APlayerCharacter;
class AEnemyCharacterBase;

UENUM(BlueprintType)
enum class  ECombatCameraMode : uint8
{
	None,
	CombatFollow,
	ParryAssist,
	FixedActionView,
	ActionFocusView,
	ForwardDashFollowView,	// 跟随动作前进方向
	// ...
};

USTRUCT(BlueprintType)
struct FCombatCameraConfig
{
	GENERATED_BODY()

public:
	void Reset()
	{
		bEnableCombatCamera = false;
		CameraMode = ECombatCameraMode::None;
		LocalCameraOffset = FVector::ZeroVector;
		LocalLookAtOffset = FVector::ZeroVector;
		AnchorTargetWeight = 0.f;
		LookAtTargetWeight = 0.f;
	}
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bEnableCombatCamera{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	ECombatCameraMode CameraMode{ECombatCameraMode::None};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	FVector LocalCameraOffset{FVector::ZeroVector};		// 相机相对Anchor的偏移

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	FVector LocalLookAtOffset{FVector::ZeroVector};		// 相机相对 看向点 的偏移

	// Interp Smooth
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	float LocationInterpSpeed{10.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	float RotationInterpSpeed{10.f};
	
	// for ActionFocusView
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bEnableCombatCamera && CameraMode == ECombatCameraMode::ActionFocusView"))
	float AnchorTargetWeight{0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bEnableCombatCamera && CameraMode == ECombatCameraMode::ActionFocusView"))
	float LookAtTargetWeight{0.f};

	// for ForwardDashFollowView
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bEnableCombatCamera && CameraMode == ECombatCameraMode::ForwardDashFollowView"))
	float ForwardFollowWeight{1.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bEnableCombatCamera && CameraMode == ECombatCameraMode::ForwardDashFollowView"))
	float LateralFollowWeight{0.2f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bEnableCombatCamera && CameraMode == ECombatCameraMode::ForwardDashFollowView"))
	float VerticalFollowWeight{0.5f};
};

USTRUCT()
struct FCombatCameraRequest
{
	GENERATED_BODY()
	
public:
	void Reset()
	{
		Agent = nullptr;
		Enemy = nullptr;
		Config.Reset();
		AnchorLocation = FVector::ZeroVector;
		BasisForward = FVector::ZeroVector;
		SideSign = 1.f;
	}
	
	UPROPERTY()
	TWeakObjectPtr<APlayerCharacter> Agent{nullptr};

	UPROPERTY()
	TWeakObjectPtr<ACharacterBase> Enemy{nullptr};

	UPROPERTY()
	FCombatCameraConfig Config;

	UPROPERTY()
	FVector AnchorLocation{FVector::ZeroVector};

	UPROPERTY()
	FVector BasisForward{FVector::ForwardVector};

	UPROPERTY()
	float SideSign{-1.f};
};

USTRUCT()
struct FCombatCameraContext
{
	GENERATED_BODY()

public:
	void Reset()
	{
		bHasAnchorLocation = false;
		AnchorLocation = FVector::ZeroVector;
		SideSign = 1.f;
		Enemy = nullptr;
	}
	
	bool bHasAnchorLocation{false};

	FVector AnchorLocation{FVector::ZeroVector};
	
	UPROPERTY()
	TWeakObjectPtr<ACharacterBase> Enemy{nullptr};

	float SideSign{1.f};
};

// Context when enter NotifyBegin
USTRUCT()
struct FCombatCameraSectionContext
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<APlayerCharacter> Agent{nullptr};

	ECombatCameraMode CameraMode{ECombatCameraMode::None};

	FCombatCameraConfig CameraConfig;

	FTransform AgentSectionTransform{FTransform::Identity};

	TWeakObjectPtr<ACharacterBase> Enemy{nullptr};

	bool bHasAnchorLocation{false};

	FVector AnchorLocation{FVector::ZeroVector};

	float SideSign{1.f};
};