#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QTEAgent.generated.h"

class UImage;

UCLASS()
class PROJECTZZZ_API UQTEAgent : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	
	void SetAgent(UTexture2D* AgentHead);
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Agent;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> KeyIcon;
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true", AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> KeyPromptIcon;
};
