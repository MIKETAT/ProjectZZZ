// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QTEWidget.generated.h"

class USquadManagerComponent;
class UQTEAgent;
class UTextBlock;
struct FCombatEventMessage;
class UProgressBar;
class UImage;
/**
 * 
 */
UCLASS()
class PROJECTZZZ_API UQTEWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	void StartQTEWindow(UTexture2D* PreviousAgentHead, UTexture2D* NextAgentHead);	// todo: parameters

	void ResetAndCloseQTEWindow();

	void InitializePtr(USquadManagerComponent* Squad) { SquadManager = Squad; }
protected:
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateQTEVisuals();

private:
	void RefreshWindow();

public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UQTEAgent> PreviousAgent;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UQTEAgent> NextAgent;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountDownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> QTEProgressBar;

	UPROPERTY()
	TWeakObjectPtr<USquadManagerComponent> SquadManager;

protected:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ProgressBarMID{nullptr};

};
