// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatEventBusSubSystem.generated.h"

UENUM()
enum class ECombatEventHandleResult : uint8
{
	UnHandled						UMETA(DisplayName = "Unhandled"),
	Handled							UMETA(DisplayName = "Handled"),
	Consumed						UMETA(DisplayName = "Consumed"),
};

USTRUCT(BlueprintType)
struct FCombatEventMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag EventTag;

	// Scope

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UObject> Source;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UObject> Target;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UObject> Instigator;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer ContextTags;

	UPROPERTY(BlueprintReadOnly)
	int32 SequenceID{0};

	UPROPERTY(BlueprintReadOnly)
	FInstancedStruct Payload;
};

DECLARE_DELEGATE_RetVal_OneParam(ECombatEventHandleResult, FCombatEventDelegate, const FCombatEventMessage&);

USTRUCT()
struct FCombatEventListener
{
	GENERATED_BODY()

	FDelegateHandle Handle;		// for unsubscribe
	TWeakObjectPtr<UObject> OwnerObject{nullptr};
	int32 Priority{0};			// todo: use enum   higher priority receive msg earlier. 
	FCombatEventDelegate Callback;		

	// override operator <
	bool operator<(const FCombatEventListener& Other) const
	{
		return Priority > Other.Priority;
	}
};

USTRUCT()
struct FCombatEventChannel
{
	GENERATED_BODY()

	TArray<FCombatEventListener> Listeners;
	bool bNeedSort{false};
};

UCLASS()
class PROJECTZZZ_API UCombatEventBusSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	template<typename PayloadType>
	void BroadcastEvent(
		const FGameplayTag& EventTag,
		// ECombatEventScope  Character/Squad/Global
		UObject* Source,
		UObject* Target,
		UObject* Instigator,
		//const FGameplayTagContainer& ContextTags,
		const PayloadType& Payload
	)
	{
		FCombatEventMessage Message;
		Message.EventTag = EventTag;
		Message.Source = Source;
		Message.Target = Target;
		Message.Instigator = Instigator;
		// ...
		Message.Payload = FInstancedStruct::Make(Payload);
		Dispatch(Message);
	}

	// Subscribe
	FDelegateHandle Subscribe(const FGameplayTag& EventTag, UObject* ListenerOwner, int32 Priority, FCombatEventDelegate Callback);
	
	// UnSubscribe
	void Unsubscribe(const FGameplayTag& EventTag, FDelegateHandle Handle);
private:
	void Dispatch(const FCombatEventMessage& Message);

private:
	int32 SequenceCounter{0};

	TMap<FGameplayTag, FCombatEventChannel> Channels;	
};
