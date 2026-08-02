#if WITH_EDITOR
#include "Character/Combat/CombatActionValidation.h"
#include "Character/Combat/CombatStep.h"
#include "Animation/AnimNotify/AnimNotify_TriggerAttackDetection.h"
#include "Animation/AnimNotifyState/AnimNotifyState_AttackDetection.h"

void CombatActionValidation::Validate(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues)
{
	OutIssues.Reset();
	
	// Detection Switch
	if (!Action.AttackDetectionConfig.bEnableDetection)
	{
		FCombatActionValidationIssue Issue;
		Issue.ErrorCode = ECombatActionValidationErrorCode::DetectionDisabled;
		Issue.Severity = ECombatActionValidationSeverity::Warning;
		Issue.Message = FText::FromString(FString::Printf(TEXT("Detection Disabled.")));
		OutIssues.Add(MoveTemp(Issue));
	}

	// Montage Validation
	if (!ValidateMontage(Action, OutIssues))
	{
		return;
	}

	// Notify Validation
	ValidateAttackNotifies(Action, OutIssues);

	// Binding Validation
	ValidateSegmentBinding(Action, OutIssues);
	
	ValidateNotifySegmentAndRelativeBinding(Action, OutIssues);

	// Spec
	ValidateDetectionSpec(Action, OutIssues);

	// TypeMisMatch
	ValidateDetectionTypeMisMatch(Action, OutIssues);
	
	ValidateDetectionModeSettings(Action, OutIssues);
}

bool CombatActionValidation::ValidateMontage(const UCombatActionStep& Action,
	TArray<FCombatActionValidationIssue>& OutIssues)
{
	UAnimMontage* Montage = Action.Montage.Get();
	if (!Montage)
	{
		FCombatActionValidationIssue Issue;
		Issue.Severity = ECombatActionValidationSeverity::Error;
		Issue.ErrorCode = ECombatActionValidationErrorCode::MissingMontage;
		Issue.Message = FText::FromString(FString::Printf(TEXT("Missing Montage")));
		OutIssues.Add(MoveTemp(Issue));
		return false;
	}

	return true;
}

