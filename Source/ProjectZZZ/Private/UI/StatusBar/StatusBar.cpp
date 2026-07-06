#include "UI/StatusBar/StatusBar.h"
#include "Character/Component/SquadManagerComponent.h"
#include "UI/StatusBar/ActiveAgentHealthWidget.h"
#include "UI/StatusBar/ActiveAgentStatusBar.h"
#include "UI/StatusBar/AgentHead.h"
#include "UI/StatusBar/AgentStatusBar.h"

void UStatusBar::BindDelegate(const FHUDSquadSource& Source)
{
	ActiveAgentHead->BindDelegate(Source);
	ActiveAgentStatusBar->BindDelegate(Source);
	SecondAgentStatBar->BindDelegate(Source.SecondAgent);
	ThirdAgentStatBar->BindDelegate(Source.ThirdAgent);
	ActiveAgentHealthWidget->BindDelegates(Source.ActiveAgent);
}

void UStatusBar::RefreshStatusBar(const FSquadStatusSnapshot& Snapshot)
{
	ActiveAgentHead->RefreshAgentHead(Snapshot.ActiveAgentStatus);
	ActiveAgentStatusBar->RefreshActiveAgentStatus(Snapshot.ActiveAgentStatus);
	SecondAgentStatBar->RefreshAgentStatus(Snapshot.SecondAgentStatus);
	ThirdAgentStatBar->RefreshAgentStatus(Snapshot.ThirdAgentStatus);
	ActiveAgentHealthWidget->RefreshActiveAgentHealthWidget(Snapshot.ActiveAgentStatus);
}
