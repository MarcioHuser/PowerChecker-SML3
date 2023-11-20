#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "PowerChecker_ConfigStruct.generated.h"

/* Struct generated from Mod Configuration Asset '/PowerChecker/Configuration/PowerChecker_Config' */
USTRUCT(BlueprintType)
struct FPowerChecker_ConfigStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    int32 logLevel;

    UPROPERTY(BlueprintReadWrite)
    float maximumPlayerDistance;

    UPROPERTY(BlueprintReadWrite)
    float spareLimit;

    UPROPERTY(BlueprintReadWrite)
    float overflowBlinkCycle;

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FPowerChecker_ConfigStruct GetActiveConfig(UObject* WorldContext) {
        FPowerChecker_ConfigStruct ConfigStruct{};
        FConfigId ConfigId{"EfficiencyCheckerMod", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull))
        {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FPowerChecker_ConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

