// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/Component/CombatAnimSchedulerComponent.h"

#include "Animation/AnimInstanceBase.h"
#include "Character/CharacterBase.h"


UCombatAnimSchedulerComponent::UCombatAnimSchedulerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}


// Called when the game starts
void UCombatAnimSchedulerComponent::BeginPlay()
{
	Super::BeginPlay();

	/*// Bind Delegate
	check(AnimInstance);
	AnimInstance->OnMontageStarted.AddDynamic(this, &ThisClass::HandleMontageStarted);
	AnimInstance->OnMontageBlendingOut.AddDynamic(this, &ThisClass::HandleMontageBlendingOut);
	AnimInstance->OnMontageEnded.AddDynamic(this, &ThisClass::HandleMontageEnd);*/
}

void UCombatAnimSchedulerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshProceedingRequest();
}

void UCombatAnimSchedulerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (ACharacterBase* CharacterBase = Cast<ACharacterBase>(GetOwner()))
	{
		Character = CharacterBase;
		AnimInstance = Cast<UAnimInstanceBase>(CharacterBase->GetMesh()->GetAnimInstance());
	}
}

int32 UCombatAnimSchedulerComponent::ExecuteAnimRequest(const FCombatAnimExecutionRequest& Request)
{
	int32 MontageID{INDEX_NONE};
	if (!IsValid(AnimInstance) || !Request.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Request or Invalid AnimInstance"));
		return MontageID;
	}

	// Check Duplicated Request
	MontageID = CheckIfRequestMontageAlreadyPlaying(Request);
	if (MontageID != INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Request Montage Already Playing"));
		return MontageID;
	}

	TArray<int32> PendingStopRequests;
	if (CanExecuteCombatAnimRequest(Request, PendingStopRequests))
	{
		// Stop all PendingStopResults Montage
		FMontageBlendSettings BlendSettings;
		BlendSettings.Blend = Request.Montage->BlendIn;
		BlendSettings.BlendMode = Request.Montage->BlendModeIn;
		BlendSettings.BlendProfile = Request.Montage->BlendProfileIn;
		for (int32 Id: PendingStopRequests)
		{ 
			const FCombatAnimExecutionRequest* StopRequest = ProceedingRequests.Find(Id);
			if (!StopRequest)
			{
				continue;
			}
			AnimInstance->Montage_StopWithBlendSettings(BlendSettings, StopRequest->Montage);
			UE_LOG(LogTemp, Warning, TEXT("Stop Montage: %s"), *StopRequest->Montage->GetName());
		}
		
		// Finish All PendingStopRequest
		for (int32 Id : PendingStopRequests)
		{
			FinishRequest(Id, ERequestFinishReason_Interrupted);
		}
		
		// Add New Request
		int32 NewId = ++NextIDGenerator;
		FCombatAnimExecutionRequest AddedRequest = Request;
		AddedRequest.RequestID = NewId;
		AddedRequest.MontageEventFlags |= static_cast<uint8>(EMontageStatusFlag::EMontageStatus_Started);
		ProceedingRequests.Add(NewId, AddedRequest);

		FMontageBlendSettings BlendInSetting{GetMontageBlendInSetting(AddedRequest.Montage)};
		AnimInstance->Montage_PlayWithBlendSettings(AddedRequest.Montage, BlendInSetting, AddedRequest.PlayRate);
		
		BindMontageNativeDelegates(AddedRequest.Montage, AddedRequest.RequestID);

		//UE_LOG(LogTemp, Error, TEXT("Action: Execute Anim Request, Request ID = %d, Montage Name = %s"), AddedRequest.RequestID, *AddedRequest.Montage->GetName())
		
		return AddedRequest.RequestID;
	}
	// Can't Execute
	UE_LOG(LogTemp, Error, TEXT("Can not Execute Anim Request. Montage = %s"), *Request.Montage->GetName());
	return INDEX_NONE;
}

void UCombatAnimSchedulerComponent::CancelAnimRequest(const int32 RequestID)
{
	if (!IsValid(AnimInstance) || !ProceedingRequests.Contains(RequestID))
	{
		return;
	}

	if (const FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID))
	{
		FMontageBlendSettings BlendSettings;
		BlendSettings.Blend = Request->Montage->BlendIn;
		BlendSettings.BlendMode = Request->Montage->BlendModeIn;
		BlendSettings.BlendProfile = Request->Montage->BlendProfileIn;
		AnimInstance->Montage_StopWithBlendSettings(BlendSettings, Request->Montage);
		FinishRequest(RequestID, ERequestFinishReason_Cancelled);
	}
}

bool UCombatAnimSchedulerComponent::RequestMontageSetNextSection(const int32 RequestID, const FName& LoopSectionName, const FName& NextSectionName)
{
	if (!IsValid(AnimInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("Attack Detection: Montage Jump to Section Failed. Invalid AnimInstance"));
		return false;
	}

	if (const FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID))
	{
		if (!IsRequestMontageBlendingOut(Request) && Request->Montage)
		{
			AnimInstance->Montage_JumpToSection(LoopSectionName, Request->Montage);
			AnimInstance->Montage_SetNextSection(LoopSectionName, NextSectionName, Request->Montage);
			//AnimInstance->Montage_JumpToSection(SectionName, Request->Montage);
			UE_LOG(LogTemp, Error, TEXT("Attack Detection: Montage Jump to Section Succeed"));
			return true;
		}
	}
	return false;
}

