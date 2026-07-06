#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActiveAgentDecibelWidget.generated.h"

struct FAgentStatusSnapShot;
class UTextBlock;
class UCharacterCombatComponent;
struct FHUDSquadAgentSource;

UCLASS()
class PROJECTZZZ_API UActiveAgentDecibelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindDelegates(const FHUDSquadAgentSource& Source);

	void UnBindDelegate();

	void RefreshActiveAgentDecibelsWidget(const FAgentStatusSnapShot& Snapshot);

	UFUNCTION()
	void OnUpdateDecibelsChanged(float CurrentDecibels, float MaxDecibels);
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DecibelsText{nullptr};

private:
	UPROPERTY()
	TWeakObjectPtr<UCharacterCombatComponent> CombatComponent{nullptr};
	
	FDelegateHandle DecibelsChangedDelegateHandle;
};
