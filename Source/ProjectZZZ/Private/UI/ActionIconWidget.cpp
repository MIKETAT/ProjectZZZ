#include "UI/ActionIconWidget.h"
#include "Components/Image.h"
#include "UI/ActionInputKeyWidget.h"
#include "UI/UITypes.h"

void UActionIconWidget::InitializeActionIcon(const FActionIconSlotConfig& Config)
{
	// Cache Icon Texture
	if (Config.DefaultIcon)
	{
		DefaultIcon = Config.DefaultIcon;
	}

	if (Config.ActiveIcon)
	{
		ActiveIcon = Config.ActiveIcon;
	}
	
	Icon->SetBrushFromTexture(Config.DefaultIcon);
	InputKey->InitializeInputKeyWidget(Config.InputKeyConfig);

	// Circle
	if (!Config.bNeedCircle)
	{
		Circle->SetBrushFromTexture(nullptr);
		Circle->SetVisibility(ESlateVisibility::Collapsed);
	} else
	{
		Circle->SetBrushFromTexture(Config.CircleIcon);
		Circle->SetVisibility(ESlateVisibility::Visible);
	}
}

void UActionIconWidget::SetActionIconState(bool bActive)
{
	if (bActive && IsValid(ActiveIcon))
	{
		Icon->SetBrushFromTexture(ActiveIcon);
	} else
	{
		Icon->SetBrushFromTexture(DefaultIcon);
	}
}
