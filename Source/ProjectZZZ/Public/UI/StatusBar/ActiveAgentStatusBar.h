#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActiveAgentStatusBar.generated.h"

class UCharacterCombatComponent;
struct FAgentStatusSnapShot;
struct FHUDSquadSource;
struct FHUDSquadAgentSource;
class UStatProgressBar;
class UProgressBar;

UCLASS()
class PROJECTZZZ_API UActiveAgentStatusBar : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	void InitializeActiveAgentStatusBar();
	
	void BindDelegate(const FHUDSquadSource& Source);

	void UnBindDelegate();

	void RefreshActiveAgentStatus(const FAgentStatusSnapShot& SnapShot);

	UFUNCTION()
	void OnUpdateHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void OnupdateHandleEnergyChanged(float CurrentEnergy, float MaxEnergy);

	UFUNCTION()
	void OnUpdateDecibelsChanged(float CurrentDecibels, float MaxDecibels);
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UStatProgressBar> EnergyBar{nullptr};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> HealthBarMaterial{nullptr};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> HealthTex{nullptr}; 

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> EnergyTex{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> HealthBarMask{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FLinearColor ProgressBarBackgroundColor{FLinearColor::Black};
	
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HealthBarBackgroundMID{nullptr};

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HealthBarFillMID{nullptr};

	inline static const FName HealthBarMaskParameterName{"BarMask"};

	inline static const FName HealthBarTextureParameterName{"BarTex"};

	inline static const FName HealthBarEnableTexScalarParameterName{FName(FString("UseTex"))};	// 0 Disable   1 Enable

	inline static const FName HealthBarColorVectorParameterName{FName(FString("BarColor"))};

private:
	UPROPERTY()
	TWeakObjectPtr<UCharacterCombatComponent> CombatComponent{nullptr};
	
	FDelegateHandle HealthChangedDelegate;

	FDelegateHandle EnergyChangedDelegate;

	FDelegateHandle DecibelsChangedDelegate;
};
