#pragma once

#include "CoreMinimal.h"
#include "AttackDetection.generated.h"

class UCombatActionStep;

UENUM(BlueprintType)
enum class EAttackDetectionShapeType : uint8
{
	Box					UMETA(DisplayName = "Box"),
	Sphere				UMETA(DisplayName = "Sphere"),
	Capsule				UMETA(DisplayName = "Capsule"),
};

USTRUCT(BlueprintType)
struct FAttackDetectionShapeConfig
{
	GENERATED_BODY()

public:
	FCollisionShape GetCollisionShape() const
	{
		FCollisionShape Shape;
		switch (ShapeType)
		{
			case EAttackDetectionShapeType::Sphere:
				Shape.SetSphere(SphereRadius);
				break;
			case EAttackDetectionShapeType::Box:
				Shape.SetBox(FVector3f(BoxHalfExtents));
				break;
			case EAttackDetectionShapeType::Capsule:
				Shape.SetCapsule(CapsuleRadius, CapsuleHalfHeight);
				break;
		}
		return Shape;
	}
	
	UPROPERTY(EditDefaultsOnly)
	EAttackDetectionShapeType ShapeType{EAttackDetectionShapeType::Sphere};		// only support sphere for now

	// Sphere Param
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType == EAttackDetectionShapeType::Sphere", EditConditionHides, ClampMin = "1.0"))
	float SphereRadius{1.f};

	// Capsule
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType == EAttackDetectionShapeType::Capsule", EditConditionHides, ClampMin = "1.0"))
	float CapsuleRadius{1.f};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType == EAttackDetectionShapeType::Capsule", EditConditionHides, ClampMin = "1.0"))
	float CapsuleHalfHeight{1.f};

	// Box Param
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "ShapeType == EAttackDetectionShapeType::Box", EditConditionHides, ClampMin = "1.0"))
	FVector BoxHalfExtents{FVector::OneVector};
};

UENUM(BlueprintType)
enum class EAttackDetectionMode : uint8
{
	None						UMETA(DisplayName = "None"),
	WeaponSweep					UMETA(DisplayName = "WeaponSweep"),
	ActorPathSweep				UMETA(DisplayName = "ActorPathSweep"),
	ShapeQueryInstant			UMETA(DisplayName = "ShapeQueryInstant"),
	ShapeQueryContinuous		UMETA(DisplayName = "ShapeQueryContinuous"),
};

/*UENUM(BlueprintType)
enum class EAttackDetectionTriggerMode : uint8
{
	None,
	ContinuousWindow,
	InstantQuery,
};*/

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
	// UnSupported: CurrentTarget, World
};

UENUM(BlueprintType)
enum class EActorPathSweepRotationPolicy : uint8
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
	TEnumAsByte<ECollisionChannel> TraceChannel;

	UPROPERTY(EditDefaultsOnly)
	FAttackDetectionShapeConfig ShapeConfig;

	UPROPERTY(EditDefaultsOnly,
		meta = (EditCondition = "DetectionMode != EAttackDetectionMode::WeaponSweep && DetectionMode != EAttackDetectionMode::None", EditConditionHides))
	FVector ShapeLocalOffset{FVector::ZeroVector};

	UPROPERTY(EditDefaultsOnly,
		meta = (EditCondition = "DetectionMode != EAttackDetectionMode::None", EditConditionHides))
	FRotator ShapeLocalRotation{FRotator::ZeroRotator};

	// WeaponSweep
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	FName WeaponRootSocketName{FName("WeaponRoot")};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	FName WeaponTipSocketName{FName("WeaponTip")};
		
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "2", EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	int32 WeaponSampleCount{2};

	// Actor Path Sweep
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "DetectionMode == EAttackDetectionMode::ActorPathSweep", EditConditionHides))
	EActorPathSweepRotationPolicy PathSweepRotationPolicy{EActorPathSweepRotationPolicy::LockOnBegin};

	// Sweep Detection, WeaponSweep only
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1",
	EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	int32 MaxSubStepCount{8};
	
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.001",
		EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	float MaxSubStepTime{1 / 120.f};		// 单子步时间跨度
	
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.1",
		EditCondition = "DetectionMode == EAttackDetectionMode::WeaponSweep", EditConditionHides))
	float MaxSubStepAngle{10.f};			// 单子步旋转跨度
	
	// ShapeQuery/ShapeQueryContinuous.  ActorPath use Owner
	UPROPERTY(EditDefaultsOnly,
		meta = (EditCondition = "DetectionMode == EAttackDetectionMode::ShapeQueryInstant || DetectionMode == EAttackDetectionMode::ShapeQueryContinuous", EditConditionHides))
	EAttackQueryReference ReferenceType{EAttackQueryReference::Owner};
	
	UPROPERTY(EditDefaultsOnly,
		meta = (EditCondition = "(DetectionMode == EAttackDetectionMode::ShapeQueryInstant || DetectionMode == EAttackDetectionMode::ShapeQueryContinuous) && ReferenceType == EAttackQueryReference::OwnerSocket", EditConditionHides))
	FName ReferenceSocketName{FName("Reference")};
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
struct PROJECTZZZ_API FAttackDetectionSegmentBinding
{
	GENERATED_BODY()

public:
	bool ResolveDetectionSpec(FAttackDetectionSpec& OutSpec) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FName SegmentName{FName("Segment")};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EAttackDetectorSpecSource SpecSource{EAttackDetectorSpecSource::Preset};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "SpecSource == EAttackDetectorSpecSource::Preset", EditConditionHides))
	TObjectPtr<UAttackDetectionPreset> Preset{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "SpecSource == EAttackDetectorSpecSource::Inline", EditConditionHides))
	FAttackDetectionSpec InlineSpec;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EHitDedupePolicy DedupePolicy{EHitDedupePolicy::None};
};

USTRUCT(BlueprintType)
struct PROJECTZZZ_API FAttackDetectionConfig
{
	GENERATED_BODY()

public:
	const FAttackDetectionSegmentBinding* FindSegmentBinding(const FName& InSegmentName) const;

	int32 CountSegmentBindings(const FName& InSegmentName) const; 
	
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

	FResolvedAttackDetectionSegment DetectionSegment{};
	
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
