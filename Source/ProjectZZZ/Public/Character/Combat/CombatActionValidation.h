#pragma once

#if WITH_EDITOR

class UCombatActionStep;

enum class ECombatActionValidationErrorCode : uint8
{
	// Asset
	MissingMontage,

	// Switch
	DetectionDisabled,

	// for one NotifyItem
	InvalidNotifyGuid,
	DuplicateNotifyGuid,
	InvalidNotifyTimeRange,
	InvalidNotifySegmentName,

	// for one Binding
	InvalidBindingSegmentName,
	MissingBinding,
	DuplicateBinding,

	// for one Binding
	MissingPreset,
	UnResolvedSpec,

	InvalidReferenceType,
	MissingReferenceSocketName,
	MissingWeaponRootSocketName,
	MissingWeaponTipSocketName,
	InvalidDedupePolicy,
	MissingDetectionMode,
	UnsupportedDetectionMode,	// ShapeContinuous

	// for one NotifyItem
	NotifyTypeMismatch,
};

enum class ECombatActionValidationSeverity : uint8
{
	Warning,
	Error
};

struct PROJECTZZZ_API FCombatActionValidationIssue
{
	ECombatActionValidationErrorCode ErrorCode;

	ECombatActionValidationSeverity Severity;

	FGuid Guid;

	FName SegmentName{NAME_None};

	int32 NotifyIndex{INDEX_NONE};

	int32 BindingIndex{INDEX_NONE};

	FText Message;
};

namespace CombatActionValidation
{
	PROJECTZZZ_API void Validate(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues);

	bool ValidateMontage(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues);

	bool ValidateAttackNotifies(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues);

	bool ValidateSegmentBinding(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues);

	PROJECTZZZ_API bool ValidateNotifySegmentAndRelativeBinding(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues);

	bool ValidateDetectionSpec(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues);

	bool ValidateDetectionModeSettings(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues);

	bool ValidateDetectionTypeMisMatch(const UCombatActionStep& Action, TArray<FCombatActionValidationIssue>& OutIssues);

	int32 GetNotifyIndexBySegmentName(const UCombatActionStep& Action, const FName& SegmentName);
}

#endif
