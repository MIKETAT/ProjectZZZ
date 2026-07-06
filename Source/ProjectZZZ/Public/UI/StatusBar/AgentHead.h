#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AgentHead.generated.h"

class USquadManagerComponent;
struct FAgentStatusSnapShot;
class APlayerCharacter;
struct FHUDSquadSource;
struct FHUDSquadAgentSource;
class UImage;

UCLASS()
class PROJECTZZZ_API UAgentHead : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindDelegate(const FHUDSquadSource& Source);

	void UnBindDelegate();

	void RefreshAgentHead(const FAgentStatusSnapShot& SnapShot);

	void ApplyAgentTexture(UTexture2D* Texture);

	UFUNCTION()
	void HandleActiveAgentChanged(APlayerCharacter* OldAgent, APlayerCharacter* NewAgent);
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> AgentHead{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> AgentHeadMaterial{nullptr};

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AgentHeadMID{nullptr};

	inline static const FName BrushTextureParameterName{FName(FString("BrushTexture"))};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float AvatarWidth{32.f};

private:
	UPROPERTY()
	TWeakObjectPtr<USquadManagerComponent> SquadManager{nullptr};
	
	FDelegateHandle ActiveAgentChangedDelegate;
};
