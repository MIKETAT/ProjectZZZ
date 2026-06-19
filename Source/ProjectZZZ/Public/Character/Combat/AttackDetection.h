#pragma once

#include "CoreMinimal.h"
#include "AttackDetection.generated.h"

class UCombatActionStep;

UENUM(BlueprintType)
enum class ESweepShapeType : uint8
{
	Box					UMETA(DisplayName = "Box"),
	Sphere				UMETA(DisplayName = "Sphere"),
	Capsule				UMETA(DisplayName = "Capsule"),
};

USTRUCT(BlueprintType)
struct FSweepShapeConfig
{
	GENERATED_BODY()

public:
	FCollisionShape GetCollisionShape() const
	{
		FCollisionShape Shape;
		switch (ShapeType)
		{
			case ESweepShapeType::Sphere:
				Shape.SetSphere(SphereRadius);
				break;
			case ESweepShapeType::Box:
				Shape.SetBox(FVector3f(BoxHalfExtents));
				break;
			case ESweepShapeType::Capsule:
				Shape.SetCapsule(CapsuleRadius, CapsuleHalfHeight);
				break;
		}
		return Shape;
	}

	bool IsValid() const
	{
		return bValidConfig;
	}
	
	UPROPERTY(EditDefaultsOnly)
	ESweepShapeType ShapeType{ESweepShapeType::Sphere};		// only support sphere for now

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType != ESweepShapeType::Sphere"))
	FRotator ShapeRotation{FRotator{-90.f, 0.f, 0.f}};
	
	// Sphere Param
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType == ESweepShapeType::Sphere", EditConditionHides, ClampMin = "1.0"))
	float SphereRadius{1.f};

	// Capsule
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType == ESweepShapeType::Capsule", EditConditionHides, ClampMin = "1.0"))
	float CapsuleRadius{1.f};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType == ESweepShapeType::Capsule", EditConditionHides, ClampMin = "1.0"))
	float CapsuleHalfHeight{1.f};

	// Box Param
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType == ESweepShapeType::Box", EditConditionHides, ClampMin = "1.0"))
	FVector BoxHalfExtents{FVector::OneVector};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bValidConfig{false};
};

UENUM(BlueprintType)
enum class EAttackDetectionMode : uint8
{
	None,
	WeaponSweep,
	ActorPathSweep,
	ShapeQuery,
};

UENUM(BlueprintType)
enum class EAttackDetectionTriggerMode : uint8
{
	None,
	ContinuousWindow,
	InstantQuery,
};

UENUM(BlueprintType)
enum class EHitDedupePolicy : uint8
{
	None,
	OncePerActivation,
	OncePerAction,
	//MultiHitInterval
};

// for ShapeQuery
UENUM(BlueprintType)
enum class EAttackQueryReference : uint8
{
	None,
	Owner,
	OwnerSocket,
	CurrentTarget,
	World
};

UENUM(BlueprintType)
enum class EActorPathSweepRotaionPolicy : uint8
{
	LockOnBegin,
	FollowActor,
};

USTRUCT(BlueprintType)
struct FAttackDetectionSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	EAttackDetectionMode DetectionMode{EAttackDetectionMode::None};

	UPROPERTY(EditDefaultsOnly)
	EAttackDetectionTriggerMode TriggerMode{EAttackDetectionTriggerMode::None};

	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> TraceChannel;

	UPROPERTY(EditDefaultsOnly)
	FSweepShapeConfig SweepShapeConfig;

	// WeaponSweep
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	FName WeaponRootSocketName{FName("WeaponRoot")};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	FName WeaponTipSocketName{FName("WeaponTip")};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep || DetectionMode == EAttackDetectionMode::ActorPathSweep", EditConditionHides))
	int32 MaxSubStepCount{1};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep || DetectionMode == EAttackDetectionMode::ActorPathSweep", EditConditionHides))
	float MaxSubStepTime{1 / 120.f};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	int32 SampleCount{1};
	
	// ActorPathSweep / ShapeQuery / DelayedShapeQuery / AttachedShape
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::ShapeQuery", EditConditionHides))
	EAttackQueryReference ReferenceType{EAttackQueryReference::None};
	
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::ShapeQuery && ReferenceType == EAttackQueryReference::OwnerSocket", EditConditionHides))
	FName ReferenceSocketName{FName("Reference")};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::ActorPathSweep", EditConditionHides))
	FTransform SweepShapeLocalOffset{FTransform::Identity};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::ActorPathSweep", EditConditionHides))
	EActorPathSweepRotaionPolicy PathSweepRotationPolicy{EActorPathSweepRotaionPolicy::LockOnBegin};
	
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::ShapeQuery", EditConditionHides))
	FTransform QueryLocalOffset{FTransform::Identity};
	
	/*UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DedupePolicy == EHitDedupePolicy::MultiHitInterval", EditConditionHides))
	float MultiHitInterval{0.2f};*/
};

