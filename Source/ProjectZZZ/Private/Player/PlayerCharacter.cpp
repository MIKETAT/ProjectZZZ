#include "Player/PlayerCharacter.h"
#include "AbilitySystem/AgentAttributeSet.h"
#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "Utility/ZZZGameplayTag.h"

void FPendingUltimateCutInRequest::Reset()
{
	Agent = nullptr;
	UltimateAction = nullptr;
	CutInSequence = nullptr;
	bIsValid = false;
}

void FActiveUltimateExecutionState::Reset()
{
	Agent = nullptr;
	SequencePlayer = nullptr;
	SequenceActor = nullptr;
	bIsValid = false;
	bSequenceFinished = false;
	bActionFinished = false;
	bAborting = false;
}

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GameplayCamera = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("GameplayCamera"));
	GameplayCamera->SetupAttachment(RootComponent);
	
	AgentAttributeSet = CreateDefaultSubobject<UAgentAttributeSet>(TEXT("AgentAttributeSet"));
	AgentCombatComponent = CreateDefaultSubobject<UCharacterCombatComponent>(TEXT("CombatComponent"));
	CombatBase = AgentCombatComponent;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>()) {
		FCombatEventDelegate Callback;
		Callback.BindUObject(this, &APlayerCharacter::HandleAgentDeath);
		DeathListenerHandle = EventBus->Subscribe(Combat::Event::Death, this,10, Callback);
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	bIsLocalPlayer = NewController->IsLocalPlayerController();
	OwnerController = Cast<AZZZPlayerController>(GetController());

	if (AgentAbilitySystemComponent)
	{
		AgentAbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	
	if (AgentCombatComponent)
	{
		AgentCombatComponent->InjectAndBindASC(AgentAbilitySystemComponent);
	}
}

void APlayerCharacter::UnPossessed()
{
	Super::UnPossessed();
	bIsLocalPlayer = false;
	OwnerController = nullptr;
}

void APlayerCharacter::InitializeAttributes()
{
	ApplyGameplayEffectToSelf(BaseInitGE);
	ApplyGameplayEffectToSelf(AgentExclusiveInitGE);
}

bool APlayerCharacter::IsMoving() const
{
	return  GetCharacterMovement() && !GetCharacterMovement()->Velocity.IsNearlyZero();
}

ECombatEventHandleResult APlayerCharacter::HandleAgentDeath(const FCombatEventMessage& Msg)
{
	return ECombatEventHandleResult::Consumed;
}

void APlayerCharacter::SwitchToOnField()
{
	SetAgentPresence(EAgentPresenceState::Active);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
}

void APlayerCharacter::SwitchToOffField()
{
	if (Controller)
	{
		Controller->UnPossess();
	}
	
	SetAgentPresence(EAgentPresenceState::OffField);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

AEnemyCharacterBase* APlayerCharacter::FindClosestEnemy(const float MaxDistance) const
{
	if (AgentCombatComponent)
	{
		return AgentCombatComponent->FindClosestEnemy(MaxDistance);
	}
	return nullptr;
}

void APlayerCharacter::ProcessFrameInput(const FCharacterFrameDataBus& DataBus)
{
	if (AgentPresenceState != EAgentPresenceState::Active)
	{
		return;
	}
	ProcessMovementInput(DataBus);
	ProcessLookInput(DataBus);
	ProcessCombatActionInput(DataBus);
}

FAgentStatusSnapShot APlayerCharacter::BuildAgentStatusSnapshot()
{
	FAgentStatusSnapShot SnapShot;
	
	SnapShot.CurrentHealth = BaseCombatAttribute->GetHealth();
	SnapShot.MaxHealth = BaseCombatAttribute->GetMaxHealth();
	SnapShot.CurrentEnergy = AgentAttributeSet->GetEnergy();
	SnapShot.MaxEnergy = AgentAttributeSet->GetMaxEnergy();
	SnapShot.CurrentDecibels = AgentAttributeSet->GetDecibels();
	SnapShot.MaxDecibels = AgentAttributeSet->GetMaxDecibels();
	SnapShot.bCanExecuteSpecialAttackEX = AgentCombatComponent ? AgentCombatComponent->MeetsActionRequirements(GetSpecialAction(Combat::SpecialAction::SpecialAttackEX)) : false;
	SnapShot.bCanExecuteUltimate = AgentCombatComponent ? AgentCombatComponent->MeetsActionRequirements(GetSpecialAction(Combat::SpecialAction::Ultimate)) : false;

	SnapShot.Agent = this;
	SnapShot.AgentHead = GetAgentHead();
	
	return SnapShot;
}

void APlayerCharacter::ProcessMovementInput(const FCharacterFrameDataBus& DataBus)
{
	if (!bIsLocalPlayer || !DataBus.HasMovementInput())
	{
		return;
	}

	if (AgentCombatComponent && AgentCombatComponent->IsAnyActionActive())
	{
		if (AgentCombatComponent->IsAllowMovementInterruptAction())
		{
			AgentCombatComponent->CancelCurrentAction();
			return;
		}
		// Block Movement while Action Active
		return;
	}
	
	float Right = DataBus.PlayerInputs.RawMovementInput.X;
	float Forward = DataBus.PlayerInputs.RawMovementInput.Y;
	
	const FRotator Rotation = GetController()->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	AddMovementInput(ForwardDirection, Forward);
	AddMovementInput(RightDirection, Right);
}

void APlayerCharacter::ProcessLookInput(const FCharacterFrameDataBus& DataBus)
{
	if (!bIsLocalPlayer || DataBus.PlayerInputs.RawLookInput.IsNearlyZero())
	{
		return;
	}
	if (GetController())
	{
		AddControllerYawInput(DataBus.PlayerInputs.RawLookInput.X);
		AddControllerPitchInput(DataBus.PlayerInputs.RawLookInput.Y);	
	}
}

void APlayerCharacter::ProcessCombatActionInput(const FCharacterFrameDataBus& DataBus)
{
	if (AgentCombatComponent.Get())
	{
		AgentCombatComponent->ProcessFrameInput(DataBus.PlayerInputs);
	}
}

