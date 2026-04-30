// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "ZZZPlayerController.generated.h"

class USquadManagerComponent;
class UPlayerInputHandlerComponent;
class UInputMappingContext;

UCLASS()
class PROJECTZZZ_API AZZZPlayerController : public APlayerController
{
	GENERATED_BODY()

	AZZZPlayerController();

public:
	UPlayerInputHandlerComponent* GetPlayerInputHandlerComponent() const { return PlayerInputHandlerComponent; }

	bool HasMovementInput() const {return PlayerInputHandlerComponent && PlayerInputHandlerComponent->HasMovementInput(); }
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPlayerInputHandlerComponent> PlayerInputHandlerComponent{nullptr};

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USquadManagerComponent> SquadManager{nullptr};

// Default	
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

};
