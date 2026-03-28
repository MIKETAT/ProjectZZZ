#pragma once

#include "LocomotionState.generated.h"

USTRUCT(BlueprintType)
struct PROJECTZZZ_API FLocomotionState
{
	GENERATED_BODY()

	// Location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FVector WorldLocation{ForceInit};
	
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
