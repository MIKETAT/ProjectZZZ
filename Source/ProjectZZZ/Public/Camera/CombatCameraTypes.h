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
	}
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bEnableCombatCamera{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	ECombatCameraMode CameraMode{ECombatCameraMode::None};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	FVector LocalCameraOffset{FVector::ZeroVector};		// 相机相对Anchor的偏移

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	FVector LocalLookAtOffset{FVector::ZeroVector};		// 相机相对 看向点 的偏移

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	float AnchorTargetWeight{0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = bEnableCombatCamera))
	float LookAtTargetWeight{0.f};
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
	TWeakObjectPtr<AEnemyCharacterBase> Enemy{nullptr};

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
	TWeakObjectPtr<AEnemyCharacterBase> Enemy{nullptr};

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