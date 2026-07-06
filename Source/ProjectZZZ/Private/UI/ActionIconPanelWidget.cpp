#include "UI/ActionIconPanelWidget.h"

#include "Character/Component/SquadManagerComponent.h"
#include "Player/PlayerCharacter.h"
#include "UI/ActionIconPreset.h"
#include "UI/ActionIconWidget.h"
#include "UI/UITypes.h"

void UActionIconPanelWidget::InitializePanelWidget(UActionIconPreset* InPreset)
{
	if (!InPreset)
	{
		return;
	}

	Preset = InPreset;

	for (const FActionIconSlotConfig& Config : Preset->SlotConfigs)
	{
		switch (Config.IconSlot)
		{
			case EActionIconSlot::BasicAttack:
				BasicAttack->InitializeActionIcon(Config);
				break;
			case EActionIconSlot::Dodge:
				Dodge->InitializeActionIcon(Config);
				break;
			case EActionIconSlot::SpecialAttack:
				SpecialAttack->InitializeActionIcon(Config);
				break;
			case EActionIconSlot::Switch:
				Switch->InitializeActionIcon(Config);
				break;
			case EActionIconSlot::Ultimate:
				Ultimate->InitializeActionIcon(Config);
				break;
			default:
				break;
		}
	}
}

void UActionIconPanelWidget::SetObservedAgent(APlayerCharacter* NewAgent)
{
	if (!NewAgent)
	{
		return;
	}

	UCharacterCombatComponent* CombatComponent{NewAgent->GetAgentCombatComponent()};
	if (!CombatComponent)
	{
		return;
	}
	
	if (ActionExecutableHandler.IsValid())
	{
		CombatComponent->OnActionExecutableChanged.Remove(ActionExecutableHandler);
	}

	CombatComponent->OnActionExecutableChanged.AddUObject(this, &ThisClass::HandleAgentActionExecutableChanged);
}

void UActionIconPanelWidget::BindDelegate(const FHUDSquadAgentSource& Source)
{
	SetObservedAgent(Source.Agent.Get());
	
}

void UActionIconPanelWidget::RefreshActionIconPanel(const FSquadStatusSnapshot& Snapshot)
{
	if (SpecialAttack)
	{
		SpecialAttack->SetActionIconState(Snapshot.ActiveAgentStatus.bCanExecuteSpecialAttackEX);
	}

	if (Ultimate)
	{
		Ultimate->SetActionIconState(Snapshot.ActiveAgentStatus.bCanExecuteUltimate);
	}
}

void UActionIconPanelWidget::HandleAgentActionExecutableChanged(EActionIconSlot ActionIconSlot, bool bCanExecute)
{
	if (ActionIconSlot == EActionIconSlot::SpecialAttack)
	{
		SpecialAttack->SetActionIconState(bCanExecute);
	} else if (ActionIconSlot == EActionIconSlot::Ultimate)
	{
		Ultimate->SetActionIconState(bCanExecute);
	}
}
