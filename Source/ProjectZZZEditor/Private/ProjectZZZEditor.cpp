#include "ProjectZZZEditor.h"

#include "AssetToolsModule.h"
#include "AssetTypeActions/CombatActionStepAssetTypeActions.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "FProjectZZZEditorModule"

void FProjectZZZEditorModule::StartupModule()
{
    UE_LOG(LogTemp, Display, TEXT("ProjectZZZ Editor Module Startup"));

    IAssetTools& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();

    CombatActionStepAssetTypeActions = MakeShared<FCombatActionStepAssetTypeActions>();

    AssetTools.RegisterAssetTypeActions(CombatActionStepAssetTypeActions.ToSharedRef());

    UE_LOG(LogTemp, Display, TEXT("ProjectZZZEditor module started and registered CombatActionStep AssetTypeActions."));
}

void FProjectZZZEditorModule::ShutdownModule()
{

    if (CombatActionStepAssetTypeActions.IsValid())
    {
        if (FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>(TEXT("AssetToolsModule")))
        {
            AssetToolsModule->Get().UnregisterAssetTypeActions(CombatActionStepAssetTypeActions.ToSharedRef());
        }
        CombatActionStepAssetTypeActions.Reset();
    }

    UE_LOG(LogTemp, Display, TEXT("ProjectZZZEditor module shut down."));
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FProjectZZZEditorModule, ProjectZZZEditor)