bool UCombatAnimSchedulerComponent::IsRequestMontageBlendingOut(const FCombatAnimExecutionRequest* Request) const
{
	if (!Request || !Request->IsValid())
	{
		return false;
	}
	return Request->MontageEventFlags & static_cast<uint8>(EMontageStatusFlag::EMontageStatus_BlendingOut);
}

int32 UCombatAnimSchedulerComponent::CheckIfRequestMontageAlreadyPlaying(const FCombatAnimExecutionRequest& Request) const
{
	for (auto& [Id, ProceedingRequest] : ProceedingRequests)
	{
		if (ProceedingRequest.Montage && ProceedingRequest.Montage == Request.Montage && !IsRequestMontageBlendingOut(&ProceedingRequest))
		{
			return Id;
		}
	}
	return INDEX_NONE;
}

bool UCombatAnimSchedulerComponent::CanExecuteCombatAnimRequest(const FCombatAnimExecutionRequest& Request, TArray<int32>& PendingStopRequestIDs) const
{
	FName TargetSlotName = GetMontageSlotName(Request.Montage);
	
	for (auto& [Id, ProceedingRequest] : ProceedingRequests)
	{
		FName ProceedingSlotName = GetMontageSlotName(ProceedingRequest.Montage);
		if (ProceedingSlotName != TargetSlotName)
		{
			continue;
		}
		
		// Not Blending Out. New Request has lower priority
		if (!IsRequestMontageBlendingOut(&ProceedingRequest) && ProceedingRequest.Priority > Request.Priority)
		{
			UE_LOG(LogTemp, Warning, TEXT("Current Montage Not Blending Out. New Request has lower priority"));
			return false;	
		}
		
		// Is Blending Out or Request has higher priority
		PendingStopRequestIDs.Add(Id);
	}
	return true;
}

void UCombatAnimSchedulerComponent::FinishRequest(const int32 RequestID, const ECombatAnimRequestFinishReason Reason)
{
	FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID);
	if (!Request || Request->bIsFinished)
	{
		return;
	}

	Request->bIsFinished = true;
	OnAnimRequestFinished.Broadcast(RequestID, Reason);
	ProceedingRequests.Remove(RequestID);
}

FName UCombatAnimSchedulerComponent::GetMontageSlotName(const UAnimMontage* Montage) const
{
	if (!IsValid(Montage) || Montage->SlotAnimTracks.Num() <= 0)
	{
		return FName("DefaultSlot");
	}
	return Montage->SlotAnimTracks[0].SlotName;
}

FMontageBlendSettings UCombatAnimSchedulerComponent::GetMontageBlendInSetting(const UAnimMontage* Montage) const
{
	FMontageBlendSettings BlendInSetting;
	if (IsValid(Montage))
	{
		BlendInSetting.Blend = Montage->BlendIn;
		BlendInSetting.BlendMode = Montage->BlendModeIn;
		BlendInSetting.BlendProfile = Montage->BlendProfileIn;
	}
	return BlendInSetting;
}

void UCombatAnimSchedulerComponent::BindMontageNativeDelegates(UAnimMontage* Montage, const int32 Id)
{
	if (!IsValid(Montage) || !IsValid(AnimInstance))
	{
		return;
	}
	
	// BlendingOut
	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUObject(this, &ThisClass::HandleMontageBlendingOut, Id);
	AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Montage);

	FOnMontageEnded EndedDelegate;
	EndedDelegate.BindUObject(this, &ThisClass::HandleMontageEnd, Id);
	AnimInstance->Montage_SetEndDelegate(EndedDelegate, Montage);
}

void UCombatAnimSchedulerComponent::HandleMontageStarted(UAnimMontage* Montage, int32 RequestID)
{
	
}

void UCombatAnimSchedulerComponent::HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 RequestID)
{
	if (!IsValid(Montage))
	{
		return;
	}
	if (FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID))
	{
		Request->MontageEventFlags |= static_cast<uint8>(EMontageStatusFlag::EMontageStatus_BlendingOut);
	}
}

void UCombatAnimSchedulerComponent::HandleMontageEnd(UAnimMontage* Montage, bool bInterrupted, int32 RequestID)
{
	if (!IsValid(Montage))
	{
		return;
	}
	if (FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID))
	{
		Request->MontageEventFlags |= static_cast<uint8>(EMontageStatusFlag::EMontageStatus_Ended);
	}
}

void UCombatAnimSchedulerComponent::RefreshProceedingRequest()
{
	
	TArray<TPair<int32, ECombatAnimRequestFinishReason>> PendingFinishRequests;
	TArray<TPair<int32, ECombatAnimRequestFinishReason>> PendingRemoveRequests;

	for (const auto Pair : ProceedingRequests)
	{
		uint8 Flags = Pair.Value.MontageEventFlags;
		if (!Pair.Value.bIsFinished)
		{
			if (Flags & static_cast<uint8>(EMontageStatusFlag::EMontageStatus_Ended))
			{
				PendingFinishRequests.Add(TPair<int32, ECombatAnimRequestFinishReason>(Pair.Key, ECombatAnimRequestFinishReason::ERequestFinishReason_CompleteNormally));
			}	
		}
	}

	for (const auto Pair : PendingFinishRequests)
	{
		FinishRequest(Pair.Key, Pair.Value);
	}
}
