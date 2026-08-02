#pragma once
#include "Character/Combat/CombatActionValidation.h"

struct FCombatActionTimelineItem;
class UCombatActionStep;
enum class ECombatActionTimelineValidationFlags : uint16;

class FCombatActionEditorValidation
{
public:
	static void ApplyIssuesToTimelineItems(const TArray<FCombatActionValidationIssue>& Issues, TArray<FCombatActionTimelineItem>& Items);

	static bool DoesIssueApplyToItem(const FCombatActionTimelineItem& Item, const FCombatActionValidationIssue& Issue);

	static ECombatActionTimelineValidationFlags BuildValidationFlags(ECombatActionValidationErrorCode ErrorCode);
};
