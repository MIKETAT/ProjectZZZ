#include "UI/QTEWidget/QuickAssistWindow.h"
#include "Components/Image.h"

void UQuickAssistWindow::NativeConstruct()
{
	Super::NativeConstruct();
}

void UQuickAssistWindow::SetAgent(UTexture2D* NewAgent)
{
	if (AgentHead)
	{
		AgentHead->SetBrushFromTexture(NewAgent);
	}
}

void UQuickAssistWindow::StartQuickAssistWindow(UTexture2D* NextAgent)
{
	if (bIsActive)
	{
		return;
	}
	bIsActive = true;

	UnbindAllFromAnimationFinished(Anim_PopIn);
	StopAnimation(Anim_PopIn);
	StopAnimation(Anim_FlashLoop);
	
	SetVisibility(ESlateVisibility::HitTestInvisible);
	
	if (AgentHead && NextAgent)
	{
		AgentHead->SetBrushFromTexture(NextAgent);
	}	
	
 	if (Anim_PopIn)
	{
		PlayAnimation(Anim_PopIn, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false);
	}

	UE_LOG(LogTemp, Warning, TEXT("Pop In Anim IsPlaying: %d"), IsAnimationPlaying(Anim_PopIn));
	
	if (Anim_FlashLoop)
	{
		PlayAnimation(Anim_FlashLoop, 0.f, 0, EUMGSequencePlayMode::Forward, 1.f, false);
	}
}

void UQuickAssistWindow::ResetAndCloseQuickAssistWindow()
{
	if (!bIsActive)
	{
		return;
	}
	bIsActive = false;

	if (Anim_FlashLoop)
	{
		StopAnimation(Anim_FlashLoop);
	}

	if (Anim_PopIn)
	{
		PlayAnimationReverse(Anim_PopIn);		

		UE_LOG(LogTemp, Warning, TEXT("Pop In Anim Reverse IsPlaying: %d"), IsAnimationPlaying(Anim_PopIn));
		
		FWidgetAnimationDynamicEvent EndDelegate;
		EndDelegate.BindDynamic(this, &UQuickAssistWindow::OnPopOutAnimationFinished);
		BindToAnimationFinished(Anim_PopIn, EndDelegate);
	} else
	{
		OnPopOutAnimationFinished();
	}
}

void UQuickAssistWindow::OnPopOutAnimationFinished()
{
	SetVisibility(ESlateVisibility::Collapsed);
	UnbindAllFromAnimationFinished(Anim_PopIn);
}
