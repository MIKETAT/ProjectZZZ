#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuickAssistWindow.generated.h"


class UImage;

UCLASS()
class PROJECTZZZ_API UQuickAssistWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	void SetAgent(UTexture2D* NewAgent);

	void StartQuickAssistWindow(UTexture2D* NextAgent);

	void ResetAndCloseQuickAssistWindow();

	UFUNCTION()
	void OnPopOutAnimationFinished();
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AgentHead;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_PopIn;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_FlashLoop;

private:
	bool bIsActive{false};

};