bool CombatActionValidation::ValidateAttackNotifies(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues)
{
	UAnimMontage* Montage = Action.Montage.Get();
	TMap<FGuid, int32> GuidCount;

	bool bAllValid = true;
	for (int32 Index = 0; Index < Montage->Notifies.Num(); Index++)
	{
		const FAnimNotifyEvent& Event = Montage->Notifies[Index];
		if (Event.Guid.IsValid())
		{
			++GuidCount.FindOrAdd(Event.Guid);
		}
	}
	
	for (int32 Index = 0; Index < Montage->Notifies.Num(); Index++)
	{
		const FAnimNotifyEvent& Event = Montage->Notifies[Index];
		
		if (Cast<UAnimNotify_TriggerAttackDetection>(Event.Notify) || Cast<UAnimNotifyState_AttackDetection>(Event.NotifyStateClass))
		{
			if (!Event.Guid.IsValid())
			{
				FCombatActionValidationIssue Issue;
				Issue.ErrorCode = ECombatActionValidationErrorCode::InvalidNotifyGuid;
				Issue.Severity = ECombatActionValidationSeverity::Error;
				Issue.Message = FText::FromString(FString::Printf(TEXT("Invalid Notify Guid")));
				Issue.NotifyIndex = Index;
				bAllValid = false;
				OutIssues.Add(MoveTemp(Issue));
			} else
			{
				// Check Duplicate when Guid is Valid
				int Count = *GuidCount.Find(Event.Guid);
				if (Count > 1)
				{
					FCombatActionValidationIssue Issue;
					Issue.ErrorCode = ECombatActionValidationErrorCode::DuplicateNotifyGuid;
					Issue.Guid = Event.Guid;
					Issue.NotifyIndex = Index;
					Issue.Severity = ECombatActionValidationSeverity::Error;
					Issue.Message = FText::FromString(FString::Printf(TEXT("Duplicate Notify Guid")));
					bAllValid = false;
					OutIssues.Add(MoveTemp(Issue));
				}
			}

			// Check Time Range
			bool bValidTimeRange = true;
			bool bValidSegmentName = true;
			const float MontageLength = Montage->GetPlayLength();
			if (UAnimNotify_TriggerAttackDetection* Notify = Cast<UAnimNotify_TriggerAttackDetection>(Event.Notify))
			{
				const float StartTime = Event.GetTime();
				if (StartTime < 0.f || StartTime > MontageLength)
				{
					bValidTimeRange = false;
				}

				if (Notify->SegmentName.IsNone())
				{
					bValidSegmentName = false;
				}
			} else if (UAnimNotifyState_AttackDetection* State = Cast<UAnimNotifyState_AttackDetection>(Event.NotifyStateClass)) {
				const float StartTime = Event.GetTime();
				const float Duration = Event.GetDuration();
				if (StartTime < 0.f || Duration <= UE_KINDA_SMALL_NUMBER || StartTime + Duration > MontageLength)
				{
					bValidTimeRange = false;
				}

				if (State->SegmentName.IsNone())
				{
					bValidSegmentName = false;
				}
			}

			if (!bValidTimeRange)
			{
				FCombatActionValidationIssue Issue;
				Issue.ErrorCode = ECombatActionValidationErrorCode::InvalidNotifyTimeRange;
				Issue.Severity = ECombatActionValidationSeverity::Error;
				Issue.Message = FText::FromString(FString::Printf(TEXT("Invalid Notify Time Range")));
				Issue.Guid = Event.Guid;
				Issue.NotifyIndex = Index;
				bAllValid = false;
				OutIssues.Add(MoveTemp(Issue));
			}

			// Check SegmentName
			if (!bValidSegmentName)
			{
				FCombatActionValidationIssue Issue;
				Issue.ErrorCode = ECombatActionValidationErrorCode::InvalidNotifySegmentName;
				Issue.Severity = ECombatActionValidationSeverity::Error;
				Issue.Guid = Event.Guid;
				Issue.NotifyIndex = Index;
				Issue.Message = FText::FromString(FString::Printf(TEXT("Invalid Segment Name")));
				bAllValid = false;
				OutIssues.Add(MoveTemp(Issue));
			}
		}
	}
	
	return bAllValid;
}

bool CombatActionValidation::ValidateSegmentBinding(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues)
{
	bool bAllValid = true;
	const TArray<FAttackDetectionSegmentBinding>& SegmentBindings =  Action.AttackDetectionConfig.Segments;

	TMap<FName, int32> SegmentBindingCount;

	// Check Segment Name
	for (int32 Index = 0; Index < SegmentBindings.Num(); ++Index)
	{
		const FAttackDetectionSegmentBinding& SegmentBinding = SegmentBindings[Index];
		if (SegmentBinding.SegmentName.IsNone())
		{
			FCombatActionValidationIssue Issue;
			Issue.ErrorCode = ECombatActionValidationErrorCode::InvalidBindingSegmentName;
			Issue.Severity = ECombatActionValidationSeverity::Error;
			Issue.Message = FText::FromString(FString::Printf(TEXT("Invalid Binding Segment Name")));
			bAllValid = false;
			OutIssues.Add(MoveTemp(Issue));
			continue;
		}

		++SegmentBindingCount.FindOrAdd(SegmentBinding.SegmentName);
	}

	// Check Duplicate Segment Name
	for (const TPair<FName, int32>& Pair : SegmentBindingCount)
	{
		if (Pair.Value > 1)
		{
			FCombatActionValidationIssue Issue;
			Issue.ErrorCode = ECombatActionValidationErrorCode::DuplicateBinding;
			Issue.Severity = ECombatActionValidationSeverity::Error;
			Issue.SegmentName = Pair.Key;
			Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
			Issue.Message = FText::FromString(FString::Printf(TEXT("Duplicate SegmentBinding")));
			bAllValid = false;
			OutIssues.Add(MoveTemp(Issue));
		}
	}
	
	return bAllValid;
}

