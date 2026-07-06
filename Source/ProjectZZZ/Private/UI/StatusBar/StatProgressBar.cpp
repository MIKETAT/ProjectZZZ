#include "UI/StatusBar/StatProgressBar.h"
#include "Components/ProgressBar.h"

void UStatProgressBar::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeStatProgressBar();
}

void UStatProgressBar::InitializeStatProgressBar()
{
	if (!ProgressBar || !ProgressBarMaterial)
	{
		return;
	}
	
	if (!FillImageMID)
	{
		FillImageMID = UMaterialInstanceDynamic::Create(ProgressBarMaterial, this);
	}

	if (!BackgroundMID)
	{
		BackgroundMID = UMaterialInstanceDynamic::Create(ProgressBarMaterial, this);
	}

	FProgressBarStyle Style{ProgressBar->GetWidgetStyle()};

	if (FillImageMID)
	{
		FSlateBrush FillBrush{Style.FillImage};
		FillImageMID->SetTextureParameterValue(ProgressBarMaskParameterName, BarMask);
		FillImageMID->SetScalarParameterValue(ProgressBarEnableTexScalarParameterName, 1.f);		// Use Texture
		FillBrush.SetResourceObject(FillImageMID);
		FillBrush.DrawAs = ESlateBrushDrawType::Type::Image;
		Style.FillImage = FillBrush;
	}

	if (BackgroundMID)
	{
		FSlateBrush BackgroundBrush{Style.BackgroundImage};
		BackgroundMID->SetTextureParameterValue(ProgressBarMaskParameterName, BarMask);
		BackgroundMID->SetVectorParameterValue(ProgressBarColorVectorParameterName, BackgroundBarColor);
		BackgroundBrush.SetResourceObject(BackgroundMID);
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Type::Image;
		Style.BackgroundImage = BackgroundBrush;
	}
	
	ProgressBar->SetWidgetStyle(Style);
}

void UStatProgressBar::SetPercentage(const float Percentage)
{
	if (Percentage < 0.f || Percentage > 1.f || !ProgressBar)
	{
		return;
	}
	
	ProgressBar->SetPercent(Percentage);
}

void UStatProgressBar::InitializeProgressBarFillImage(UTexture2D* FillImage)
{ 
	if (!ProgressBar || !FillImage)
	{
		return;
	}
	
	FProgressBarStyle Style{ProgressBar->GetWidgetStyle()};

	if (FillImageMID)
	{
		FSlateBrush FillBrush{Style.FillImage};
		FillImageMID->SetTextureParameterValue(ProgressBarTexParameterName, FillImage);
		FillBrush.SetResourceObject(FillImageMID);
		FillBrush.DrawAs = ESlateBrushDrawType::Type::Image;
		Style.FillImage = FillBrush;
	}

	ProgressBar->SetWidgetStyle(Style);
}
