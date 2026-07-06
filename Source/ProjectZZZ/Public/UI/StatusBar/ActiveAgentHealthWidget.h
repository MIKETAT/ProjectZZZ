#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActiveAgentHealthWidget.generated.h"

struct FAgentStatusSnapShot;
class UCharacterCombatComponent;
struct FHUDSquadAgentSource;
class UTextBlock;

UCLASS()
class PROJECTZZZ_API UActiveAgentHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindDelegates(const FHUDSquadAgentSource& Source);

	void UnBindDelegate();
	
	void RefreshActiveAgentHealthWidget(const FAgentStatusSnapShot& Snapshot);

	UFUNCTION()
	void OnUpdateHealthChanged(float CurrentHealth, float MaxHealth);
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText{nullptr};

private:
	UPROPERTY()
	TWeakObjectPtr<UCharacterCombatComponent> CombatComponent{nullptr};
	
	FDelegateHandle HealthChangedDelegateHandle;
};
