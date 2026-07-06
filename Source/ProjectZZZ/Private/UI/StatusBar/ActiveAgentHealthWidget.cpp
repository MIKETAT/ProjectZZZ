#include "UI/StatusBar/ActiveAgentHealthWidget.h"
#include "Components/TextBlock.h"
#include "Character/Component/SquadManagerComponent.h"

void UActiveAgentHealthWidget::BindDelegates(const FHUDSquadAgentSource& Source)
{
	if (!Source.CombatComponent.IsValid())
	{
		return;
	}

	UnBindDelegate();

	CombatComponent = Source.CombatComponent;
	HealthChangedDelegateHandle = Source.CombatComponent->OnHealthChanged.AddUObject(this, &ThisClass::OnUpdateHealthChanged);
}

void UActiveAgentHealthWidget::UnBindDelegate()
{
	if (CombatComponent.IsValid() && HealthChangedDelegateHandle.IsValid())
	{
		CombatComponent->OnHealthChanged.Remove(HealthChangedDelegateHandle);
	}
}

void UActiveAgentHealthWidget::RefreshActiveAgentHealthWidget(const FAgentStatusSnapShot& Snapshot)
{
	OnUpdateHealthChanged(Snapshot.CurrentHealth, Snapshot.MaxHealth);
}

void UActiveAgentHealthWidget::OnUpdateHealthChanged(float CurrentHealth, float MaxHealth)
{
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), static_cast<uint32>(CurrentHealth), static_cast<uint32>(MaxHealth))));	
	}
}
