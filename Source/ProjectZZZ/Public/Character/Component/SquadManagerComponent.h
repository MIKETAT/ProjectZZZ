// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SquadManagerComponent.generated.h"


struct FCharacterFrameDataBus;
class AZZZPlayerController;
class APlayerCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API USquadManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USquadManagerComponent();
	
protected:
	virtual void BeginPlay() override;
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void InitializeComponent() override;
public:
	// Getter and Setter
	APlayerCharacter* GetActivePlayerCharacter() const;
	
	// Switch Agent
	// Todo: 切换到后台的代理人动作只执行到逻辑结算结束就退到后台，不播放收刀之类的后摇动画
	// Todo: 运动状态下切换代理人, 应继承原运动状态(动画表现)
	void SwitchToPreviousAgent();
	
	void SwitchToNextAgent();
	
	void SwitchToAgent(const int32 TargetIndex);

private:
	// Squad 
	void InitializeAgentSquad();

	void AgentSwapImplementation(const int32 AgentIndex = 0);
	
	int32 GetPreviousAgentIndex() const;
	
	int32 GetNextAgentIndex() const;

	// Input
	void RouteInput(FCharacterFrameDataBus& DataBus);

	bool SquadConsumeInput(FCharacterFrameDataBus& DataBus);
	
	void AgentConsumeInput(FCharacterFrameDataBus& DataBus);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<APlayerCharacter>> SquadPreset;
	
private:
	UPROPERTY()
	TArray<APlayerCharacter*> Squad;

	UPROPERTY()
	TWeakObjectPtr<AZZZPlayerController> OwnerController{nullptr};

	int32 ActiveAgentIndex{INDEX_NONE};
};
