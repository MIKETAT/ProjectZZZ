#include "UI/QTEWidget/QTEWidget.h"

#include "Character/Component/SquadManagerComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/QTEWidget/QTEAgent.h"

void UQTEWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (!QTEProgressBar)
	{
		return;
	}
	
	if (UObject* ResourceObject{QTEProgressBar->GetBrush().GetResourceObject()})
	{
		if (UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(ResourceObject))
		{
			ProgressBarMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			QTEProgressBar->SetBrushFromMaterial(ProgressBarMID);
		}
	}
}

void UQTEWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshWindow();
}

void UQTEWidget::StartQTEWindow(UTexture2D* PreviousAgentHead, UTexture2D* NextAgentHead)
{
	if (PreviousAgent && PreviousAgentHead)
	{
		PreviousAgent->SetAgent(PreviousAgentHead);
	}

	if (NextAgent && NextAgentHead)
	{
		NextAgent->SetAgent(NextAgentHead);
	}
	SetVisibility(ESlateVisibility::Visible);
	RefreshWindow();
}

void UQTEWidget::RefreshWindow()
{
	if (!SquadManager.IsValid())
	{
		return;
	}

	FChainAttackWindowStatus Status = SquadManager->GetChainAttackWindowStatus(); 
	
	float Ratio{Status.QTERemainingTime / Status.QTEDuration};
	int32 Seconds{FMath::FloorToInt(Status.QTERemainingTime)};
	int32 Milliseconds{FMath::FloorToInt((Status.QTERemainingTime - Seconds) * 100)};
	FString TimeString{FString::Printf(TEXT("00:%02d:%02d"), Seconds, Milliseconds)};

	if (ProgressBarMID)
	{
		ProgressBarMID->SetScalarParameterValue(FName("Ratio"), Ratio);
	}

	if (CountDownText)
	{
		CountDownText->SetText(FText::FromString(TimeString));
	}

	UpdateQTEVisuals();
}

void UQTEWidget::ResetAndCloseQTEWindow()
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (PreviousAgent)
	{
		PreviousAgent->SetAgent(nullptr);
	}

	if (NextAgent)
	{
		NextAgent->SetAgent(nullptr);
	}
}
