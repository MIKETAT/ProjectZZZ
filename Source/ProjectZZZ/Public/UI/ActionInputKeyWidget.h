#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActionInputKeyWidget.generated.h"

class UBorder;
class UTextBlock;
struct FActionInputKeyConfig;
class UImage;

UCLASS()
class PROJECTZZZ_API UActionInputKeyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeInputKeyWidget(const FActionInputKeyConfig& Config);

	void RefreshInputKeyWidget(const FActionInputKeyConfig& Config);
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> InputKeyIcon{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeyText{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> OuterBorder{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> InnerBorder{nullptr};
};
