#include "Character/Combat/CombatStep.h"

#include "Character/Combat/CombatActionValidation.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UCombatActionStep::IsDataValid(class FDataValidationContext& Context) const
{
	TArray<FCombatActionValidationIssue> Issues;
	CombatActionValidation::Validate(*this, Issues);

	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	
	for (const FCombatActionValidationIssue Issue : Issues)
	{
		if (Issue.Severity == ECombatActionValidationSeverity::Warning)
		{
			Context.AddWarning(Issue.Message);
		} else if (Issue.Severity == ECombatActionValidationSeverity::Error)
		{
			Context.AddError(Issue.Message);
			Result = EDataValidationResult::Invalid;
		}
	}
	
	return Result;
}

#endif