bool CombatActionValidation::ValidateNotifySegmentAndRelativeBinding(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues)
{
	bool bAllValid = true;
	const TArray<FAttackDetectionSegmentBinding>& SegmentBindings =  Action.AttackDetectionConfig.Segments;
	TMap<FName, int32> SegmentBindingCount;

	for (int32 Index = 0; Index < SegmentBindings.Num(); ++Index)
	{
		const FAttackDetectionSegmentBinding& SegmentBinding = SegmentBindings[Index];
		if (!SegmentBinding.SegmentName.IsNone())
		{
			++SegmentBindingCount.FindOrAdd(SegmentBinding.SegmentName);
		}
	}

	UAnimMontage* Montage = Action.Montage.Get();
	if (!Montage)
	{
		return false;
	}
	
	for (int32 Index = 0; Index < Montage->Notifies.Num(); ++Index)
	{
		const FAnimNotifyEvent& Event = Montage->Notifies[Index];
		FName NotifySegmentName{NAME_None};
		if (UAnimNotify_TriggerAttackDetection* Notify = Cast<UAnimNotify_TriggerAttackDetection>(Event.Notify))
		{
			NotifySegmentName = Notify->SegmentName;
		} else if (UAnimNotifyState_AttackDetection* State = Cast<UAnimNotifyState_AttackDetection>(Event.NotifyStateClass))
		{
			NotifySegmentName = State->SegmentName;
		}

		if (NotifySegmentName.IsNone())
		{
			continue;
		}

		const int32 BindingCount = SegmentBindingCount.FindRef(NotifySegmentName);
		if (BindingCount == 0)
		{
			// MissingBinding
			FCombatActionValidationIssue Issue;
			Issue.ErrorCode = ECombatActionValidationErrorCode::MissingBinding;
			Issue.Severity = ECombatActionValidationSeverity::Error;
			Issue.SegmentName = NotifySegmentName;
			Issue.Guid = Event.Guid;
			Issue.NotifyIndex = Index;
			Issue.Message = FText::FromString(FString::Printf(TEXT("Missing Valid Binding")));
			bAllValid = false;
			OutIssues.Add(MoveTemp(Issue));
		}
	}
	
	return bAllValid;
}

bool CombatActionValidation::ValidateDetectionSpec(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues)
{
	bool bAllValid = true;
	const TArray<FAttackDetectionSegmentBinding>& SegmentBindings = Action.AttackDetectionConfig.Segments;
	UAnimMontage* Montage = Action.Montage.Get();
	
	for (const FAttackDetectionSegmentBinding& SegmentBinding : SegmentBindings)
	{
		FAttackDetectionSpec Spec;
		bool bResolvedSuccess = SegmentBinding.ResolveDetectionSpec(Spec);
		FCombatActionValidationIssue Issue;
		bool bValidSegment = true;
		
		if (SegmentBinding.SpecSource == EAttackDetectorSpecSource::Preset && !IsValid(SegmentBinding.Preset))
		{
			Issue.ErrorCode = ECombatActionValidationErrorCode::MissingPreset;
			Issue.Severity = ECombatActionValidationSeverity::Error;
			Issue.SegmentName = SegmentBinding.SegmentName;
			Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
			Issue.Message = FText::FromString(TEXT("Missing Preset"));
			bAllValid = false;
			bValidSegment = false;
			OutIssues.Add(MoveTemp(Issue));
		} else if (!bResolvedSuccess)
		{
			Issue.ErrorCode = ECombatActionValidationErrorCode::UnResolvedSpec;
			Issue.Severity = ECombatActionValidationSeverity::Error;
			Issue.SegmentName = SegmentBinding.SegmentName;
			Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
			Issue.Message = FText::FromString(TEXT("UnResolved Spec"));
			bAllValid = false;
			bValidSegment = false;
			OutIssues.Add(MoveTemp(Issue));
		}
	}
	
	return bAllValid;
}

