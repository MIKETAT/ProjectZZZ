#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AgentStatusBar.generated.h"

class UCharacterCombatComponent;
struct FAgentStatusSnapShot;
class USquadManagerComponent;
struct FHUDSquadSource;
class UStatProgressBar;
class UAgentHead;
struct FHUDSquadAgentSource;

UCLASS()
class PROJECTZZZ_API UAgentStatusBar : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	void BindDelegate(const FHUDSquadAgentSource& Source);

	void UnBindDelegate();

	void InitializeAgentStatusBar();
	
	void RefreshAgentStatus(const FAgentStatusSnapShot& SnapShot);

	void OnUpdateAgentHead(const FAgentStatusSnapShot& SnapShot);

	void OnUpdateHealth(float CurrentHealth, float MaxHealth);

	void OnUpdateEnergy(float CurrentEnergy, float MaxEnergy);

	void OnUpdateDecibel(float CurrentDecibel, float MaxDecibel);
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgentHead> AgentHead{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UStatProgressBar> AgentHealthBar{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UStatProgressBar> AgentDecibelsBar{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UStatProgressBar> AgentEnergyBar{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> HealthTex{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> EnergyTex{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> DecibelsTex{nullptr};

private:
	UPROPERTY()
	TWeakObjectPtr<UCharacterCombatComponent> CombatComponent{nullptr};
	
	FDelegateHandle HealthChangedDelegate;

	FDelegateHandle EnergyChangedDelegate;

	FDelegateHandle DecibelsChangedDelegate;
};
