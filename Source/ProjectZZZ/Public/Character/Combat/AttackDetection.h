// Fill out your copyright notice in the Description page of Project Settings.

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
	FRotator ShapeOrientation{FRotator{-90.f, 0.f, 0.f}};
	
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

/*USTRUCT()
struct FWeaponSweepDirectionState
{
public:
	GENERATED_BODY()
	
	void Reset()
	{
		WeaponSweepDirection = FVector::ZeroVector;
		LastFrameWeaponEndPosition = FVector::ZeroVector;
	} 
	
	FVector WeaponSweepDirection{FVector::ZeroVector};

	FVector LastFrameWeaponEndPosition{FVector::ZeroVector};

	float DirectionBlendTime{0.001f};	// const?
};*/

UCLASS()
class UAttackDetectionConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	FName WeaponRootSocketName;

	UPROPERTY(EditDefaultsOnly)
	FName WeaponTipSocketName;
	
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> Channel;

	UPROPERTY(EditDefaultsOnly)
	float MaxSegmentLength{20.f};
	
	UPROPERTY(EditDefaultsOnly)
	int32 SubStepCount{4};

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"))
	int32 SampleCount{10};

	UPROPERTY(EditDefaultsOnly)
	FSweepShapeConfig SweepShapeConfig;
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
	
	UPROPERTY()
	bool bEnableAttackDetection{false};

	UPROPERTY()
	TObjectPtr<UAttackDetectionConfig> DetectionConfig{nullptr};
		
	UPROPERTY()
	TObjectPtr<const UCombatActionStep> AttackingAction{nullptr};

	UPROPERTY()
	bool bIsFirstFrame{true};
	
	UPROPERTY()
	FTransform LastWeaponRootTransform{FTransform::Identity};

	UPROPERTY()
	FTransform LastWeaponTipTransform{FTransform::Identity};
	
	UPROPERTY()
	TArray<AActor*> HitActors;
};
