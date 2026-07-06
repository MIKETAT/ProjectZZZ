#include "UI/StatusBar/ActiveAgentStatusBar.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Components/ProgressBar.h"
#include "UI/StatusBar/StatProgressBar.h"

void UActiveAgentStatusBar::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeActiveAgentStatusBar();
}

void UActiveAgentStatusBar::InitializeActiveAgentStatusBar()
{
	if (HealthBar && HealthTex && HealthBarMaterial)
	{
		if (!HealthBarBackgroundMID)
		{
			HealthBarBackgroundMID = UMaterialInstanceDynamic::Create(HealthBarMaterial, this);
		}

		if (!HealthBarFillMID)
		{
			HealthBarFillMID = UMaterialInstanceDynamic::Create(HealthBarMaterial, this);
		}
		
		FProgressBarStyle Style{HealthBar->GetWidgetStyle()};
		if (HealthBarBackgroundMID)
		{
			FSlateBrush BackgroundBrush{Style.BackgroundImage};
			HealthBarBackgroundMID->SetTextureParameterValue(HealthBarMaskParameterName, HealthBarMask);
			HealthBarBackgroundMID->SetVectorParameterValue(HealthBarColorVectorParameterName, ProgressBarBackgroundColor);
			BackgroundBrush.SetResourceObject(HealthBarBackgroundMID);
			BackgroundBrush.DrawAs = ESlateBrushDrawType::Type::Image;
			Style.BackgroundImage = BackgroundBrush;
		}

		if (HealthBarFillMID)
		{
			FSlateBrush FillBrush{Style.BackgroundImage};
			HealthBarFillMID->SetTextureParameterValue(HealthBarMaskParameterName, HealthBarMask);
			HealthBarFillMID->SetScalarParameterValue(HealthBarEnableTexScalarParameterName, 1.f);
			HealthBarFillMID->SetTextureParameterValue(HealthBarTextureParameterName, HealthTex);
			FillBrush.SetResourceObject(HealthBarFillMID);
			FillBrush.DrawAs = ESlateBrushDrawType::Type::Image;
			Style.FillImage = FillBrush;
		}

		HealthBar->SetWidgetStyle(Style);
	}

	if (EnergyBar && EnergyTex)
	{
		EnergyBar->InitializeProgressBarFillImage(EnergyTex);
	}
}

void UActiveAgentStatusBar::BindDelegate(const FHUDSquadSource& Source)
{
	UnBindDelegate();
		
	if (Source.ActiveAgent.CombatComponent.IsValid())
	{
		CombatComponent = Source.ActiveAgent.CombatComponent;
		HealthChangedDelegate = Source.ActiveAgent.CombatComponent->OnHealthChanged.AddUObject(this, &ThisClass::OnUpdateHealthChanged);
		EnergyChangedDelegate = Source.ActiveAgent.CombatComponent->OnEnergyChanged.AddUObject(this, &ThisClass::OnupdateHandleEnergyChanged);
		DecibelsChangedDelegate = Source.ActiveAgent.CombatComponent->OnDecibelsChanged.AddUObject(this, &ThisClass::OnUpdateDecibelsChanged);
	}
}

void UActiveAgentStatusBar::UnBindDelegate()
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

void UActiveAgentStatusBar::RefreshActiveAgentStatus(const FAgentStatusSnapShot& SnapShot)
{
	OnUpdateHealthChanged(SnapShot.CurrentHealth, SnapShot.MaxHealth);
	OnupdateHandleEnergyChanged(SnapShot.CurrentEnergy, SnapShot.MaxEnergy);
}

void UActiveAgentStatusBar::OnUpdateHealthChanged(float CurrentHealth, float MaxHealth)
{
	HealthBar->SetPercent(FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f));
}

void UActiveAgentStatusBar::OnupdateHandleEnergyChanged(float CurrentEnergy, float MaxEnergy)
{
	EnergyBar->SetPercentage(FMath::Clamp(CurrentEnergy / MaxEnergy, 0.f, 1.f));
}

void UActiveAgentStatusBar::OnUpdateDecibelsChanged(float CurrentDecibels, float MaxDecibels)
{
	
}
