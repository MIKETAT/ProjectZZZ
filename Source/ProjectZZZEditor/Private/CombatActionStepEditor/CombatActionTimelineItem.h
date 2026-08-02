#pragma once

UENUM()
enum class ECombatActionTimelineItemType : uint8
{
	None,
	InstantNotify,
	NotifyState
};

enum class ECombatActionTimelineValidationFlags : uint16
{
	None = 0,
	InvalidIdentity								= 1 << 0,
	InvalidTimeRange							= 1 << 1,
	BindingError								= 1 << 2,
	SpecError									= 1 << 3,
	NotifyTypeMismatch							= 1 << 4,
	InvalidDetectionMode						= 1 << 5,
	InvalidModeSetting							= 1 << 6,
};

ENUM_CLASS_FLAGS(ECombatActionTimelineValidationFlags)

struct FCombatActionTimelineItem
{
	FGuid NotifyGuid;

	int32 NotifyIndex{INDEX_NONE};
	
	ECombatActionTimelineItemType ItemType{ECombatActionTimelineItemType::InstantNotify};

	FName SegmentName{NAME_None};

	float StartTime{0.0f};

	float EndTime{0.0f};

	ECombatActionTimelineValidationFlags ValidationFlags{ECombatActionTimelineValidationFlags::None};
};
