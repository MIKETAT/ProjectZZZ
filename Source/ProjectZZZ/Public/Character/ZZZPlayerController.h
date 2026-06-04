// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "ZZZPlayerController.generated.h"

class APlayerCharacter;
class UGameplayCameraComponent;
class UQuickAssistWindow;
class UQTEWidget;
class USquadManagerComponent;
class UPlayerInputHandlerComponent;
class UInputMappingContext;

UCLASS()
class PROJECTZZZ_API AZZZPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AZZZPlayerController();
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;
	
	UPlayerInputHandlerComponent* GetPlayerInputHandlerComponent() const { return PlayerInputHandlerComponent; }

	bool HasMovementInput() const {return PlayerInputHandlerComponent && PlayerInputHandlerComponent->HasMovementInput(); }

	UFUNCTION(BlueprintCallable)
	USquadManagerComponent* GetSquadManagerComponent() const { return SquadManager; }

	UFUNCTION(BlueprintCallable)
	APlayerCharacter* GetActiveAgent() const;
	
private:
	void CreateQTEWidget();
	
	void BindUIDelegate();

	void CreateQuickAssistWidget();
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer CameraStatusTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UQTEWidget> QTEWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UQuickAssistWindow> QuickAssistWidgetClass;
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPlayerInputHandlerComponent> PlayerInputHandlerComponent{nullptr};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USquadManagerComponent> SquadManager{nullptr};
	
	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGameplayCameraComponent> GameplayCamera;*/

	UPROPERTY()
	TObjectPtr<UQTEWidget> QTEWidget{nullptr};

	UPROPERTY()
	TObjectPtr<UQuickAssistWindow> QuickAssistWidget{nullptr};
	
// Default	
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;
};

