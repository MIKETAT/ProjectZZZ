#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatusBar.generated.h"

class UActiveAgentDecibelWidget;
class UActiveAgentHealthWidget;
class UTextBlock;
struct FSquadStatusSnapshot;
struct FHUDSquadSource;
class UAgentStatusBar;
class UActiveAgentStatusBar;
class UAgentHead;
class UCharacterCombatComponent;

UCLASS()
class PROJECTZZZ_API UStatusBar : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindDelegate(const FHUDSquadSource& Source);

	void RefreshStatusBar(const FSquadStatusSnapshot& Snapshot);
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgentHead> ActiveAgentHead{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActiveAgentStatusBar> ActiveAgentStatusBar{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgentStatusBar> SecondAgentStatBar{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgentStatusBar> ThirdAgentStatBar{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UActiveAgentHealthWidget> ActiveAgentHealthWidget{nullptr};
};
