#include "UI/HUDWidget.h"

#include "Character/ZZZPlayerController.h"
#include "Character/Component/SquadManagerComponent.h"
#include "UI/ActionIconPanelWidget.h"
#include "UI/ActionIconPreset.h"
#include "UI/StatusBar/ActiveAgentDecibelWidget.h"
#include "UI/StatusBar/StatusBar.h"

void UHUDWidget::InitializeHUD(UActionIconPreset* Preset, AZZZPlayerController* PC, USquadManagerComponent* SquadManagerComponent)
{
	if (!Preset || !PC || !SquadManagerComponent)
	{
		return;
	}

	if (!ActionIconPanel)
	{
		return;
	}

	// Cache Pointers
	PlayerController = PC;
	SquadManager = SquadManagerComponent;
	
	ActionIconPanel->InitializePanelWidget(Preset);
}

void UHUDWidget::BindDelegate(const FHUDSquadSource& Source, AZZZPlayerController* PC)
{
	if (StatusBar && ActionIconPanel)
	{
		StatusBar->BindDelegate(Source);
		ActionIconPanel->BindDelegate(Source.ActiveAgent);
		ActiveAgentDecibelWidget->BindDelegates(Source.ActiveAgent);
	}

	if (PC)
	{
		PC->OnUltimateExecutionStatusChangedDelegate.AddUObject(this, &ThisClass::HandleUltimateExecutionStatusChanged);
	}
}

void UHUDWidget::RefreshHUD(const FSquadStatusSnapshot& Snapshot)
{
	if (StatusBar)
	{
		StatusBar->RefreshStatusBar(Snapshot);
	}

	if (ActionIconPanel)
	{
		ActionIconPanel->RefreshActionIconPanel(Snapshot);
	}

	if (ActiveAgentDecibelWidget)
	{
		ActiveAgentDecibelWidget->RefreshActiveAgentDecibelsWidget(Snapshot.ActiveAgentStatus);
	}
}

void UHUDWidget::HandleUltimateExecutionStatusChanged(bool bFinished)
{
	if (bFinished)
	{
		SetVisibility(ESlateVisibility::Visible);
	} else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
