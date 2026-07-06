#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActionIconPanelWidget.generated.h"

struct FSquadStatusSnapshot;
struct FHUDSquadAgentSource;
enum class EActionIconSlot : uint8;
class APlayerCharacter;
class UActionIconPreset;
class UActionIconWidget;

UCLASS()
class PROJECTZZZ_API UActionIconPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializePanelWidget(UActionIconPreset* InPreset);

	void SetObservedAgent(APlayerCharacter* NewAgent);

	void BindDelegate(const FHUDSquadAgentSource& Source);

	void RefreshActionIconPanel(const FSquadStatusSnapshot& Snapshot);
	
	UFUNCTION()
	void HandleAgentActionExecutableChanged(EActionIconSlot ActionIconSlot, bool bCanExecute);
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActionIconWidget> BasicAttack{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActionIconWidget> Dodge{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActionIconWidget> SpecialAttack{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActionIconWidget> Switch{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActionIconWidget> Ultimate{nullptr};

	UPROPERTY(Transient)
	TObjectPtr<UActionIconPreset> Preset{nullptr};

private:
	FDelegateHandle ActionExecutableHandler;
};
