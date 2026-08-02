#include "FCombatActionEditorValidation.h"
#include "CombatActionTimelineItem.h"

void FCombatActionEditorValidation::ApplyIssuesToTimelineItems(const TArray<FCombatActionValidationIssue>& Issues,
	TArray<FCombatActionTimelineItem>& Items)
{
	for (FCombatActionTimelineItem& Item : Items)
	{
		Item.ValidationFlags = ECombatActionTimelineValidationFlags::None;

		for (const FCombatActionValidationIssue& Issue : Issues)
		{
			if (!DoesIssueApplyToItem(Item, Issue))
			{
				continue;
			}

			Item.ValidationFlags |= BuildValidationFlags(Issue.ErrorCode);
		}
	}
}

bool FCombatActionEditorValidation::DoesIssueApplyToItem(const FCombatActionTimelineItem& Item, const FCombatActionValidationIssue& Issue)
{
	return Item.NotifyIndex == Issue.NotifyIndex || Item.SegmentName == Issue.SegmentName;
	/*switch (Issue.ErrorCode)
	{
		case ECombatActionValidationErrorCode::InvalidNotifyGuid:
		case ECombatActionValidationErrorCode::DuplicateNotifyGuid:
		case ECombatActionValidationErrorCode::InvalidNotifyTimeRange:
		case ECombatActionValidationErrorCode::InvalidNotifySegmentName:
		case ECombatActionValidationErrorCode::MissingBinding:
		case ECombatActionValidationErrorCode::NotifyTypeMismatch:
			return Issue.NotifyIndex == Item.NotifyIndex;
		
			
		case ECombatActionValidationErrorCode::DuplicateBinding:
		case ECombatActionValidationErrorCode::UnResolvedSpec:
		case ECombatActionValidationErrorCode::MissingPreset:
		case ECombatActionValidationErrorCode::MissingDetectionMode:
		case ECombatActionValidationErrorCode::UnsupportedDetectionMode:
		case ECombatActionValidationErrorCode::InvalidDedupePolicy:
		case ECombatActionValidationErrorCode::InvalidReferenceType:
		case ECombatActionValidationErrorCode::MissingReferenceSocketName:
		case ECombatActionValidationErrorCode::MissingWeaponRootSocketName:
		case ECombatActionValidationErrorCode::MissingWeaponTipSocketName:
			return Issue.SegmentName == Item.SegmentName;
		default:
			return false;
	}*/
}

ECombatActionTimelineValidationFlags FCombatActionEditorValidation::BuildValidationFlags(ECombatActionValidationErrorCode ErrorCode)
{
	ECombatActionTimelineValidationFlags Flags{ECombatActionTimelineValidationFlags::None};
	switch (ErrorCode)
	{
		case ECombatActionValidationErrorCode::InvalidNotifyGuid:
		case ECombatActionValidationErrorCode::DuplicateNotifyGuid:
			Flags |= ECombatActionTimelineValidationFlags::InvalidIdentity;
			break;
		case ECombatActionValidationErrorCode::InvalidNotifyTimeRange:
			Flags |= ECombatActionTimelineValidationFlags::InvalidTimeRange;
			break;
		case ECombatActionValidationErrorCode::InvalidNotifySegmentName:
		case ECombatActionValidationErrorCode::MissingBinding:
		case ECombatActionValidationErrorCode::DuplicateBinding:
			Flags |= ECombatActionTimelineValidationFlags::BindingError;
			break;
		case ECombatActionValidationErrorCode::UnResolvedSpec:
		case ECombatActionValidationErrorCode::MissingPreset:
			Flags |= ECombatActionTimelineValidationFlags::SpecError;
			break;
		case ECombatActionValidationErrorCode::NotifyTypeMismatch:
			Flags |= ECombatActionTimelineValidationFlags::NotifyTypeMismatch;
			break;
		case ECombatActionValidationErrorCode::MissingDetectionMode:
		case ECombatActionValidationErrorCode::UnsupportedDetectionMode:
			Flags |= ECombatActionTimelineValidationFlags::InvalidDetectionMode;
			break;
		case ECombatActionValidationErrorCode::InvalidDedupePolicy:
		case ECombatActionValidationErrorCode::MissingReferenceSocketName:
		case ECombatActionValidationErrorCode::MissingWeaponRootSocketName:
		case ECombatActionValidationErrorCode::MissingWeaponTipSocketName:
		case ECombatActionValidationErrorCode::InvalidReferenceType:
		case ECombatActionValidationErrorCode::InvalidBindingSegmentName:
			Flags |= ECombatActionTimelineValidationFlags::InvalidModeSetting;
			break;
		case ECombatActionValidationErrorCode::MissingMontage:
		case ECombatActionValidationErrorCode::DetectionDisabled:
			break;
		default:
			break;
	}
	return Flags;
}
