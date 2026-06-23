#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Character/Combat/CombatStep.h"
#include "AnimNotifyState_CombatCameraSwitch.generated.h"

class AZZZPlayerController;
enum class ECombatCameraMode : uint8;

UCLASS()
class PROJECTZZZ_API UAnimNotifyState_CombatCameraSwitch : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
private:
	AZZZPlayerController* GetPlayerController(USkeletalMeshComponent* MeshComp);

	void ToggleCombatCamera(USkeletalMeshComponent* MeshComp, bool bOpen);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECombatCameraMode Mode{ECombatCameraMode::None};
};
