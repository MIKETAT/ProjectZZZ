// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/PlayerInputHandlerComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/CharacterBase.h"
#include "Character/CharacterFrameDataBus.h"

UPlayerInputHandlerComponent::UPlayerInputHandlerComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerInputHandlerComponent::BeginPlay()
{
	Super::BeginPlay();
	// update PlayerCharacter here for now, fix this later
	if (!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<ACharacterBase>(GetOwner());
	}
	RegisterInput();
}

void UPlayerInputHandlerComponent::BuildCharacterFrameDataBus(FCharacterFrameDataBus& DataBus)
{
	DataBus.InputActionBitmask = InputActionBitmask;
	DataBus.RawMovementInput = RawInputMovementVector;
	DataBus.RawLookInput = RawInputLookVector;
}

void UPlayerInputHandlerComponent::RegisterInput()
{
	checkf(IsValid(PlayerCharacter), TEXT("Register Input but PlayerCharacter is Invalid"));
	UEnhancedInputLocalPlayerSubsystem* InputSubSystem{nullptr};
	if (const APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController()))
	{
		EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
		if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			InputSubSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		}
	}

	if (IsValid(InputSubSystem) && IsValid(DefaultInputMappingContext))
	{
		InputSubSystem->AddMappingContext(DefaultInputMappingContext, DefaultInputMappingContextPriority);
	}

	// Bind Action Macro
	if (IsValid(EnhancedInputComponent))
	{
#define BIND_ALL_TRIGGER_STATE(InputComponent, IA_Action, CallbackFunc) \
		{ \
			if (IA_Action) \
			{ \
				InputComponent->BindAction(IA_Action, ETriggerEvent::Started, this, &ThisClass::CallbackFunc); \
				InputComponent->BindAction(IA_Action, ETriggerEvent::Triggered, this, &ThisClass::CallbackFunc); \
				InputComponent->BindAction(IA_Action, ETriggerEvent::Completed, this, &ThisClass::CallbackFunc); \
				InputComponent->BindAction(IA_Action, ETriggerEvent::Canceled, this, &ThisClass::CallbackFunc); \
			} \
		} 

	// Bind Action
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, Movement_Action, On_Input_Movement);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, Look_Action, On_Input_Look);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, Dodge_Action, On_Input_Dodge);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, Basic_Attack_Action, On_Input_Basic_Attack);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, Special_Attack_Action, On_Input_Special_Attack);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, Ultimate_Action, On_Input_Ultimate);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, SwitchCharacter_Previous_Action, On_Input_SwitchCharacter_Previous);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, SwitchCharacter_Next_Action, On_Input_SwitchCharacter_Next);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, ChainAttack_Left_Action, On_Input_ChainAttack_Left);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, ChainAttack_Right_Action, On_Input_ChainAttack_Right);
		BIND_ALL_TRIGGER_STATE(EnhancedInputComponent, ChainAttack_Cancel_Action, On_Input_ChainAttack_Cancel);
#undef BIND_ALL_TRIGGER_STATE
	}
}

void UPlayerInputHandlerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPlayerInputHandlerComponent::InitializeComponent()
{
	Super::InitializeComponent();
	PlayerCharacter = Cast<ACharacterBase>(GetOwner());
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerCharacter is null"));
	}
}

void UPlayerInputHandlerComponent::On_Input_Movement(const FInputActionInstance& Instance)
{
	if (const ETriggerEvent TriggerEvent{Instance.GetTriggerEvent()}; TriggerEvent == ETriggerEvent::Triggered)
	{
		RawInputMovementVector = Instance.GetValue().Get<FVector2D>();
	} else if (TriggerEvent == ETriggerEvent::Completed || TriggerEvent == ETriggerEvent::Canceled)
	{
		RawInputMovementVector = FVector2D::ZeroVector;
	}
}

