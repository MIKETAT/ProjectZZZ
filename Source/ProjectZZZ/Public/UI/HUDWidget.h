#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UActiveAgentDecibelWidget;
struct FSquadStatusSnapshot;
struct FHUDSquadSource;
class UCharacterCombatComponent;
class UStatusBar;
class APlayerCharacter;
class USquadManagerComponent;
class AZZZPlayerController;
class UActionIconPreset;
class UActionIconPanelWidget;

UCLASS()
class PROJECTZZZ_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeHUD(UActionIconPreset* Preset, AZZZPlayerController* PC, USquadManagerComponent* SquadManagerComponent);

	void BindDelegate(const FHUDSquadSource& Source, AZZZPlayerController* PC);

	void RefreshHUD(const FSquadStatusSnapshot& Snapshot);

	UFUNCTION()
	void HandleUltimateExecutionStatusChanged(bool bFinished);
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActionIconPanelWidget> ActionIconPanel{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UStatusBar> StatusBar{nullptr};
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActiveAgentDecibelWidget> ActiveAgentDecibelWidget{nullptr};
private:
	UPROPERTY()
	TObjectPtr<AZZZPlayerController> PlayerController{nullptr};

	UPROPERTY()
	TObjectPtr<USquadManagerComponent> SquadManager{nullptr};
};
