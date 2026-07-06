#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActionIconWidget.generated.h"

class UActionInputKeyWidget;
struct FActionIconSlotConfig;
class UTextBlock;
class UImage;

UCLASS()
class PROJECTZZZ_API UActionIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeActionIcon(const FActionIconSlotConfig& Config);

	void SetActionIconState(bool bActive = false);
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Icon{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Circle{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActionInputKeyWidget> InputKey{nullptr};

private:
	UPROPERTY()
	TObjectPtr<UTexture2D> DefaultIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> ActiveIcon;
};

