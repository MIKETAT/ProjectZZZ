// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterFrameDataBus.h"
#include "GameFramework/Character.h"
#include "State/LocomotionState.h"
#include "CharacterBase.generated.h"

struct FCharacterFrameDataBus;
class UPlayerInputHandlerComponent;
struct FInputActionValue;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;

UCLASS()
class PROJECTZZZ_API ACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACharacterBase();

	// Interface
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
protected:
	// ~Interface
	
public:
	const FLocomotionState& GetLocomotionState() const { return LocomotionState; }

protected:
	void RefreshInput(const float DeltaTime);
	void RefreshLocomotionState(const float DeltaTime);

public:
// Components
	
// todo: 规范替代下面的Loco State 变量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	uint8 bHasMovementInput : 1 {false};

	
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	FLocomotionState LocomotionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FCharacterFrameDataBus CharacterFrameDataBus;
};
