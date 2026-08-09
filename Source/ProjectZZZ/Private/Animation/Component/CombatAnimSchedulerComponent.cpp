#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Animation/AnimInstanceBase.h"
#include "Character/CharacterBase.h"

UCombatAnimSchedulerComponent::UCombatAnimSchedulerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UCombatAnimSchedulerComponent::BeginPlay()
{
	Super::BeginPlay();
	CachePointers();
}

void UCombatAnimSchedulerComponent::InitializeComponent()
{
	Super::InitializeComponent();
	CachePointers();
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
	if (CheckIfRequestMontageAlreadyPlaying(Request))
	{
		UE_LOG(LogTemp, Warning, TEXT("Request Montage Already Playing"));
		return INDEX_NONE;
	}

	TArray<int32> PendingStopRequests;
	CollectConflictingAnimRequest(Request, PendingStopRequests);
	
	// Add New Request
	int32 NewId = ++NextIDGenerator;
	FCombatAnimExecutionRequest AddedRequest = Request;
	AddedRequest.RequestID = NewId;
	AddedRequest.MontageEventFlags |= static_cast<uint8>(EMontageStatusFlag::EMontageStatus_Started);
	FMontageBlendSettings BlendInSetting = GetMontageBlendInSetting(Request.Montage.Get());

	const float Result = AnimInstance->Montage_PlayWithBlendSettings(
		AddedRequest.Montage.Get(),
		BlendInSetting,
		AddedRequest.PlayRate,
		EMontagePlayReturnType::MontageLength
	);
	
	if (Result <= 0.f)
	{
		return INDEX_NONE;
	}
	
	BindMontageNativeDelegates(AddedRequest.Montage.Get(), AddedRequest.RequestID);
	ProceedingRequests.Add(NewId, AddedRequest);
		
	// Finish All PendingStopRequest
	for (int32 Id : PendingStopRequests)
	{
		FinishRequest(Id, ERequestFinishReason_Interrupted);
	}
	
	return AddedRequest.RequestID;
}

void UCombatAnimSchedulerComponent::CancelAnimRequest(const int32 RequestID)
{
	const FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID);
	if (!IsValid(AnimInstance) || !Request)
	{
		return;
	}

	CancelAnimRequestWithBlendOutSetting(RequestID, GetMontageBlendOutSetting(Request->Montage.Get()));
}

void UCombatAnimSchedulerComponent::CancelAnimRequestWithBlendOutSetting(const int32 RequestID, const FMontageBlendSettings& BlendOutSetting)
{
	const FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID);
	if (!AnimInstance || !Request || !Request->Montage.IsValid())
	{
		return;
	}

	AnimInstance->Montage_StopWithBlendSettings(BlendOutSetting, Request->Montage.Get());
	FinishRequest(RequestID, ERequestFinishReason_Cancelled);
}

bool UCombatAnimSchedulerComponent::RequestMontageSetNextSection(const int32 RequestID, const FName& LoopSectionName, const FName& NextSectionName)
{
	if (!IsValid(AnimInstance))
	{
		return false;
	}

	if (const FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID))
	{
		if (!IsRequestMontageBlendingOut(Request) && Request->Montage.Get())
		{
			AnimInstance->Montage_JumpToSection(LoopSectionName, Request->Montage.Get());
			AnimInstance->Montage_SetNextSection(LoopSectionName, NextSectionName, Request->Montage.Get());
			return true;
		}
	}
	return false;
}

void UCombatAnimSchedulerComponent::CachePointers()
{
	ACharacterBase* CharacterBase = Cast<ACharacterBase>(GetOwner());
	if (!CharacterBase)
	{
		return;
	}
	Character = CharacterBase;
	
	if (CharacterBase->GetMesh())
	{
		AnimInstance = Cast<UAnimInstanceBase>(CharacterBase->GetMesh()->GetAnimInstance());
	}
}

bool UCombatAnimSchedulerComponent::IsRequestMontageBlendingOut(const FCombatAnimExecutionRequest* Request) const
{
	if (!Request || !Request->IsValid())
	{
		return false;
	}
	return Request->MontageEventFlags & static_cast<uint8>(EMontageStatusFlag::EMontageStatus_BlendingOut);
}

bool UCombatAnimSchedulerComponent::CheckIfRequestMontageAlreadyPlaying(const FCombatAnimExecutionRequest& Request) const
{
	if (!Request.Montage.IsValid())
	{
		return false;
	}
	
	for (auto& [Id, ProceedingRequest] : ProceedingRequests)
	{
		if (ProceedingRequest.Montage.IsValid()
			&& ProceedingRequest.Montage == Request.Montage
			&& !IsRequestMontageBlendingOut(&ProceedingRequest))
		{
			return true;
		}
	}
	return false;
}

void UCombatAnimSchedulerComponent::CollectConflictingAnimRequest(const FCombatAnimExecutionRequest& Request, TArray<int32>& OutConflictingRequestIDs) const
{
	if (!Request.Montage.IsValid())
	{
		return;
	}
	
	FName TargetGroupName = GetMontageGroupName(Request.Montage.Get());
	if (TargetGroupName.IsNone())
	{
		return;
	}
	
	for (auto& [Id, ProceedingRequest] : ProceedingRequests)
	{
		if (!ProceedingRequest.Montage.IsValid())
		{
			continue;;
		}
		FName ProceedingGroupName = GetMontageGroupName(ProceedingRequest.Montage.Get());
		if (ProceedingGroupName == TargetGroupName)
		{
			OutConflictingRequestIDs.Add(Id);
		}
	}
}

void UCombatAnimSchedulerComponent::FinishRequest(const int32 RequestID, const ECombatAnimRequestFinishReason Reason)
{
	FCombatAnimExecutionRequest* Request = ProceedingRequests.Find(RequestID);
	if (!Request || Request->bIsFinished)
	{
		return;
	}

	Request->bIsFinished = true;
	ProceedingRequests.Remove(RequestID);
	OnAnimRequestFinished.Broadcast(RequestID, Reason);
}

FName UCombatAnimSchedulerComponent::GetMontageGroupName(const UAnimMontage* Montage) const
{
	if (!IsValid(Montage))
	{
		return NAME_None;
	}
	return Montage->GetGroupName();
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

FMontageBlendSettings UCombatAnimSchedulerComponent::GetMontageBlendOutSetting(const UAnimMontage* Montage) const
{
	FMontageBlendSettings BlendOutSetting;
	if (IsValid(Montage))
	{
		BlendOutSetting.Blend = Montage->BlendOut;
		BlendOutSetting.BlendMode = Montage->BlendModeOut;
		BlendOutSetting.BlendProfile = Montage->BlendProfileOut;
	}
	return BlendOutSetting;
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
	// Nothing to do
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

	const ECombatAnimRequestFinishReason Reason = bInterrupted ? ERequestFinishReason_Interrupted : ERequestFinishReason_CompleteNormally;
	FinishRequest(RequestID, Reason);
}
