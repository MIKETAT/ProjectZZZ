#include "Character/Component/SquadManagerComponent.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerCharacter.h"

USquadManagerComponent::USquadManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void USquadManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeAgentSquad();
}

void USquadManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (OwnerController.IsValid() && OwnerController->GetPlayerInputHandlerComponent())
	{
		RouteInput(OwnerController->GetPlayerInputHandlerComponent()->GetCharacterFrameDataBus());	
	}
}

void USquadManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
	OwnerController = Cast<AZZZPlayerController>(GetOwner());
}

APlayerCharacter* USquadManagerComponent::GetActivePlayerCharacter() const
{
	if (Squad.IsValidIndex(ActiveAgentIndex))
	{
		return Squad[ActiveAgentIndex];
	}
	
	return nullptr;
}

void USquadManagerComponent::SwitchToPreviousAgent()
{
	SwitchToAgent(GetPreviousAgentIndex());
}

void USquadManagerComponent::SwitchToNextAgent()
{
	SwitchToAgent(GetNextAgentIndex());
}

void USquadManagerComponent::SwitchToAgent(const int32 TargetIndex)
{
	if (TargetIndex != INDEX_NONE && Squad.IsValidIndex(TargetIndex))
	{
		AgentSwapImplementation(TargetIndex);
	}
}


void USquadManagerComponent::InitializeAgentSquad()
{
	if (!OwnerController.IsValid() || SquadPreset.IsEmpty())
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	Squad.Empty();

	AActor* StartSpot{nullptr};
	if (AGameModeBase* GameMode = World->GetAuthGameMode())
	{
		StartSpot = GameMode->ChoosePlayerStart(OwnerController.Get());
	}
	FTransform SpawnTransform{StartSpot ? StartSpot->GetActorTransform() : FTransform::Identity};
	
	for (TSubclassOf AgentClass : SquadPreset)
	{
		if (AgentClass)
		{
			APlayerCharacter* NewAgent = World->SpawnActorDeferred<APlayerCharacter>(
				AgentClass,
				SpawnTransform,
				GetOwner(),
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			if (NewAgent)
			{
				NewAgent->SetActorHiddenInGame(true);
				NewAgent->SetActorEnableCollision(false);

				UGameplayStatics::FinishSpawningActor(NewAgent, SpawnTransform);

				Squad.Add(NewAgent);
			}
		}
	}
	AgentSwapImplementation(0);
}

void USquadManagerComponent::AgentSwapImplementation(const int32 AgentIndex)
{
	if (!Squad.IsValidIndex(AgentIndex) || !OwnerController.IsValid())
	{
		return;
	}

	APlayerCharacter* CurrentAgent{GetActivePlayerCharacter()};
	APlayerCharacter* TargetAgent{Squad[AgentIndex]};
	FVector InheritedVelocity{FVector::ZeroVector};
	FRotator InheritedRotation{FRotator::ZeroRotator};
	bool bCurrentAgentMoving{false};
	
	// Switch out Current Agent
	if (CurrentAgent)
	{
		OwnerController->UnPossess();
		// AI Controller?
		
		CurrentAgent->SetAgentPresence(EAgentPresenceState::Lingering);

		InheritedVelocity = CurrentAgent->GetCharacterMovement()->Velocity;
		InheritedRotation = CurrentAgent->GetActorRotation();
		bCurrentAgentMoving = !InheritedVelocity.IsNearlyZero();
		
		// If CurrentAgent has unfinished action. Continue until it's finished.
		UCharacterCombatComponent* CurrentAgentCombatComp = CurrentAgent->GetCombatComponent();
		checkf(CurrentAgentCombatComp, TEXT("Switch Out Agent CombatComponent Invalid"));
		if (!CurrentAgentCombatComp->IsAnyActionActive())
		{
			// If CurrentAgent is not play any animation/action. Play Switch Out Animation then go OffField.
			CurrentAgentCombatComp->ExecuteSwitchOutAction();
		}
	}

	// Switch In Target Agent
	if (TargetAgent)
	{
		FVector SpawnOffset = CurrentAgent ? CurrentAgent->GetActorRightVector() * 100.f : FVector::ZeroVector;
		FTransform SpawnTransform = CurrentAgent ? CurrentAgent->GetTransform() : FTransform::Identity;
		SpawnTransform.AddToTranslation(SpawnOffset);

		TargetAgent->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		
		TargetAgent->SwitchToOnField();
		
		OwnerController->Possess(TargetAgent);

		UCharacterCombatComponent* TargetCombatComp = TargetAgent->GetCombatComponent();
		checkf(TargetCombatComp, TEXT("Switch In Agent CombatComponent Invalid"));
		
		if (OwnerController->HasMovementInput() && bCurrentAgentMoving)
		{
			TargetAgent->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			TargetAgent->GetCharacterMovement()->Velocity = InheritedVelocity;
			TargetAgent->SetActorRotation(InheritedRotation);
		} else
		{
			TargetCombatComp->ExecuteSwitchInAction();
		}
		
		ActiveAgentIndex = AgentIndex;
	}
}

int32 USquadManagerComponent::GetPreviousAgentIndex() const
{
	const int32 AgentNums{Squad.Num()};
	
	if (AgentNums <= 0 || !Squad.IsValidIndex(ActiveAgentIndex))
	{
		return INDEX_NONE;
	}

	if (AgentNums == 1)
	{
		return ActiveAgentIndex;
	}
	int32 TargetIndex{(ActiveAgentIndex - 1 + AgentNums) % AgentNums};
	return Squad.IsValidIndex(TargetIndex) ? TargetIndex : INDEX_NONE; 
}

int32 USquadManagerComponent::GetNextAgentIndex() const
{
	const int32 AgentNums{Squad.Num()};
	if (AgentNums <= 0 || !Squad.IsValidIndex(ActiveAgentIndex))
	{
		return INDEX_NONE;
	}

	if (AgentNums == 1)
	{
		return ActiveAgentIndex;
	}

	const int32 TargetIndex{(ActiveAgentIndex + 1) % AgentNums};
	return Squad.IsValidIndex(TargetIndex) ? TargetIndex : INDEX_NONE; 
}

void USquadManagerComponent::RouteInput(FCharacterFrameDataBus& DataBus)
{
	if (!SquadConsumeInput(DataBus))
	{
		AgentConsumeInput(DataBus);
	}	
}

bool USquadManagerComponent::SquadConsumeInput(FCharacterFrameDataBus& DataBus)
{
	// Is Chain Attack Window Open
	
	// Is Quick Assist Activated

	// Try to Switch Agent
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Previous))
	{
		SwitchToPreviousAgent();
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Previous);
		return true;
	}
	if (DataBus.PlayerInputs.InputActionBitmask.Test(EInputAction::EInputActionFlag_SwitchCharacter_Next))
	{
		SwitchToNextAgent();
		DataBus.PlayerInputs.ConsumeInputAction(EInputAction::EInputActionFlag_SwitchCharacter_Next);
		return true;
	}
	return false;
}

void USquadManagerComponent::AgentConsumeInput(FCharacterFrameDataBus& DataBus)
{
	GetActivePlayerCharacter()->RefreshCharacterFrameInputData(DataBus);
}
