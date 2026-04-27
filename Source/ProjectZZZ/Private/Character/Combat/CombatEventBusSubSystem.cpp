// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/CombatEventBusSubSystem.h"

FDelegateHandle UCombatEventBusSubSystem::Subscribe(const FGameplayTag& EventTag, UObject* ListenerOwner, int32 Priority, FCombatEventDelegate Callback)
{
	if (!ListenerOwner || !Callback.IsBound())
	{
		return FDelegateHandle();
	}

	FCombatEventChannel& Channel = Channels.FindOrAdd(EventTag);

	FCombatEventListener Listener;
	Listener.Handle = Callback.GetHandle();
	Listener.OwnerObject = ListenerOwner;
	Listener.Priority = Priority;
	Listener.Callback = Callback;
	Channel.Listeners.Add(MoveTemp(Listener));
	Channel.bNeedSort = true;

	return Listener.Handle;
}

void UCombatEventBusSubSystem::Unsubscribe(const FGameplayTag& EventTag, FDelegateHandle Handle)
{
	if (FCombatEventChannel* Channel = Channels.Find(EventTag))
	{
		Channel->Listeners.RemoveAll([Handle](const FCombatEventListener& Listener){ return Listener.Handle == Handle; });
	}
}

void UCombatEventBusSubSystem::Dispatch(const FCombatEventMessage& Message)
{
	FCombatEventChannel* Channel = Channels.Find(Message.EventTag);
	if (!Channel)
	{
		return;
	}

	if (Channel->bNeedSort)
	{
		Channel->Listeners.Sort();
		Channel->bNeedSort = false;
	}

	bool bNeedCleanUp{false};
	for (FCombatEventListener& Listener : Channel->Listeners)
	{
		if (!Listener.OwnerObject.IsValid())
		{
			bNeedCleanUp = true;
			continue;
		}

		ECombatEventHandleResult Result = Listener.Callback.Execute(Message);
		
		if (Result == ECombatEventHandleResult::Consumed)
		{
			break;
		}
	}

	if (bNeedCleanUp)
	{
		Channel->Listeners.RemoveAll([](const FCombatEventListener& Listener){ return !Listener.OwnerObject.IsValid(); });
	}
}
