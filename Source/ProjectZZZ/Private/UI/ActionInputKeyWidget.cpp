#include "UI/ActionInputKeyWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/UITypes.h"

void UActionInputKeyWidget::InitializeInputKeyWidget(const FActionInputKeyConfig& Config)
{
	RefreshInputKeyWidget(Config);
}

void UActionInputKeyWidget::RefreshInputKeyWidget(const FActionInputKeyConfig& Config)
{
	if (Config.bUseIcon && IsValid(Config.InputKeyIcon))
	{
		InputKeyIcon->SetBrushFromTexture(Config.InputKeyIcon);
		InputKeyIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		
		KeyText->SetVisibility(ESlateVisibility::Collapsed);
		OuterBorder->SetVisibility(ESlateVisibility::Collapsed);
		InnerBorder->SetVisibility(ESlateVisibility::Collapsed);
	} else
	{
		InputKeyIcon->SetBrushFromTexture(nullptr);
		InputKeyIcon->SetVisibility(ESlateVisibility::Collapsed);
		KeyText->SetText(Config.KeyText);
		KeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
		OuterBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		InnerBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}
