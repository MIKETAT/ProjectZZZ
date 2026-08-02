#pragma once
#include "CombatActionTimelineItem.h"
#include "Character/Combat/AttackDetection.h"
#include "Character/Combat/CombatActionValidation.h"

enum class ECombatActionTimelineItemType : uint8;

enum class ECombatTimelineHitPart : uint8
{
	None,
	InstantHandle,
	StateBody,
	StateStartHandle,
	StateEndHandle
};

struct FCombatTimelineHitResult
{
	bool IsValid() const
	{
		return NotifyGuid.IsValid() && HitPart != ECombatTimelineHitPart::None;
	}

	void Reset()
	{
		NotifyGuid = FGuid();
		HitPart = ECombatTimelineHitPart::None;
	}
	
	FGuid NotifyGuid;

	ECombatTimelineHitPart HitPart{ECombatTimelineHitPart::None};
};

enum class ECombatTimelineItemGeometryType : uint8
{
	Invalid,
	InstantNotifyGeometry,
	StateNotifyGeometry
};

struct FCombatTimelineItemGeometry
{
	FSlateRect BodyRect;
	FSlateRect StartHandleRect;
	FSlateRect EndHandleRect;

	float StartAnchorX{0.f};
	float EndAnchorX{0.f};
	
	ECombatTimelineItemGeometryType GeometryType{ECombatTimelineItemGeometryType::Invalid};
};

enum class ECombatActionSegmentSelectionStatus : uint8
{
	NoSelection,
	NotifyNotFound,
	UnsupportedNotifyType,
	Selected
};

enum class ECombatActionSegmentResolveStatus : uint8
{
	NotApplicable,
	MissingBinding,
	DuplicateBinding,
	UnresolvedSpec,
	HasInvalidBinding,
	ResolvedSuccessfully,
};

struct FCombatActionSegmentSelectionContext
{
	void Reset()
	{
		*this = FCombatActionSegmentSelectionContext{};
	}

	bool HasResolvedConfig() const { return ResolveStatus == ECombatActionSegmentResolveStatus::ResolvedSuccessfully;}
	
	ECombatActionSegmentSelectionStatus SelectionStatus{ECombatActionSegmentSelectionStatus::NoSelection};

	ECombatActionSegmentResolveStatus ResolveStatus{ECombatActionSegmentResolveStatus::NotApplicable};

	FGuid NotifyGuid;

	FName SegmentName{NAME_None};

	ECombatActionTimelineItemType ItemType{ECombatActionTimelineItemType::InstantNotify};

	float StartTime{0.f};

	float EndTime{0.f};

	int32 BindingCount{0};
	
	bool bEnableDetection{false};
	
	TOptional<FAttackDetectionSpec> ResolvedSpec;

	ECombatActionTimelineValidationFlags ValidationFlags{ECombatActionTimelineValidationFlags::None};

	TArray<FCombatActionValidationIssue> Issues;

	TArray<FCombatActionValidationIssue> AllIssues;
};

struct FAttackDetectionVisualizationRequest
{
	void Reset()
	{
		*this = FAttackDetectionVisualizationRequest{};
	}
	
	bool bEnable{false};

	FAttackDetectionSpec Spec;

	float StartTime{0.f};

	float EndTime{0.f};

	TWeakObjectPtr<USkeletalMeshComponent> PreviewMesh{nullptr};

	TWeakObjectPtr<USkeletalMeshComponent> SamplingMesh{nullptr};

	TWeakObjectPtr<UAnimMontage> Montage{nullptr};
};

// Delegates
DECLARE_DELEGATE_OneParam(FOnCombatTimelineItemSelected, const FCombatTimelineHitResult&);

DECLARE_DELEGATE_TwoParams(FOnCombatTimelineItemDragged, const FCombatTimelineHitResult&, const float);

DECLARE_DELEGATE(FOnCombatTimelineItemDragFinished);