bool CombatActionValidation::ValidateDetectionModeSettings(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues)
{
	bool bAllValid = true;
	const TArray<FAttackDetectionSegmentBinding>& SegmentBindings = Action.AttackDetectionConfig.Segments;
	for (const FAttackDetectionSegmentBinding& SegmentBinding : SegmentBindings)
	{
		FAttackDetectionSpec Spec;
		if (!SegmentBinding.ResolveDetectionSpec(Spec))
		{
			continue;
		}

		switch (Spec.DetectionMode)
		{
			case EAttackDetectionMode::ShapeQueryInstant:
				{
					// ReferenceType and ReferenceSocket Validation
					if (Spec.ReferenceType == EAttackQueryReference::None)
					{
						FCombatActionValidationIssue Issue;
						Issue.ErrorCode = ECombatActionValidationErrorCode::InvalidReferenceType;
						Issue.Severity = ECombatActionValidationSeverity::Error;
						Issue.SegmentName = SegmentBinding.SegmentName;
						Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
						Issue.Message = FText::FromString(TEXT("Invalid Reference Type"));
						bAllValid = false;
						OutIssues.Add(MoveTemp(Issue));
					} else if (Spec.ReferenceType == EAttackQueryReference::OwnerSocket && Spec.ReferenceSocketName.IsNone())
					{
						FCombatActionValidationIssue Issue;
						Issue.ErrorCode = ECombatActionValidationErrorCode::MissingReferenceSocketName;
						Issue.Severity = ECombatActionValidationSeverity::Error;
						Issue.SegmentName = SegmentBinding.SegmentName;
						Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
						Issue.Message = FText::FromString(TEXT("Missing Reference Socket Name"));
						bAllValid = false;
						OutIssues.Add(MoveTemp(Issue));	
					}
		
					// DedupePolicy
					if (SegmentBinding.DedupePolicy == EHitDedupePolicy::OncePerActivation)	// Not for InstantQuery. Activation means StateNotify
					{
						FCombatActionValidationIssue DedupeIssue;
						DedupeIssue.ErrorCode = ECombatActionValidationErrorCode::InvalidDedupePolicy;
						DedupeIssue.Severity = ECombatActionValidationSeverity::Error;
						DedupeIssue.SegmentName = SegmentBinding.SegmentName;
						DedupeIssue.NotifyIndex = GetNotifyIndexBySegmentName(Action, DedupeIssue.SegmentName);
						DedupeIssue.Message = FText::FromString(TEXT("Invalid Dedupe Policy"));
						bAllValid = false;
						OutIssues.Add(MoveTemp(DedupeIssue));
					}
				}
				break;
			case EAttackDetectionMode::WeaponSweep:
				{
					if (Spec.WeaponRootSocketName.IsNone())
					{
						FCombatActionValidationIssue Issue;
						Issue.ErrorCode = ECombatActionValidationErrorCode::MissingWeaponRootSocketName;
						Issue.Severity = ECombatActionValidationSeverity::Error;
						Issue.SegmentName = SegmentBinding.SegmentName;
						Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
						Issue.Message = FText::FromString(FString::Printf(TEXT("Missing Weapon Root Socket Name")));
						bAllValid = false;
						OutIssues.Add(MoveTemp(Issue));
					}
					if (Spec.WeaponTipSocketName.IsNone())
					{
						FCombatActionValidationIssue Issue;
						Issue.ErrorCode = ECombatActionValidationErrorCode::MissingWeaponTipSocketName;
						Issue.Severity = ECombatActionValidationSeverity::Error;
						Issue.SegmentName = SegmentBinding.SegmentName;
						Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
						Issue.Message = FText::FromString(FString::Printf(TEXT("Missing Weapon Tip SocketName.")));
						bAllValid = false;
						OutIssues.Add(MoveTemp(Issue));
					}
				}
				break;
			case EAttackDetectionMode::ActorPathSweep:
				// No need to validate for now.
				break;
			case EAttackDetectionMode::None:
				{
					FCombatActionValidationIssue Issue;
					Issue.ErrorCode = ECombatActionValidationErrorCode::MissingDetectionMode;
					Issue.Severity = ECombatActionValidationSeverity::Error;
					Issue.SegmentName = SegmentBinding.SegmentName;
					Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
					Issue.Message = FText::FromString(TEXT("Missing DetectionMode"));
					bAllValid = false;
					OutIssues.Add(MoveTemp(Issue));
				}
				break;
			default:
			{
				// ShapeContinuous
				FCombatActionValidationIssue Issue;
				Issue.ErrorCode = ECombatActionValidationErrorCode::UnsupportedDetectionMode;
				Issue.Severity = ECombatActionValidationSeverity::Error;
				Issue.SegmentName = SegmentBinding.SegmentName;	
				Issue.NotifyIndex = GetNotifyIndexBySegmentName(Action, Issue.SegmentName);
				Issue.Message = FText::FromString(TEXT("UnSupported Detection Mode"));
				bAllValid = false;
				OutIssues.Add(MoveTemp(Issue));
			}
			break;
		}
	}
	
	return bAllValid;
}

