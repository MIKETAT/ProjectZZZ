#include "Character/ZZZPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "AI/EnemyCharacterBase.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/Component/SquadManagerComponent.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "UI/QTEWidget/QTEWidget.h"
#include "UI/QTEWidget/QuickAssistWindow.h"
#include "Utility/ZZZGameplayTag.h"

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

void AZZZPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DisableUltimateCutInPostProcess();
	Super::EndPlay(EndPlayReason);
}

void AZZZPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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

APlayerCharacter* AZZZPlayerController::GetActiveAgent() const
{
	if (SquadManager)
	{
		return SquadManager->GetActiveAgent();
	}
	return nullptr;
}

void AZZZPlayerController::RequestUltimateCutIn(const FPendingUltimateCutInRequest& Request)
{
	if (!Request.bIsValid || !Request.Agent.IsValid() || PendingUltimateCutInRequest.bIsValid || UltimateExecutionState.bIsValid)
	{
		return;
	}

	PendingUltimateCutInRequest = Request;

	// Add Camera Tag and Lock Switch
	GetSquadManagerComponent()->SetLockAgentSwitch(true);
	bBlockGameplayCameraActivation = true;
	Request.Agent->GetAbilitySystemComp()->AddLooseGameplayTag(Combat::Camera::Status::UltimateCamera);
}

void AZZZPlayerController::OnCameraRigSelected(const FGameplayTag& SelectedCameraRigTag)
{
	if (SelectedCameraRigTag == CurrentCameraRigTag)
	{
		return;
	}

	CurrentCameraRigTag = SelectedCameraRigTag;
	if (PendingUltimateCutInRequest.bIsValid && SelectedCameraRigTag == PendingUltimateCutInRequest.CameraStateTag)
	{
		CommitPendingUltimateCutIn();
	} 
}

