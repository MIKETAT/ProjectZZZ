#include "CombatActionStepAssetTypeActions.h"
#include "FCombatActionStepEditorToolkit.h"
#include "Character/Combat/CombatStep.h"

FText FCombatActionStepAssetTypeActions::GetName() const
{
	return FText::FromString("CombatAction");
}

FColor FCombatActionStepAssetTypeActions::GetTypeColor() const
{
	return FColor(42, 42, 42);
}

UClass* FCombatActionStepAssetTypeActions::GetSupportedClass() const
{
	return UCombatActionStep::StaticClass();
}

uint32 FCombatActionStepAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Gameplay;
}

void FCombatActionStepAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects,
                                                        TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		if (UCombatActionStep* ActionStep = Cast<UCombatActionStep>(Object))
		{
			TSharedPtr<FCombatActionStepEditorToolkit> Toolkit = MakeShared<FCombatActionStepEditorToolkit>();
			Toolkit->InitCombatActionStepEditor(ActionStep);
		}
	}
}

