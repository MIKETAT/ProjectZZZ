#pragma once

#include "LocomotionAnimationState.generated.h"

USTRUCT(BlueprintType)
struct PROJECTZZZ_API FLocomotionAnimationState
{
	GENERATED_BODY()

	// Location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FVector WorldLocation{ForceInit};

	// todo: Displacement Speed ? bFirstUpdate?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (ClampMin = 0))
	float DisplacementSinceLastUpdate{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (ClampMin = 0))
	float DisplacementSpeed{0.f};
	
	// Rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FRotator WorldRotation{ForceInit};
	
	// Velocity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FVector WorldVelocity{ForceInit};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FVector WorldVelocity2D{ForceInit};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FVector LocalVelocity2D{ForceInit};
	
	// Acceleration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FVector WorldAcceleration2D{ForceInit};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FVector LocalAcceleration2D{ForceInit};
};
