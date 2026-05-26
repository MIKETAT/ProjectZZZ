#pragma once

#include "LocomotionAnimationState.generated.h"

USTRUCT(BlueprintType)
struct PROJECTZZZ_API FLocomotionAnimationState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FVector WorldLocation{ForceInit};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FRotator WorldRotation{ForceInit};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FVector WorldVelocity{ForceInit};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FVector WorldVelocity2D{ForceInit};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FVector LocalVelocity2D{ForceInit};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FVector WorldAcceleration2D{ForceInit};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FVector LocalAcceleration2D{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	float Speed2D{0.f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	float Acceleration2D{0.f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	float DisplacementSinceLastUpdate{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (ClampMin = 0))
	float DisplacementSpeed{0.f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	bool bIsMoving{false};
};

USTRUCT(BlueprintType)
struct FAgentLocomotionAnimationState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	bool bHasMovementInput{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FVector2D MovementInput2D{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Loco State")
	FVector WorldMovementInput{ForceInit};
};

USTRUCT(BlueprintType)
struct FEnemyLocomotionAnimationState
{
	GENERATED_BODY()

	
};
