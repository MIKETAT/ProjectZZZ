#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActionIconPreset.generated.h"

struct FActionIconSlotConfig;

UCLASS()
class PROJECTZZZ_API UActionIconPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FActionIconSlotConfig> SlotConfigs;
};