bool CombatActionValidation::ValidateDetectionTypeMisMatch(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues)
{
	bool bAllValid = true;
	UAnimMontage* Montage = Action.Montage.Get();
	
	for (int32 Index = 0; Index < Montage->Notifies.Num(); Index++)
	{
		const FAnimNotifyEvent& Event = Montage->Notifies[Index];

		if (!Cast<UAnimNotify_TriggerAttackDetection>(Event.Notify) && !Cast<UAnimNotifyState_AttackDetection>(Event.NotifyStateClass))
		{
			continue;
		}

		// Check Type MissMatch
		const TArray<FAttackDetectionSegmentBinding>& SegmentBindings =  Action.AttackDetectionConfig.Segments;
		const FGuid Guid = Event.Guid;
		
		bool bNotifyState = true;
		FName NotifySegmentName{NAME_None};
		if (UAnimNotify_TriggerAttackDetection* Notify = Cast<UAnimNotify_TriggerAttackDetection>(Event.Notify))
		{
			bNotifyState = false;
			NotifySegmentName = Notify->SegmentName;
		} else if (UAnimNotifyState_AttackDetection* State = Cast<UAnimNotifyState_AttackDetection>(Event.NotifyStateClass))
		{
			NotifySegmentName = State->SegmentName;
		}

		for (const FAttackDetectionSegmentBinding& Binding : SegmentBindings)
		{
			if (Binding.SegmentName == NotifySegmentName)
			{
				FAttackDetectionSpec Spec;
				if (!Binding.ResolveDetectionSpec(Spec))
				{
					continue;	
				}
					
				if ((bNotifyState && Spec.DetectionMode == EAttackDetectionMode::ShapeQueryInstant)
					|| (!bNotifyState && (Spec.DetectionMode == EAttackDetectionMode::ActorPathSweep || Spec.DetectionMode == EAttackDetectionMode::WeaponSweep)))
				{
					FCombatActionValidationIssue Issue;
					Issue.ErrorCode = ECombatActionValidationErrorCode::NotifyTypeMismatch;
					Issue.Severity = ECombatActionValidationSeverity::Error;
					Issue.SegmentName = NotifySegmentName;
					Issue.NotifyIndex = Index;
					Issue.Message = FText::FromString(FString::Printf(TEXT("Notify Type and Detection Mode Mismatch.")));
					Issue.Guid = Guid;
					bAllValid = false;
					OutIssues.Add(MoveTemp(Issue));		
				}
			}
		}
	}

	return bAllValid;
}

int32 CombatActionValidation::GetNotifyIndexBySegmentName(const UCombatActionStep& Action, const FName& SegmentName)
{
	UAnimMontage* Montage = Action.Montage.Get();
	if (!Montage || SegmentName.IsNone())
	{
		return INDEX_NONE;
	}

	FName Name{NAME_None};
	for (int32 Index  = 0; Index < Montage->Notifies.Num(); Index++)
	{
		const FAnimNotifyEvent& Event = Montage->Notifies[Index];
		if (UAnimNotify_TriggerAttackDetection* Notify = Cast<UAnimNotify_TriggerAttackDetection>(Event.Notify))
		{
			Name = Notify->SegmentName;
		} else if (UAnimNotifyState_AttackDetection* State = Cast<UAnimNotifyState_AttackDetection>(Event.NotifyStateClass)) 
		{
			Name = State->SegmentName;
		}

		if (Name == SegmentName)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

#endif
