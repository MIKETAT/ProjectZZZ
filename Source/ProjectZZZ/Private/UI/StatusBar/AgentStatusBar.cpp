#include "UI/StatusBar/AgentStatusBar.h"

#include "Character/Component/SquadManagerComponent.h"
#include "UI/StatusBar/AgentHead.h"
#include "UI/StatusBar/StatProgressBar.h"

void UAgentStatusBar::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeAgentStatusBar();
}

void UAgentStatusBar::BindDelegate(const FHUDSquadAgentSource& Source)
{
	UnBindDelegate();
	
	if (Source.CombatComponent.IsValid()) {
		CombatComponent = Source.CombatComponent;
		HealthChangedDelegate = Source.CombatComponent->OnHealthChanged.AddUObject(this, &ThisClass::OnUpdateHealth);
		EnergyChangedDelegate = Source.CombatComponent->OnEnergyChanged.AddUObject(this, &ThisClass::OnUpdateEnergy);
		DecibelsChangedDelegate = Source.CombatComponent->OnDecibelsChanged.AddUObject(this, &ThisClass::OnUpdateDecibel);
	}
}

void UAgentStatusBar::UnBindDelegate()
{
	if (CombatComponent.IsValid())
	{
		if (HealthChangedDelegate.IsValid())
		{
			CombatComponent->OnHealthChanged.Remove(HealthChangedDelegate);
		}
		
		if (EnergyChangedDelegate.IsValid())
		{
			CombatComponent->OnEnergyChanged.Remove(EnergyChangedDelegate);
		}

		if (DecibelsChangedDelegate.IsValid())
		{
			CombatComponent->OnDecibelsChanged.Remove(DecibelsChangedDelegate);
		}
	}
}

void UAgentStatusBar::InitializeAgentStatusBar()
{
	if (AgentHealthBar && HealthTex)
	{
		AgentHealthBar->InitializeProgressBarFillImage(HealthTex);
	}

	if (AgentEnergyBar && EnergyTex)
	{
		AgentEnergyBar->InitializeProgressBarFillImage(EnergyTex);
	}

	if (AgentDecibelsBar && DecibelsTex)
	{
		AgentDecibelsBar->InitializeProgressBarFillImage(DecibelsTex);
	}
}

void UAgentStatusBar::RefreshAgentStatus(const FAgentStatusSnapShot& SnapShot)
{
	if (!SnapShot.Agent.IsValid())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	OnUpdateAgentHead(SnapShot);
	OnUpdateHealth(SnapShot.CurrentHealth, SnapShot.MaxHealth);
	OnUpdateEnergy(SnapShot.CurrentEnergy, SnapShot.MaxEnergy);
	OnUpdateDecibel(SnapShot.CurrentDecibels, SnapShot.MaxDecibels);
}

void UAgentStatusBar::OnUpdateAgentHead(const FAgentStatusSnapShot& SnapShot)
{
	if (AgentHead)
	{
		AgentHead->RefreshAgentHead(SnapShot);
	}
}

void UAgentStatusBar::OnUpdateHealth(float CurrentHealth, float MaxHealth)
{
	if (AgentHealthBar)
	{
		AgentHealthBar->SetPercentage(FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f));
	}
}

void UAgentStatusBar::OnUpdateEnergy(float CurrentEnergy, float MaxEnergy)
{
	if (AgentEnergyBar)
	{
		AgentEnergyBar->SetPercentage(FMath::Clamp(CurrentEnergy / MaxEnergy, 0.f, 1.f));
	}
}

void UAgentStatusBar::OnUpdateDecibel(float CurrentDecibel, float MaxDecibel)
{
	if (AgentDecibelsBar)
	{
		AgentDecibelsBar->SetPercentage(FMath::Clamp(CurrentDecibel / MaxDecibel, 0.f, 1.f));	
	}
}
