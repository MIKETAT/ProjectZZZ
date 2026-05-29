#pragma once

#include "CoreMinimal.h"
#include "ZZZCombatEventTypes.generated.h"

USTRUCT(BlueprintType)
struct FPlainPayload
{
	GENERATED_BODY()
	
};

USTRUCT(BlueprintType)
struct FChainAttackPayload
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FPerfectAssistStatePayload
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	bool bWindowOpen{false};

	UPROPERTY(EditDefaultsOnly)
	float ParryReferenceOffset{0.f};
};

USTRUCT(BlueprintType)
struct FQuickAssistPayload
{
	GENERATED_BODY()

};

USTRUCT(BlueprintType)
struct FCharacterDeathPayload
{
	GENERATED_BODY()
	
};