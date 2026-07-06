#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatProgressBar.generated.h"

class UProgressBar;

UCLASS()
class PROJECTZZZ_API UStatProgressBar : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void InitializeStatProgressBar();
	
	void SetPercentage(const float Percentage);

	void InitializeProgressBarFillImage(UTexture2D* FillImage);
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> ProgressBarMaterial{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> BarMask{nullptr};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FLinearColor BackgroundBarColor{FLinearColor::Black};		// 404040FF
	
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FillImageMID{nullptr};

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BackgroundMID{nullptr};

	inline static const FName ProgressBarMaskParameterName{FName(FString("BarMask"))};
	
	inline static const FName ProgressBarTexParameterName{FName(FString("BarTex"))};

	inline static const FName ProgressBarEnableTexScalarParameterName{FName(FString("UseTex"))};	// 0 Disable   1 Enable

	inline static const FName ProgressBarColorVectorParameterName{FName(FString("BarColor"))};
};
