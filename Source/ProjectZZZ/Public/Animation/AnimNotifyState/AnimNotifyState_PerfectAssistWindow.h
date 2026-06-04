#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_PerfectAssistWindow.generated.h"

UCLASS()
class PROJECTZZZ_API UAnimNotifyState_PerfectAssistWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	
#endif
	
private:
	void BroadcastPerfectAssistWindowStateChanged(const bool bWindowOpen, AActor* Owner);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector IdealParryOffset{0.f, 0.f, 0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName{FName("WeaponReference")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDrawDebug{false};
};
