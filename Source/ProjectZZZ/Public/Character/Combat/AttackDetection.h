// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttackDetection.generated.h"

class UCombatActionStep;

UENUM(BlueprintType)
enum class EHitShapeType : uint8
{
	Box					UMETA(DisplayName = "Box"),
	Sphere				UMETA(DisplayName = "Sphere"),
	Capsule				UMETA(DisplayName = "Capsule"),
};

USTRUCT(BlueprintType)
struct FHitShapeConfig
{
	GENERATED_BODY()

public:
	FCollisionShape GetCollisionShape() const
	{
		FCollisionShape Shape;
		switch (ShapeType)
		{
			case EHitShapeType::Sphere:
				Shape.SetSphere(SphereRadius);
				break;
			case EHitShapeType::Box:
				Shape.SetBox(FVector3f(BoxHalfExtents));
				break;
			case EHitShapeType::Capsule:
				Shape.SetCapsule(CapsuleRadius, CapsuleHalfHeight);
				break;
		}
		return Shape;
	}

	bool IsValid() const
	{
		return true;		// todo
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitShapeType ShapeType{EHitShapeType::Sphere};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttackBoneName{"Root"};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ShapeCenter{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform RelativeTransform{FTransform::Identity};

	UPROPERTY(EditAnywhere, meta = (EditConditionHides))
	FRotator ShapeOrientation{FRotator{-90.f, 0.f, 0.f}};
	
	// Sphere Param
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ShapeType == EHitShapeType::Sphere", EditConditionHides, ClampMin = "1.0"))
	float SphereRadius{1.f};

	// Capsule
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ShapeType == EHitShapeType::Capsule", EditConditionHides, ClampMin = "1.0"))
	float CapsuleRadius{1.f};

	UPROPERTY(EditAnywhere, meta = (EditCondition = "ShapeType == EHitShapeType::Capsule", EditConditionHides, ClampMin = "1.0"))
	float CapsuleHalfHeight{1.f};

	// Box Param
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ShapeType == EHitShapeType::Box", EditConditionHides, ClampMin = "1.0"))
	FVector BoxHalfExtents{FVector::OneVector};
};

USTRUCT()
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
};

USTRUCT()
struct FAttackDetectionConfig
{
	GENERATED_BODY()
	
	bool bEnableAttackDetection{false};
	
	FHitShapeConfig ShapeConfig;
	
	FWeaponSweepDirectionState WeaponSweepState;
	
	UPROPERTY()
	TObjectPtr<UCombatActionStep> AttackingAction{nullptr};

	UPROPERTY()
	TArray<AActor*> HitActors;
};
