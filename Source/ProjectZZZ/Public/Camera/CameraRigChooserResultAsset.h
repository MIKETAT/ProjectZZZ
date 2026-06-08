// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CameraRigChooserResultAsset.generated.h"

class UCameraRigAsset;
/**
 * 
 */
UCLASS(BlueprintType)
class PROJECTZZZ_API UCameraRigChooserResultAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCameraRigAsset> CameraRig{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CameraRigTag;
};
