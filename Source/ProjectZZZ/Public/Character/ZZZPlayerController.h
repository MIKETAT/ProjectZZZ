// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/SquadManagerComponent.h"
#include "GameFramework/PlayerController.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "ZZZPlayerController.generated.h"

class UCombatCameraDirectorComponent;
class UPostProcessComponent;
struct FPendingUltimateCutInRequest;
class APlayerCharacter;
class UGameplayCameraComponent;
class UQuickAssistWindow;
class UQTEWidget;
class USquadManagerComponent;
class UPlayerInputHandlerComponent;
class UInputMappingContext;

USTRUCT()
struct FCutInStencilData
{
	GENERATED_BODY()
	
	TWeakObjectPtr<UPrimitiveComponent> Component;

	bool bEnableRenderCustomDepth{false};

	int32 CustomStencilDepth{0};
};

UCLASS()
class PROJECTZZZ_API AZZZPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AZZZPlayerController();
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaTime) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;
	
	UPlayerInputHandlerComponent* GetPlayerInputHandlerComponent() const { return PlayerInputHandlerComponent; }

	UFUNCTION(BlueprintCallable)
	USquadManagerComponent* GetSquadManagerComponent() const { return SquadManager; }

	UFUNCTION()
	UCombatCameraDirectorComponent* GetCameraDirectorComponent() const { return CameraDirectorComponent;}
	
	UFUNCTION(BlueprintCallable)
	APlayerCharacter* GetActiveAgent() const;

	bool GetBlockGameplayCameraActivation() const { return bBlockGameplayCameraActivation; }
	
	void SetBlockGameplayCameraActivation(bool bBlock) { bBlockGameplayCameraActivation = bBlock; }
	
	void RequestUltimateCutIn(const FPendingUltimateCutInRequest& Request);
	
	void CommitPendingUltimateCutIn();

	void ClearPreparedSequence(ULevelSequencePlayer* SequencePlayer, ALevelSequenceActor* SequenceActor);
	
	void CancelPendingUltimateCutIn();

	UFUNCTION()
	void HandleSequencePlayFinished();

	UFUNCTION()
	void HandleUltimateActionFinished(APlayerCharacter* Agent, ECombatAnimRequestFinishReason Reason);

	UFUNCTION(BlueprintCallable)
	void UpdateCombatCameraForEvaluator(const float DeltaTime);

	UFUNCTION(BlueprintCallable)
	ECombatCameraMode GetActiveCombatCameraMode() const;

	UFUNCTION()
	ECombatEventHandleResult HandleParrySucceedEvent(const FCombatEventMessage& Message);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayParryRadialBlur();

	UFUNCTION(BlueprintCallable)
	void SetParryRadialBlurParams(const float BlurOffset, const float BlurIntensity);
private:
	void CreateQTEWidget();
	
	void BindUIDelegate();

	void CreateQuickAssistWidget();

	void EnableUltimateCutInStencil(APlayerCharacter* Agent, const int32 StencilValue = 1);

	void DisableUltimateCutInStencil();

	void EnableUltimateCutInPostProcess(APlayerCharacter* Agent, const FLinearColor& BackgroundColor, const int32 StencilValue);

	void DisableUltimateCutInPostProcess();

	void BindParrySucceedEvent();

	void UnbindParrySucceededEvent();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCinematicCameraOverrideActive{false};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UQTEWidget> QTEWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UQuickAssistWindow> QuickAssistWidgetClass;
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPlayerInputHandlerComponent> PlayerInputHandlerComponent{nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "Feature | Ultimate")
	TObjectPtr<UMaterialInterface> UltimateCutInPostProcessMaterial{nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "Feature | Ultimate")
	TObjectPtr<UMaterialParameterCollection> MPC_UltimateCutIn;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bUltimateCutInPostProcessActive{false};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USquadManagerComponent> SquadManager{nullptr};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCombatCameraDirectorComponent> CameraDirectorComponent{nullptr};
	
	UPROPERTY()
	TObjectPtr<UQTEWidget> QTEWidget{nullptr};

	UPROPERTY()
	TObjectPtr<UQuickAssistWindow> QuickAssistWidget{nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "Feature | Parry")
	TObjectPtr<UMaterialParameterCollection> ParryRadialBlurMPC{nullptr};
	
	FDelegateHandle ParrySucceededDelegateHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bBlockGameplayCameraActivation{false};

	UPROPERTY(Transient)
	FPendingUltimateCutInRequest PendingUltimateCutInRequest;

	UPROPERTY(Transient)
	FActiveUltimateExecutionState UltimateExecutionState;

	UPROPERTY(Transient)
	TArray<FCutInStencilData> CutInStencilDate;
	
// Default	
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;
};