void AZZZPlayerController::CommitPendingUltimateCutIn()
{
	// Invalid Pending Request	
	if (!PendingUltimateCutInRequest.bIsValid || !PendingUltimateCutInRequest.Agent.IsValid()
		|| !PendingUltimateCutInRequest.CutInSequence.IsValid() || !PendingUltimateCutInRequest.UltimateAction.IsValid())
	{
		CancelPendingUltimateCutIn();
		return;
	}

	APlayerCharacter* Agent{PendingUltimateCutInRequest.Agent.Get()};
	UCharacterCombatComponent* CombatComponent{Agent->GetAgentCombatComponent()};
	if (!CombatComponent)
	{
		CancelPendingUltimateCutIn();
		return;
	}
	
	ALevelSequenceActor* SequenceActor{nullptr};
	FMovieSceneSequencePlaybackSettings Settings;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), PendingUltimateCutInRequest.CutInSequence.Get(), Settings, SequenceActor);
	
	TArray<AActor*> Actors;
	Actors.Add(PendingUltimateCutInRequest.Agent.Get());
	
	if (!SequencePlayer || !SequenceActor)
	{
		CancelPendingUltimateCutIn();
		return;
	}
	
	const TArray<FMovieSceneObjectBindingID>& Bindings =
		SequenceActor->FindNamedBindings(PendingUltimateCutInRequest.UltimateAction->UltimateConfig.SequenceBindingTag);
	
	if (Bindings.IsEmpty())
	{
		CancelPendingUltimateCutIn();
		return;
	}
	
	SequenceActor->SetBindingByTag(FName("RuntimeAgent"), Actors, false);
	SequencePlayer->OnFinished.AddDynamic(this, &AZZZPlayerController::HandleSequencePlayFinished);
	const int32 InstanceID{CombatComponent->ExecuteUltimateAction(FCombatActionContext())};
	if (InstanceID == INDEX_NONE)
	{
		ClearPreparedSequence(SequencePlayer, SequenceActor);
		CancelPendingUltimateCutIn();
		return; 
	}
	
	SequencePlayer->Play();
	
	// Commit Successfully
	UCombatActionStep* Ultimate{CombatComponent->GetSpecialAction(Combat::SpecialAction::Ultimate)};
	EnableUltimateCutInPostProcess(Agent, PendingUltimateCutInRequest.BackgroundColor, PendingUltimateCutInRequest.StencilValue);

	// Set MPC Parameters
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(),MPC_UltimateCutIn, TEXT("BackgroundColor"), PendingUltimateCutInRequest.BackgroundColor);
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), MPC_UltimateCutIn, TEXT("AgentStencilValue"), PendingUltimateCutInRequest.StencilValue);
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), MPC_UltimateCutIn, TEXT("EffectAlpha"), 1.f);

	if (SquadManager && Ultimate)
	{
		// Adjust Rotation to Face Enemy
		
		if (AEnemyCharacterBase* Enemy = Agent->FindClosestEnemy(Ultimate->MotionWarpingEffectiveDistance))
		{
			const FVector FaceEnemyDir{(Enemy->GetActorLocation() - Agent->GetActorLocation()).GetSafeNormal2D()};
			
			Agent->SetActorRotation(FRotator(0.f, FaceEnemyDir.Rotation().Yaw, 0.f));
		}

		FTransform CameraTransform{SquadManager->CalculateUltimateCameraPosition(Ultimate, Agent->GetTransform())};
		SquadManager->OnUpdateCameraTransform.Broadcast(CameraTransform);
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("SquadManager invalid! THIS LOG SHOULD NOT BE PRINTED!"));
	}
	
	UltimateExecutionState.OnUltimateActionFinishedHandle = CombatComponent->OnCombatActionFinished.AddUObject(this, &AZZZPlayerController::HandleUltimateActionFinished);
	
	UltimateExecutionState.bIsValid = true;
	UltimateExecutionState.Agent = PendingUltimateCutInRequest.Agent;
	UltimateExecutionState.SequencePlayer = SequencePlayer;
	UltimateExecutionState.SequenceActor = SequenceActor;
	UltimateExecutionState.CameraStateTag = PendingUltimateCutInRequest.CameraStateTag;

	PendingUltimateCutInRequest.Reset();
}

void AZZZPlayerController::ClearPreparedSequence(ULevelSequencePlayer* SequencePlayer, ALevelSequenceActor* SequenceActor)
{
	if (SequencePlayer)
	{
		SequencePlayer->OnFinished.RemoveAll(this);
		SequencePlayer->Stop();
	}
	if (SequenceActor)
	{
		SequenceActor->Destroy();
	}
}

void AZZZPlayerController::CancelPendingUltimateCutIn()
{
	if (PendingUltimateCutInRequest.Agent.IsValid())
	{
		if (UAbilitySystemComponent* ASC = PendingUltimateCutInRequest.Agent->GetAbilitySystemComponent())
		{
			ASC->SetLooseGameplayTagCount(Combat::Camera::Status::UltimateCamera, 0);
		}
	}
	
	bBlockGameplayCameraActivation = false;
	GetSquadManagerComponent()->SetLockAgentSwitch(false);
	PendingUltimateCutInRequest.Reset();
}

void AZZZPlayerController::HandleSequencePlayFinished()
{
	ensureMsgf(GetSquadManagerComponent(), TEXT("SquadManagerComponent Invalid in PlayerController"));
	if (!UltimateExecutionState.bIsValid)
	{
		DisableUltimateCutInPostProcess();
		return;
	}
	
	// Close PostProcess
	DisableUltimateCutInPostProcess();
	
	UltimateExecutionState.bActionFinished = true;
	
	if (UltimateExecutionState.SequencePlayer.IsValid())
	{
		UltimateExecutionState.SequencePlayer->OnFinished.RemoveDynamic(this, &AZZZPlayerController::HandleSequencePlayFinished);
	}
	
	UltimateExecutionState.SequencePlayer = nullptr;

	if (UltimateExecutionState.SequenceActor.IsValid())
	{
		UltimateExecutionState.SequenceActor->Destroy();
		UltimateExecutionState.SequenceActor = nullptr;
	}
	
	// Ultimate Action Not Finished yet.	
}

