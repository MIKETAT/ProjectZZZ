// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Component/HitStopComponent.h"

UHitStopComponent::UHitStopComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
}

void UHitStopComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHitStopComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bHitStopActive)
	{
		UWorld* World = GetWorld();
		if (World && World->GetRealTimeSeconds() >= HitStopEndTime)
		{
			ClearHitStop();
		}
	}
}

void UHitStopComponent::ApplyHitStop(const float Duration, const float TimeScale)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || Duration <= 0.f)
	{
		return;	
	}

	float CurrentRealTime = World->GetRealTimeSeconds();
	float TargetEndTime = CurrentRealTime + Duration;

	// override previous HitStop Settings
	if (bHitStopActive)
	{
		if (TargetEndTime > HitStopEndTime)
		{
			HitStopEndTime = TargetEndTime;
			Owner->CustomTimeDilation = TimeScale;
		}
	} else
	{
		OriginTimeDilation = Owner->CustomTimeDilation;
		bHitStopActive = true;
		HitStopEndTime = TargetEndTime;
		Owner->CustomTimeDilation = TimeScale;
	}
}

void UHitStopComponent::ClearHitStop()
{
	if (bHitStopActive)
	{
		bHitStopActive = false;
		if (AActor* Owner = GetOwner())
		{
			Owner->CustomTimeDilation = OriginTimeDilation;
		}
	}
}

