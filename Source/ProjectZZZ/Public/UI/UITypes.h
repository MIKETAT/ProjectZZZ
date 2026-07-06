#pragma once

#include "CoreMinimal.h"
#include "UITypes.generated.h"

UENUM(BlueprintType)
enum class EActionIconSlot : uint8
{
	None,
	BasicAttack,
	Dodge,
	SpecialAttack,
	Switch,
	Ultimate
};

USTRUCT(BlueprintType)
struct FActionInputKeyConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bUseIcon{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText KeyText{FText::FromString("Q")};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseIcon"))
	TObjectPtr<UTexture2D> InputKeyIcon{nullptr};
};

USTRUCT(BlueprintType)
struct FActionIconSlotConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EActionIconSlot IconSlot{EActionIconSlot::None};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bNeedCircle{false};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> ActiveIcon{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> DefaultIcon{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = bNeedCircle))
	TObjectPtr<UTexture2D> CircleIcon{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FActionInputKeyConfig InputKeyConfig;
};
