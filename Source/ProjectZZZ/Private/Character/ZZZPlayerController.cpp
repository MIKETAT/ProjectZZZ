#include "Character/ZZZPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "UI/QTEWidget/QTEWidget.h"
#include "UI/QTEWidget/QuickAssistWindow.h"

AZZZPlayerController::AZZZPlayerController()
{
	// Input Handler
	PlayerInputHandlerComponent = CreateDefaultSubobject<UPlayerInputHandlerComponent>(TEXT("InputHandlerComponent"));

	SquadManager = CreateDefaultSubobject<USquadManagerComponent>(TEXT("SquadManager"));
}

void AZZZPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CreateQTEWidget();
	CreateQuickAssistWidget();
	BindUIDelegate();
}

void AZZZPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}

void AZZZPlayerController::CreateQTEWidget()
{
	if (IsValid(QTEWidgetClass))
	{
		QTEWidget = CreateWidget<UQTEWidget>(this, QTEWidgetClass);
		if (QTEWidget)
		{
			QTEWidget->InitializePtr(SquadManager);
			QTEWidget->AddToViewport();
			QTEWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create QTEWidget class"));
	}
}

void AZZZPlayerController::BindUIDelegate()
{
	if (!IsValid(SquadManager))
	{
		return;
	}
	// Chain Attack
	if (QTEWidget)
	{
		SquadManager->OnTriggerChainAttack.AddUObject(QTEWidget, &UQTEWidget::StartQTEWindow);
		SquadManager->OnFinishChainAttack.AddUObject(QTEWidget, &UQTEWidget::ResetAndCloseQTEWindow);
	}
	// Quick Assit
	if (QuickAssistWidget)
	{
		SquadManager->OnTriggerQuickAssist.AddUObject(QuickAssistWidget, &UQuickAssistWindow::StartQuickAssistWindow);
		SquadManager->OnFinishQuickAssist.AddUObject(QuickAssistWidget, &UQuickAssistWindow::ResetAndCloseQuickAssistWindow);
	}
}

void AZZZPlayerController::CreateQuickAssistWidget()
{
	if (IsValid(QuickAssistWidgetClass))
	{
		QuickAssistWidget = CreateWidget<UQuickAssistWindow>(this, QuickAssistWidgetClass);
		if (QuickAssistWidget)
		{
			QuickAssistWidget->AddToViewport();
			QuickAssistWidget->SetVisibility(ESlateVisibility::Collapsed);
		} else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create QuickAssistWindow class"));
		}
	}
}