void AZZZPlayerController::HandleUltimateActionFinished(APlayerCharacter* Agent, ECombatAnimRequestFinishReason Reason)
{
	if (!Agent || Agent != UltimateExecutionState.Agent || !UltimateExecutionState.bIsValid)
	{
		return;
	}

	DisableUltimateCutInPostProcess();
	
	if (Reason == ERequestFinishReason_Cancelled && UltimateExecutionState.SequencePlayer.IsValid())
	{
		UltimateExecutionState.SequencePlayer->Stop();
		DisableUltimateCutInPostProcess();
	}
	
	if (UCharacterCombatComponent* CombatComponent = Agent->GetAgentCombatComponent())
	{
		if (UltimateExecutionState.OnUltimateActionFinishedHandle.IsValid())
		{
			CombatComponent->OnCombatActionFinished.Remove(UltimateExecutionState.OnUltimateActionFinishedHandle);
			UltimateExecutionState.OnUltimateActionFinishedHandle.Reset();
		}
	}
	
	Agent->GetAbilitySystemComponent()->SetLooseGameplayTagCount(Combat::Camera::Status::UltimateCamera, 0);
	
	bBlockGameplayCameraActivation = false;
	SquadManager->SetLockAgentSwitch(false);
	
	UltimateExecutionState.Reset();
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
		SquadManager->OnTriggerChainAttackWindow.AddUObject(QTEWidget, &UQTEWidget::StartQTEWindow);
		SquadManager->OnFinishChainAttack.AddUObject(QTEWidget, &UQTEWidget::ResetAndCloseQTEWindow);
	}
	// Quick Assist
	if (QuickAssistWidget)
	{
		SquadManager->OnTriggerQuickAssistWindow.AddUObject(QuickAssistWidget, &UQuickAssistWindow::StartQuickAssistWindow);
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

void AZZZPlayerController::EnableUltimateCutInStencil(APlayerCharacter* Agent, const int32 StencilValue)
{
	if (!IsValid(Agent))
	{
		return;
	}

	DisableUltimateCutInStencil();

	TArray<UPrimitiveComponent*> Primitives;
	Agent->GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!IsValid(Primitive) || !Primitive->IsVisible() || Primitive->bHiddenInGame)
		{
			continue;
		}

		FCutInStencilData Data;
		Data.Component = Primitive;
		Data.bEnableRenderCustomDepth = true;
		Data.CustomStencilDepth = Primitive->CustomDepthStencilValue;

		CutInStencilDate.Add(Data);
		Primitive->SetRenderCustomDepth(true);
		Primitive->SetCustomDepthStencilValue(StencilValue);
	}
}

void AZZZPlayerController::DisableUltimateCutInStencil()
{
	for (const FCutInStencilData& Data : CutInStencilDate)
	{
		UPrimitiveComponent* Comp = Data.Component.Get();
		if (!IsValid(Comp))
		{
			continue;
		}

		Comp->SetRenderCustomDepth(false);
		Comp->SetCustomDepthStencilValue(Data.CustomStencilDepth);
	}

	CutInStencilDate.Reset();
}

void AZZZPlayerController::EnableUltimateCutInPostProcess(APlayerCharacter* Agent, const FLinearColor& BackgroundColor, const int32 StencilValue)
{

	EnableUltimateCutInStencil(Agent, StencilValue);

	bUltimateCutInPostProcessActive = true;
}

void AZZZPlayerController::DisableUltimateCutInPostProcess()
{
	if (MPC_UltimateCutIn)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), MPC_UltimateCutIn, TEXT("EffectAlpha"), 0.f);
	}
	
	DisableUltimateCutInStencil();

	bUltimateCutInPostProcessActive = false;
}

