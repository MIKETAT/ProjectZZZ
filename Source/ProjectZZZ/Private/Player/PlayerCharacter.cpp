// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerCharacter.h"

#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/PlayerInputHandlerComponent.h"

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
	
	// Input Handler
	PlayerInputHandlerComponent = CreateDefaultSubobject<UPlayerInputHandlerComponent>(TEXT("InputHandlerComponent"));
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (CharacterFrameDataBus.bIsLocalPlayer)
	{
		PlayerInputHandlerComponent->BuildCharacterFrameDataBus(CharacterFrameDataBus);
		ProcessMovementInput(DeltaTime);
		ProcessLookInput(DeltaTime);
		ProcessCombatActionInput(DeltaTime);
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void APlayerCharacter::ProcessMovementInput(float DeltaTime)
{
	if (!CharacterFrameDataBus.bIsLocalPlayer || CharacterFrameDataBus.RawMovementInput.IsNearlyZero())
	{
		return;
	}

	check(CombatComponent);

	if (CombatComponent->IsAllowMovementInterruptAction())
	{
		CombatComponent->CancelCurrentAction();
		return;
	}	

	
	float Right = CharacterFrameDataBus.RawMovementInput.X;
	float Forward = CharacterFrameDataBus.RawMovementInput.Y;
	
	const FRotator Rotation = GetController()->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, Forward);
	AddMovementInput(RightDirection, Right);
}

void APlayerCharacter::ProcessLookInput(float DeltaTime)
{
	if (!CharacterFrameDataBus.bIsLocalPlayer || CharacterFrameDataBus.RawLookInput.IsNearlyZero())
	{
		return;
	}
	if (GetController())
	{
		AddControllerYawInput(CharacterFrameDataBus.RawLookInput.X);
		AddControllerPitchInput(CharacterFrameDataBus.RawLookInput.Y);	
	}
}

void APlayerCharacter::ProcessCombatActionInput(float DeltaTime)
{
	if (CombatComponent.Get())
	{
		CombatComponent->InputActionBitmask = CharacterFrameDataBus.InputActionBitmask; 
	}
}

