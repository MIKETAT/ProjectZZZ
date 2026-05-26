#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitStopComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API UHitStopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHitStopComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void ApplyHitStop(const float Duration, const float TimeScale);

	UFUNCTION(BlueprintCallable)
	void ClearHitStop();

	bool IsInHitStop() const { return bHitStopActive; }
	
private:
	bool bHitStopActive{false};

	float HitStopEndTime{0.0f};

	float OriginTimeDilation{1.f};
};
