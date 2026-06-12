#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_AttackDetection.generated.h"

class UAttackDetectionConfig;
class UCombatActionStep;
class UCombatComponentBase;

UCLASS()
class PROJECTZZZ_API UAnimNotifyState_AttackDetection : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
private:
	UCombatComponentBase* GetCombatComponentBase(const USkeletalMeshComponent* MeshComp) const;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Detection")
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Detection")
	FName SegmentName{FName("Default")};

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Detection")
	TObjectPtr<UAttackDetectionConfig> DetectionConfig;*/
};