UENUM(BlueprintType)
enum class EAttackDetectorSpecSource : uint8
{
	Preset,
	Inline
};

UCLASS(BlueprintType)
class UAttackDetectionPreset : public  UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FAttackDetectionSpec DetectionSpec;
};

USTRUCT(BlueprintType)
struct FAttackDetectionSegmentBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FName SegmentName{FName("Segment")};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EAttackDetectorSpecSource SpecSource{EAttackDetectorSpecSource::Preset};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UAttackDetectionPreset> Preset{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "SpecSource == EAttackDetectorSpecSource::Inline", EditConditionHides))
	FAttackDetectionSpec InlineSpec;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EHitDedupePolicy DedupePolicy{EHitDedupePolicy::None};

	// todo: DOT
	/*UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "DedupePolicy == EHitDedupePolicy::MultiHitInterval", EditConditionHides))
	float MultiHitInterval{0.2f};*/
};

USTRUCT(BlueprintType)
struct FAttackDetectionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bEnableDetection{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bEnableDetection == true", EditConditionHides))
	TArray<FAttackDetectionSegmentBinding> Segments;
};

// Runtime 
USTRUCT()
struct FResolvedAttackDetectionSegment
{
	GENERATED_BODY()

	FName SegmentName{FName("Default")};

	FAttackDetectionSpec DetectionSpec;

	EHitDedupePolicy DedupePolicy{EHitDedupePolicy::None};

	int32 ActionRequestId{INDEX_NONE};

	TWeakObjectPtr<const UCombatActionStep> SourceAction{nullptr};
};


USTRUCT(BlueprintType)
struct FDetectionDebugConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bDrawDebug{false};

	UPROPERTY(EditDefaultsOnly)
	float DrawTime{5.f};

	UPROPERTY(EditDefaultsOnly)
	FLinearColor TraceColor{FLinearColor::Green};

	UPROPERTY(EditDefaultsOnly)
	FLinearColor HitColor{FLinearColor::Red};
};

USTRUCT()
struct FAttackDetectionStatus
{
	GENERATED_BODY()

	void ResetAll();

	// 重置当前检测窗口, 对应NotifyState
	void ResetActivationState();

	// 重置某动作检测状态, 对应ActionStep
	void ClearActionHitActor();
	
	UPROPERTY()
	bool bActive{false};

	FResolvedAttackDetectionSegment DetectionSegment;
	
	bool bIsFirstFrame{true};

	// for WeaponSweep
	FTransform LastWeaponRootTransform{FTransform::Identity};

	FTransform LastWeaponTipTransform{FTransform::Identity};

	// for ActorPathSweep
	FTransform LastOwnerTransform{FTransform::Identity};

	FVector LockedForward{FVector::ForwardVector};
	
	FRotator LockedRotator{FRotator::ZeroRotator};
	
	TSet<TObjectKey<AActor>> SegmentHitActors;

	TSet<TObjectKey<AActor>> ActionHitActors;	// for PerAction

	int32 CurrentActionRequestId{INDEX_NONE};
};

inline void FAttackDetectionStatus::ResetAll()
{
	ResetActivationState();
	ClearActionHitActor();
}

inline void FAttackDetectionStatus::ResetActivationState()
{
	bActive = false;
	bIsFirstFrame = true;
	LastOwnerTransform = FTransform::Identity;
	LastWeaponRootTransform = FTransform::Identity;
	LastWeaponTipTransform = FTransform::Identity;
	LockedForward = FVector::ForwardVector;
	LockedRotator = FRotator::ZeroRotator;
	DetectionSegment = FResolvedAttackDetectionSegment();
	SegmentHitActors.Empty();
}

inline void FAttackDetectionStatus::ClearActionHitActor()
{
	ActionHitActors.Empty();
}
