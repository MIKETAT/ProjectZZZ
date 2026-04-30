// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerCharacter.h"

#include "AbilitySystem/AgentAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Character/ZZZPlayerController.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "Utility/ZZZGameplayTag.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	AgentAttributeSet = CreateDefaultSubobject<UAgentAttributeSet>(TEXT("AgentAttributeSet"));
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Test
	if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>()) {
		FCombatEventDelegate Callback;
		Callback.BindUObject(this, &APlayerCharacter::HandleEnemyDeath);
		DeathListenerHandle = EventBus->Subscribe(Combat::Event::Death, this,10, Callback);
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	ProcessMovementInput(DeltaTime);
	ProcessLookInput(DeltaTime);
	ProcessCombatActionInput(DeltaTime);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	CharacterFrameDataBus.bIsLocalPlayer = NewController->IsLocalPlayerController();
	OwnerController = Cast<AZZZPlayerController>(GetController());
}

void APlayerCharacter::UnPossessed()
{
	Super::UnPossessed();
	CharacterFrameDataBus.bIsLocalPlayer = false;
	OwnerController = nullptr;
}

ECombatEventHandleResult APlayerCharacter::HandleEnemyDeath(const FCombatEventMessage& Msg)
{
	UE_LOG(LogTemp, Error, TEXT("PlayCharacter now heard Enemy Death Msg, EventTag = %s"), *Msg.EventTag.ToString());
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

void APlayerCharacter::ProcessMovementInput(float DeltaTime)
{
	if (!CharacterFrameDataBus.bIsLocalPlayer || !CharacterFrameDataBus.HasMovementInput())
	{
		return;
	}

	check(CombatComponent);

	if (CombatComponent->IsAllowMovementInterruptAction())
	{
		CombatComponent->CancelCurrentAction();
		return;
	}	
	
	float Right = CharacterFrameDataBus.PlayerInputs.RawMovementInput.X;
	float Forward = CharacterFrameDataBus.PlayerInputs.RawMovementInput.Y;
	
	const FRotator Rotation = GetController()->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, Forward);
	AddMovementInput(RightDirection, Right);
}

void APlayerCharacter::ProcessLookInput(float DeltaTime)
{
	if (!CharacterFrameDataBus.bIsLocalPlayer || CharacterFrameDataBus.PlayerInputs.RawLookInput.IsNearlyZero())
	{
		return;
	}
	if (GetController())
	{
		AddControllerYawInput(CharacterFrameDataBus.PlayerInputs.RawLookInput.X);
		AddControllerPitchInput(CharacterFrameDataBus.PlayerInputs.RawLookInput.Y);	
	}
}

void APlayerCharacter::ProcessCombatActionInput(float DeltaTime)
{
	if (CombatComponent.Get())
	{
		CombatComponent->InputActionBitmask = CharacterFrameDataBus.PlayerInputs.InputActionBitmask; 
	}
}

