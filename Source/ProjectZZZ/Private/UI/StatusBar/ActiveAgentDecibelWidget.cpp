#include "UI/StatusBar/ActiveAgentDecibelWidget.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Components/TextBlock.h"

void UActiveAgentDecibelWidget::BindDelegates(const FHUDSquadAgentSource& Source)
{
	if (!Source.CombatComponent.IsValid())
	{
		return;
	}

	UnBindDelegate();

	CombatComponent = Source.CombatComponent;
	DecibelsChangedDelegateHandle = Source.CombatComponent->OnDecibelsChanged.AddUObject(this, &ThisClass::OnUpdateDecibelsChanged);
}

void UActiveAgentDecibelWidget::UnBindDelegate()
{
	if (CombatComponent.IsValid() && DecibelsChangedDelegateHandle.IsValid())
	{
		CombatComponent->OnHealthChanged.Remove(DecibelsChangedDelegateHandle);
	}
}

void UActiveAgentDecibelWidget::RefreshActiveAgentDecibelsWidget(const FAgentStatusSnapShot& Snapshot)
{
	OnUpdateDecibelsChanged(Snapshot.CurrentDecibels, Snapshot.MaxDecibels);
}

void UActiveAgentDecibelWidget::OnUpdateDecibelsChanged(float CurrentDecibels, float MaxDecibels)
{
	if (DecibelsText)
	{
		DecibelsText->SetText(FText::FromString(FString::Printf(TEXT("%d"), static_cast<uint32>(CurrentDecibels))));	
	}
}
