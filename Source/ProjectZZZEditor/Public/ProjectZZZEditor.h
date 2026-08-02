#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;
class FCombatActionStepAssetTypeActions;

class FProjectZZZEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    
    virtual void ShutdownModule() override;

private:
    TSharedPtr<IAssetTypeActions> CombatActionStepAssetTypeActions{nullptr};
};
