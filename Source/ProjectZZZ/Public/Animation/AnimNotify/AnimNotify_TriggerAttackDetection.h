#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_TriggerAttackDetection.generated.h"

class UCombatComponentBase;

UCLASS()
class PROJECTZZZ_API UAnimNotify_TriggerAttackDetection : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
private:
	UCombatComponentBase* GetCombatComponentBase(const USkeletalMeshComponent* MeshComp) const;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Detection")
	FGameplayTag ActionTag{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Detection")
	FName SegmentName{FName("Default")};
};
