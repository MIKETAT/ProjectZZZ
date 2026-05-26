#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
	Idle,
	Chase,
	
};