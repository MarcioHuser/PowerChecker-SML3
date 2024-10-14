#pragma once

#include "Modules/ModuleManager.h"

#include <map>

#include "Buildables/FGBuildableFactory.h"

class FPowerCheckerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	static void onPowerCircuitChangedHook(class UFGPowerCircuit* powerCircuit);
	static void setPendingPotentialCallback(class AFGBuildableFactory* buildable, float potential);
	static void setPendingProductionBoostCallback(class AFGBuildableFactory* buildable, float productionBoost);

	//static std::map<FString, float> powerConsumptionMap;
};
