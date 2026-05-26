// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class APlayerCharacter;

UCLASS()
class PROJECTZZZ_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEnemyAIController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnPossess(APawn* InPawn) override;

private:
	void HandlePlayerAgentChanged(APlayerCharacter* OldAgent, APlayerCharacter* NewAgent);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UBehaviorTree> BehaviorTree;

	/*UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAIPerceptionComponent> PerceptionComponent;
	*/
};
