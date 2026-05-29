// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotifyState/AnimNotifyState_PerfectAssistWindow.h"

#include "AI/EnemyCharacterBase.h"
#include "AI/EnemyCombatComponent.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Combat/ZZZCombatEventTypes.h"
#include "Utility/ZZZGameplayTag.h"

void UAnimNotifyState_PerfectAssistWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                       float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(MeshComp->GetOwner()))
	{
		Enemy->BP_OnParryWindowOpened();
	}
	BroadcastPerfectAssistWindowStateChanged(true, MeshComp->GetOwner());
}

void UAnimNotifyState_PerfectAssistWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	BroadcastPerfectAssistWindowStateChanged(false, MeshComp->GetOwner());
}

#if WITH_EDITOR
void UAnimNotifyState_PerfectAssistWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	FTransform SocketWorldTransform{MeshComp->GetSocketTransform(SocketName, RTS_World)};
	FTransform RootWorldTransform{MeshComp->GetComponentTransform()};

//	FVector WorldOffset{SocketWorldTransform.GetLocation() - RootWorldTransform.GetLocation()};
	FVector WorldOffset{SocketWorldTransform.GetLocation()};
	
	FVector Forward{RootWorldTransform.GetRotation().GetForwardVector()};
	FVector Right{RootWorldTransform.GetRotation().GetRightVector()};
	FVector Up{RootWorldTransform.GetRotation().GetUpVector()};
	
	IdealParryOffset.X = FVector::DotProduct(WorldOffset, Forward);
	IdealParryOffset.Y = FVector::DotProduct(WorldOffset, Right);
	IdealParryOffset.Z = FVector::DotProduct(WorldOffset, Up);
	
	float Length = 150.f;
	
	DrawDebugLine(
		  MeshComp->GetWorld(),
		  RootWorldTransform.GetLocation(),
		  RootWorldTransform.GetLocation()
		  + Forward * Length,
		  FColor::Green);

	DrawDebugLine(
		MeshComp->GetWorld(),
		RootWorldTransform.GetLocation(),
		RootWorldTransform.GetLocation()
		+ Right * Length,
		FColor::Red);

	DrawDebugLine(
		MeshComp->GetWorld(),
		RootWorldTransform.GetLocation(),
		RootWorldTransform.GetLocation()
		+ Up * Length,
		FColor::Blue);
}
#endif

void UAnimNotifyState_PerfectAssistWindow::BroadcastPerfectAssistWindowStateChanged(const bool bWindowOpen, AActor* Owner)
{
	if (!IsValid(Owner))
	{
		return;
	}
	
	UWorld* World = Owner->GetWorld();

	if (!IsValid(World) || !World->IsGameWorld())
	{
		return;
	}

	UCombatEventBusSubSystem* EventBus = World->GetSubsystem<UCombatEventBusSubSystem>();
	if (!IsValid(EventBus))
	{
		return;
	}

	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Owner);
	if (!IsValid(Enemy))
	{
		return;
	}
	
	if (UEnemyCombatComponent* CombatComponent = Enemy->GetEnemyCombatComponent())
	{
		FPerfectAssistStatePayload Payload;
		Payload.bWindowOpen = bWindowOpen;
		Payload.ParryReferenceOffset = CombatComponent->GetCurrentActionParryOffset();
		
		EventBus->BroadcastEvent<FPerfectAssistStatePayload>(
			Combat::Event::PerfectAssist,
			Owner,
			nullptr,
			Owner,
			Payload);
	}
}