void UPlayerInputHandlerComponent::On_Input_Look(const FInputActionInstance& Instance)
{
	if (const ETriggerEvent TriggerEvent{Instance.GetTriggerEvent()}; TriggerEvent == ETriggerEvent::Triggered)
	{
		RawInputLookVector = Instance.GetValue().Get<FVector2D>();
	} else if (TriggerEvent == ETriggerEvent::Completed || TriggerEvent == ETriggerEvent::Canceled)
	{
		RawInputLookVector = FVector2D::ZeroVector;
	}
}

// Todo: Dodge can be complicated
void UPlayerInputHandlerComponent::On_Input_Dodge(const FInputActionInstance& Instance)
{
	if (const ETriggerEvent TriggerEvent{Instance.GetTriggerEvent()}; TriggerEvent == ETriggerEvent::Triggered)
	{
		InputActionBitmask.Set(EInputAction::EInputActionFlag_Dodge, true);
	} else if (TriggerEvent == ETriggerEvent::Completed || TriggerEvent == ETriggerEvent::Canceled)
	{
		InputActionBitmask.Set(EInputAction::EInputActionFlag_Dodge, false);
	}
}

#define DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(FuncName, EInputAction) \
void UPlayerInputHandlerComponent::FuncName(const FInputActionInstance& Instance) \
{ \
	if (const ETriggerEvent TriggerEvent{Instance.GetTriggerEvent()}; TriggerEvent == ETriggerEvent::Triggered) \
	{ \
		InputActionBitmask.Set(EInputAction, true); \
	} else if (TriggerEvent == ETriggerEvent::Completed || TriggerEvent == ETriggerEvent::Canceled) \
	{ \
		InputActionBitmask.Set(EInputAction, false); \
	} \
} \

DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(On_Input_Basic_Attack, EInputAction::EInputActionFlag_Basic_Attack)
DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(On_Input_Special_Attack, EInputAction::EInputActionFlag_Special_Attack)
DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(On_Input_Ultimate, EInputAction::EInputActionFlag_Ultimate)
DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(On_Input_SwitchCharacter_Previous, EInputAction::EInputActionFlag_SwitchCharacter_Previous)
DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(On_Input_SwitchCharacter_Next, EInputAction::EInputActionFlag_SwitchCharacter_Next)
DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(On_Input_ChainAttack_Left, EInputAction::EInputActionFlag_Chain_Attack_Left)
DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(On_Input_ChainAttack_Right, EInputAction::EInputActionFlag_Chain_Attack_Right)
DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED(On_Input_ChainAttack_Cancel, EInputAction::EInputActionFlag_Chain_Attack_Cancel)

#undef DEFINE_INPUT_FUNCTION_SIMPLE_PRESSED

/*void UPlayerInputHandlerComponent::On_Input_Basic_Attack(const FInputActionInstance& Instance)
{
	if (const ETriggerEvent TriggerEvent{Instance.GetTriggerEvent()}; TriggerEvent == ETriggerEvent::Triggered)
	{
		InputActionBitmask.Set(EInputAction::EInputActionFlag_Basic_Attack, true);
	} else if (TriggerEvent == ETriggerEvent::Completed || TriggerEvent == ETriggerEvent::Canceled)
	{
		InputActionBitmask.Set(EInputAction::EInputActionFlag_Basic_Attack, false);
	}
}

void UPlayerInputHandlerComponent::On_Input_Special_Attack(const FInputActionInstance& Instance)
{
	
}

void UPlayerInputHandlerComponent::On_Input_Ultimate(const FInputActionInstance& Instance)
{
}

void UPlayerInputHandlerComponent::On_Input_SwitchCharacter_Previous(const FInputActionInstance& Instance)
{
}

void UPlayerInputHandlerComponent::On_Input_SwitchCharacter_Next(const FInputActionInstance& Instance)
{
}

void UPlayerInputHandlerComponent::On_Input_ChainAttack_Left(const FInputActionInstance& Instance)
{
}

void UPlayerInputHandlerComponent::On_Input_ChainAttack_Right(const FInputActionInstance& Instance)
{
}

void UPlayerInputHandlerComponent::On_Input_ChainAttack_Cancel(const FInputActionInstance& Instance)
{
}*/
