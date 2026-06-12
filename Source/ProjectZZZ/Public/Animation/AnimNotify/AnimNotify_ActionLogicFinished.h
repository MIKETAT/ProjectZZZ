#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ActionLogicFinished.generated.h"

UCLASS()
class PROJECTZZZ_API UAnimNotify_ActionLogicFinished : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Detection")
	FGameplayTag ActionTag{FGameplayTag::EmptyTag};

};
