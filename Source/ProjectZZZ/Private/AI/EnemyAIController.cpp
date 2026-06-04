#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerCharacter.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTree)
	{
		UBlackboardComponent* BlackBoard;
		if (UseBlackboard(BehaviorTree->BlackboardAsset, BlackBoard))
		{
			RunBehaviorTree(BehaviorTree);
			
			if (AZZZPlayerController* PC = Cast<AZZZPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
			{
				if (USquadManagerComponent* SquadManager = PC->GetSquadManagerComponent())
				{
					Blackboard->SetValueAsObject(FName("TargetActor"), SquadManager->GetActiveAgent());
					SquadManager->OnActiveAgentChanged.AddUObject(this, &AEnemyAIController::HandlePlayerAgentChanged);
				}
			}
		} else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed To Initialize BlackBoard"));
		}
	}
}

void AEnemyAIController::SetBBBool(const FGameplayTag& Key, const bool Value)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(Key.GetTagName(), Value);
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed To Setting BlackBoard Value"));
	}
	
}

void AEnemyAIController::HandlePlayerAgentChanged(APlayerCharacter* OldAgent, APlayerCharacter* NewAgent)
{
	if (NewAgent && GetBlackboardComponent())
	{
		GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), NewAgent);
	}
}
