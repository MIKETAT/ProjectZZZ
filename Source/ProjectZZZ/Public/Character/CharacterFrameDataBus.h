// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "CharacterFrameDataBus.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct PROJECTZZZ_API FCharacterFrameDataBus
{
	GENERATED_BODY()
	
	FCharacterFrameDataBus();
	~FCharacterFrameDataBus();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	uint8 bIsLocalPlayer : 1 {false};
	
	// Player Only
	FInputBitmask InputActionBitmask;

	FVector2D RawMovementInput{FVector2D::ZeroVector};
	FVector2D RawLookInput{FVector2D::ZeroVector};
};
