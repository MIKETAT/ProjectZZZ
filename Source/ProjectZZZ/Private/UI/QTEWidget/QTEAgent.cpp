// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/QTEWidget/QTEAgent.h"

#include "Components/Image.h"

void UQTEAgent::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (KeyIcon && KeyPromptIcon)
	{
		KeyIcon->SetBrushFromTexture(KeyPromptIcon);
	}
}

void UQTEAgent::SetAgent(UTexture2D* AgentHead)
{
	if (Agent)
	{
		Agent->SetBrushFromTexture(AgentHead);
	}

}
