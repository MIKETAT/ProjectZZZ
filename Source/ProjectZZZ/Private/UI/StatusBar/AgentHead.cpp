#include "UI/StatusBar/AgentHead.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Components/Image.h"

void UAgentHead::BindDelegate(const FHUDSquadSource& Source)
{
	if (!Source.SquadManager.IsValid())
	{
		return;
	}

	UnBindDelegate();

	SquadManager = Source.SquadManager.Get();
	ActiveAgentChangedDelegate = Source.SquadManager->OnActiveAgentChanged.AddUObject(this, &ThisClass::HandleActiveAgentChanged);
}

void UAgentHead::UnBindDelegate()
{
	if (SquadManager.IsValid())
	{
		if (ActiveAgentChangedDelegate.IsValid())
		{
			SquadManager->OnActiveAgentChanged.Remove(ActiveAgentChangedDelegate);
		}
	}
}

void UAgentHead::RefreshAgentHead(const FAgentStatusSnapShot& SnapShot)
{
	if (SnapShot.Agent.IsValid())
	{
		ApplyAgentTexture(SnapShot.AgentHead.Get());
	}
}

void UAgentHead::ApplyAgentTexture(UTexture2D* Texture)
{
	if (!AgentHeadMaterial || !AgentHead || !Texture)
	{
		return;
	}

	if (!AgentHeadMID)
	{
		AgentHeadMID = UMaterialInstanceDynamic::Create(AgentHeadMaterial, this);
		AgentHead->SetBrushFromMaterial(AgentHeadMID);
	}

	AgentHeadMID->SetTextureParameterValue(BrushTextureParameterName, Texture);
}

void UAgentHead::HandleActiveAgentChanged(APlayerCharacter* OldAgent, APlayerCharacter* NewAgent)
{
	if (NewAgent)
	{
		ApplyAgentTexture(NewAgent->GetAgentHead());
	}
}
