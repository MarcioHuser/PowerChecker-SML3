#include "Logic/PowerCheckerLogic.h"
#include "PowerCheckerModule.h"
#include "PowerCheckerRCO.h"
#include "Util/Logging.h"
#include "Util/Optimize.h"

#include "Buildables/FGBuildablePowerPole.h"
#include "FGCircuitSubsystem.h"
#include "FGDropPod.h"
#include "FGFactoryConnectionComponent.h"
#include "FGGameMode.h"
#include "FGLocomotive.h"
#include "FGPowerCircuit.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
// #include "FGRailroadSubsystem.h"
// #include "FGRailroadVehicle.h"
// #include "FGTrain.h"
// #include "FGTrainStationIdentifier.h"

#include <map>

#include "Buildables/FGBuildableGenerator.h"
#include "Buildables/FGBuildablePowerStorage.h"

#ifndef OPTIMIZE
#pragma optimize( "", off )
#endif

APowerCheckerLogic* APowerCheckerLogic::singleton = nullptr;
FPowerChecker_ConfigStruct APowerCheckerLogic::configuration;
TSubclassOf<class UFGItemDescriptor> APowerCheckerLogic::dropPodStub = nullptr;

APowerCheckerLogic::APowerCheckerLogic()
{
}

void APowerCheckerLogic::Initialize(TSubclassOf<UFGItemDescriptor> in_dropPodStub)
{
	singleton = this;

	auto subsystem = AFGBuildableSubsystem::Get(this);

	dropPodStub = in_dropPodStub;

	if (subsystem)
	{
		FOnBuildableConstructedGlobal::FDelegate constructedDelegate;

		constructedDelegate.BindDynamic(this, &APowerCheckerLogic::OnFGBuildableSubsystemBuildableConstructed);

		subsystem->BuildableConstructedGlobalDelegate.Add(constructedDelegate);

		TArray<AActor*> allBuildables;
		UGameplayStatics::GetAllActorsOfClass(subsystem->GetWorld(), AFGBuildable::StaticClass(), allBuildables);

		// removeTeleporterDelegate.BindDynamic(this, &APowerCheckerLogic::removeTeleporter);
		removePowerCheckerDelegate.BindDynamic(this, &APowerCheckerLogic::removePowerChecker);

		for (auto buildableActor : allBuildables)
		{
			IsValidBuildable(Cast<AFGBuildable>(buildableActor));
		}

		// auto gameMode = Cast<AFGGameMode>(UGameplayStatics::GetGameMode(subsystem->GetWorld()));
		// if (gameMode)
		// {
		// 	gameMode->RegisterRemoteCallObjectClass(UPowerCheckerRCO::StaticClass());
		// }
	}
}

void APowerCheckerLogic::OnFGBuildableSubsystemBuildableConstructed(AFGBuildable* buildable)
{
	IsValidBuildable(buildable);
}

void APowerCheckerLogic::Terminate()
{
	FScopeLock ScopeLock(&eclCritical);
	// allTeleporters.Empty();

	singleton = nullptr;
}

void APowerCheckerLogic::GetMaximumPotential
(
	UFGPowerConnectionComponent* powerConnection,
	float& totalMaximumPotential,
	bool includePaused,
	bool includeOutOfFuel,
	PowerCheckerFilterType filterType
)
{
	TArray<FPowerDetail> dummyPowerDetails;

	GetMaximumPotentialWithDetails(
		powerConnection,
		totalMaximumPotential,
		includePaused,
		includeOutOfFuel,
		false,
		dummyPowerDetails,
		dummyPowerDetails,
		dummyPowerDetails,
		filterType
		);
}

void APowerCheckerLogic::GetMaximumPotentialWithDetails
(
	UFGPowerConnectionComponent* powerConnection,
	float& totalMaximumPotential,
	bool includePaused,
	bool includeOutOfFuel,
	bool includePowerDetails,
	TArray<FPowerDetail>& outGeneratorDetails,
	TArray<FPowerDetail>& outPowerStorageDetails,
	TArray<FPowerDetail>& outConsumerDetails,
	PowerCheckerFilterType filterType
)
{
	totalMaximumPotential = 0;

	TSet<AActor*> seenActors;
	std::map<TSubclassOf<UFGItemDescriptor>, std::map<float, std::map<float, FPowerDetail>>> generatorDetails;
	std::map<TSubclassOf<UFGItemDescriptor>, std::map<float, std::map<float, FPowerDetail>>> powerStorageDetails;
	std::map<TSubclassOf<UFGItemDescriptor>, std::map<float, std::map<float, FPowerDetail>>> consumerDetails;

	// auto railroadSubsystem = AFGRailroadSubsystem::Get(powerConnection->GetWorld());
	auto currentPowerCircuit = powerConnection->GetPowerCircuit();
	if (!currentPowerCircuit)
	{
		return;
	}

	TArray<UFGPowerCircuit*> circuits;

	auto circuitGroupId = currentPowerCircuit->GetCircuitGroupID();
	if (circuitGroupId < 0)
	{
		circuits.Add(currentPowerCircuit);
	}
	else
	{
		auto powerCircuitSubsystem = AFGCircuitSubsystem::Get(powerConnection->GetWorld());
		auto powerGroup = Cast<UFGPowerCircuitGroup>(powerCircuitSubsystem->GetCircuitGroup(currentPowerCircuit->GetCircuitGroupID()));
		if (powerGroup)
		{
			circuits = powerGroup->mCircuits;
		}
	}

	for (auto powerCircuit : circuits)
	{
		for (auto powerInfo : powerCircuit->mPowerInfos)
		{
			auto nextActor = powerInfo->GetOwner();
			if (!nextActor || seenActors.Contains(nextActor) || Cast<AFGBuildablePowerPole>(nextActor) || Cast<APowerCheckerBuilding>(nextActor))
			{
				continue;
			}

			seenActors.Add(nextActor);

			auto className = nextActor->GetClass()->GetPathName();
			auto buildable = Cast<AFGBuildable>(nextActor);
			auto buildDescriptor = buildable ? buildable->GetBuiltWithDescriptor() : nullptr;

			auto generator = Cast<AFGBuildableGenerator>(nextActor);
			auto factory = Cast<AFGBuildableFactory>(nextActor);
			auto powerStorage = Cast<AFGBuildablePowerStorage>(nextActor);

			if (IS_PC_LOG_LEVEL(ELogVerbosity::Log))
			{
				PC_LOG_Display(
					TEXT(" PowerChecker: "),
					*nextActor->GetName(),
					TEXT(" / "),
					*className
					);

				PC_LOG_Display(TEXT("    Base Production: "), powerInfo->GetBaseProduction());
				PC_LOG_Display(TEXT("    Dynamic Production Capacity: "), powerInfo->GetDynamicProductionCapacity());
				PC_LOG_Display(TEXT("    Target Consumption: "), powerInfo->GetTargetConsumption());
			}

			/*if (className == TEXT("/Game/StorageTeleporter/Buildables/Hub/Build_STHub.Build_STHub_C"))
			{
			if (filterType != PowerCheckerFilterType::Any)
			{
			continue;
			}

			totalMaximumPotential += powerInfo->GetTargetConsumption();

			if (configuration.logInfoEnabled)
			{
			PC_LOG_Display( TEXT("    Item Teleporter Power Comsumption: "), powerInfo->GetTargetConsumption());
			}

			if (buildDescriptor && includePowerDetails)
			{
			buildingDetails[buildDescriptor]
			[-powerInfo->GetTargetConsumption()]
			[100].amount++;
			}
			}
			else*/
			if (auto locomotive = Cast<AFGLocomotive>(nextActor))
			{
				if (filterType != PowerCheckerFilterType::Any)
				{
					continue;
				}

				totalMaximumPotential += locomotive->GetPowerInfo()->GetMaximumTargetConsumption();

				if (includePowerDetails)
				{
					consumerDetails[UFGRecipe::GetProducts(locomotive->GetBuiltWithRecipe())[0].ItemClass]
						[locomotive->GetPowerInfo()->GetMaximumTargetConsumption()]
						[100].amount++;
				}
			}
			else if (auto dropPod = Cast<AFGDropPod>(nextActor))
			{
				if (filterType != PowerCheckerFilterType::Any)
				{
					continue;
				}

				// dumpUnknownClass(dropPod);
				// dumpUnknownClass(powerInfo);

				if (powerInfo->GetTargetConsumption())
				{
					PC_LOG_Display_Condition(ELogVerbosity::Log, TEXT("    Target Consumption: "), powerInfo->GetTargetConsumption());

					totalMaximumPotential += powerInfo->GetTargetConsumption();

					if (includePowerDetails && dropPodStub)
					{
						consumerDetails[dropPodStub]
							[powerInfo->GetTargetConsumption()]
							[100].amount++;
					}
				}
			}
			else if (generator || powerInfo->GetDynamicProductionCapacity())
			{
				if (IS_PC_LOG_LEVEL(ELogVerbosity::Log))
				{
					PC_LOG_Display(TEXT("    Can Produce: "), (generator && generator->CanProduce()) ? TEXT("true") : TEXT("false"));
					PC_LOG_Display(TEXT("    Load: "), generator ? generator->GetLoadPercentage() : 0, TEXT("%"));
					PC_LOG_Display(TEXT("    Power Production: "), generator ? generator->GetPowerProductionCapacity() : 0);
					PC_LOG_Display(TEXT("    Default Power Production: "), generator ? generator->GetDefaultPowerProductionCapacity() : 0);
					PC_LOG_Display(TEXT("    Pending Potential: "), generator ? generator->GetPendingPotential() : 0);
					PC_LOG_Display(
						TEXT("    Producing Power Production Capacity For Potential: "),
						generator ? generator->CalcPowerProductionCapacityForPotential(generator->GetPendingPotential()) : 0
						);
				}

				auto producedPower = generator ? generator->GetPowerProductionCapacity() : 0;
				if (!producedPower)
				{
					producedPower = powerInfo->GetBaseProduction();
					if (!producedPower)
					{
						//producedPower = generator->CalcPowerProductionCapacityForPotential(generator->GetPendingPotential());
						producedPower = powerInfo->GetDynamicProductionCapacity();
					}
				}

				auto includeDetails = producedPower && buildDescriptor && includePowerDetails;

				if (includeDetails)
				{
					switch (filterType)
					{
					case PowerCheckerFilterType::PausedOnly:
						includeDetails &= generator && generator->IsProductionPaused();
						break;

					case PowerCheckerFilterType::OutOfFuelOnly:
						includeDetails &= generator && !generator->CanStartPowerProduction() && !powerInfo->GetBaseProduction();
						break;

					default:
						includeDetails &= (includePaused || generator && !generator->IsProductionPaused()) &&
							(includeOutOfFuel || generator && generator->CanStartPowerProduction() || powerInfo->GetBaseProduction());
						break;
					}
				}

				if (includeDetails)
				{
					auto& detail = generatorDetails[buildDescriptor]
						[producedPower]
						[(generator ? generator->GetPendingPotential() : 1) * 100];

					detail.amount++;

					if (generator)
					{
						detail.factories.Add(generator);
					}
				}
			}
			else if (powerStorage)
			{
				if (buildDescriptor && includePowerDetails)
				{
					auto& detail = powerStorageDetails[buildDescriptor]
						[powerStorage->GetPowerStore()]
						[100];

					detail.amount++;
					if (powerStorage)
					{
						detail.factories.Add(powerStorage);
					}
				}
			}
			else if (factory || powerInfo->GetTargetConsumption() || powerInfo->GetMaximumTargetConsumption())
			{
				if (!factory || factory->IsConfigured() && factory->GetProducingPowerConsumption())
				{
					if (IS_PC_LOG_LEVEL(ELogVerbosity::Log))
					{
						PC_LOG_Display(TEXT("    Power Comsumption: "), factory ? factory->GetProducingPowerConsumption() : 0);
						PC_LOG_Display(TEXT("    Default Power Comsumption: "), factory ? factory->GetDefaultProducingPowerConsumption() : 0);
						PC_LOG_Display(TEXT("    Pending Potential: "), factory ? factory->GetPendingPotential() : 0);
						PC_LOG_Display(
							TEXT("    Producing Power Consumption For Potential: "),
							factory ? factory->CalcProducingPowerConsumptionForPotential(factory->GetPendingPotential()) : 0
							);
						PC_LOG_Display(TEXT("    Maximum Target Consumption: "), powerInfo->GetMaximumTargetConsumption());
						PC_LOG_Display(TEXT("    Is Production Paused: "), (factory ? factory->IsProductionPaused() : false) ? TEXT("true") : TEXT("false"));
					}

					auto maxComsumption = FMath::Max(
						factory ? factory->GetProducingPowerConsumption() : 0,
						FMath::Max(
							powerInfo->GetTargetConsumption(),
							powerInfo->GetMaximumTargetConsumption()
							)
						);

					if (includePaused || !factory || !factory->IsProductionPaused())
					{
						totalMaximumPotential += maxComsumption;
					}

					auto includeDetails = (includePaused || !factory || factory && !factory->IsProductionPaused()) && buildDescriptor && includePowerDetails;

					if (includeDetails)
					{
						switch (filterType)
						{
						case PowerCheckerFilterType::PausedOnly:
							includeDetails &= factory && factory->IsProductionPaused();
							break;

						case PowerCheckerFilterType::OutOfFuelOnly:
							includeDetails = false;
							break;

						default:
							break;
						}
					}

					if (includeDetails)
					{
						auto& detail = consumerDetails[buildDescriptor]
							[maxComsumption]
							[(factory ? factory->GetPendingPotential() : 1) * 100];

						detail.amount++;
						if (factory)
						{
							detail.factories.Add(factory);
						}
					}
				}
			}
			else
			{
				PC_LOG_Display_Condition(ELogVerbosity::Log, TEXT("PowerChecker: Unknown "), *className);

				// dumpUnknownClass(nextActor);
			}
		}
	}

	if (includePowerDetails)
	{
		auto owner = powerConnection->GetOwner();

		auto powerDetailsSorter = [owner]
		(const std::map<TSubclassOf<UFGItemDescriptor>, std::map<float, std::map<float, FPowerDetail>>>& powerDetails, TArray<FPowerDetail>& sortedPowerDetails)
		{
			for (auto itBuilding = powerDetails.begin(); itBuilding != powerDetails.end(); itBuilding++)
			{
				for (auto itPower = itBuilding->second.begin(); itPower != itBuilding->second.end(); itPower++)
				{
					for (auto itClock = itPower->second.begin(); itClock != itPower->second.end(); itClock++)
					{
						FPowerDetail powerDetail = itClock->second;
						powerDetail.buildingType = itBuilding->first;
						powerDetail.powerPerBuilding = itPower->first;
						powerDetail.potential = itClock->first;

						sortedPowerDetails.Add(powerDetail);

						powerDetail.factories.Sort(
							[owner](const AFGBuildableFactory& x, const AFGBuildableFactory& y)
							{
								float order = owner->GetDistanceTo(&x) - owner->GetDistanceTo(&y);

								if (order == 0)
								{
									order = x.GetName().Compare(y.GetName());
								}

								return order < 0;
							}
							);
					}
				}
			}

			sortedPowerDetails.Sort(
				[](const FPowerDetail& x, const FPowerDetail& y)
				{
					float order = (x.powerPerBuilding > 0 ? 0 : 1) - (y.powerPerBuilding > 0 ? 0 : 1);

					if (order == 0)
					{
						order = x.powerPerBuilding - y.powerPerBuilding;
					}

					if (order == 0)
					{
						order = UFGItemDescriptor::GetItemName(x.buildingType).CompareTo(UFGItemDescriptor::GetItemName(y.buildingType));
					}

					if (order == 0)
					{
						order = x.potential - y.potential;
					}

					return order < 0;
				}
				);
		};

		powerDetailsSorter(generatorDetails, outGeneratorDetails);
		powerDetailsSorter(powerStorageDetails, outPowerStorageDetails);
		powerDetailsSorter(consumerDetails, outConsumerDetails);
	}
}

int APowerCheckerLogic::GetLogLevel()
{
	return configuration.logLevel;
}

float APowerCheckerLogic::GetMaximumPlayerDistance()
{
	return configuration.maximumPlayerDistance;
}

float APowerCheckerLogic::GetSpareLimit()
{
	return configuration.spareLimit;
}

bool APowerCheckerLogic::inheritsFromClass(AActor* owner, const FString& className)
{
	for (auto cls = owner->GetClass()->GetSuperClass(); cls && cls != AActor::StaticClass(); cls = cls->GetSuperClass())
	{
		if (cls->GetPathName() == className)
		{
			return true;
		}
	}

	return false;
}

void APowerCheckerLogic::dumpUnknownClass(UObject* obj)
{
	if (IS_PC_LOG_LEVEL(ELogVerbosity::Log))
	{
		PC_LOG_Display(TEXT("Unknown Class "), *obj->GetClass()->GetPathName());

		for (auto cls = obj->GetClass()->GetSuperClass(); cls && cls != AActor::StaticClass(); cls = cls->GetSuperClass())
		{
			PC_LOG_Display(TEXT("    - Super: "), *cls->GetPathName());
		}

		for (TFieldIterator<FProperty> property(obj->GetClass()); property; ++property)
		{
			PC_LOG_Display(
				TEXT("    - "),
				*property->GetName(),
				TEXT(" ("),
				*property->GetCPPType(),
				TEXT(" / "),
				*property->GetClass()->GetName(),
				TEXT(")")
				);

			auto floatProperty = CastField<FFloatProperty>(*property);
			if (floatProperty)
			{
				PC_LOG_Display(TEXT("        = "), floatProperty->GetPropertyValue_InContainer(obj));
			}

			auto intProperty = CastField<FIntProperty>(*property);
			if (intProperty)
			{
				PC_LOG_Display(TEXT("        = "), intProperty->GetPropertyValue_InContainer(obj));
			}

			auto boolProperty = CastField<FBoolProperty>(*property);
			if (boolProperty)
			{
				PC_LOG_Display(TEXT("        = "), boolProperty->GetPropertyValue_InContainer(obj) ? TEXT("true") : TEXT("false"));
			}

			auto structProperty = CastField<FStructProperty>(*property);
			if (structProperty && property->GetName() == TEXT("mFactoryTickFunction"))
			{
				auto factoryTick = structProperty->ContainerPtrToValuePtr<FFactoryTickFunction>(obj);
				if (factoryTick)
				{
					PC_LOG_Display(TEXT("    - Tick Interval = "), factoryTick->TickInterval);
				}
			}

			auto strProperty = CastField<FStrProperty>(*property);
			if (strProperty)
			{
				PC_LOG_Display(TEXT("        = "), *strProperty->GetPropertyValue_InContainer(obj));
			}
		}
	}
}

bool APowerCheckerLogic::IsValidBuildable(AFGBuildable* newBuildable)
{
	if (!newBuildable)
	{
		return false;
	}

	/*if (newBuildable->GetClass()->GetPathName().EndsWith(TEXT("/StorageTeleporter/Buildables/ItemTeleporter/ItemTeleporter_Build.ItemTeleporter_Build_C")))
	{
		addTeleporter(newBuildable);
	}
	else*/
	if (auto powerChecker = Cast<APowerCheckerBuilding>(newBuildable))
	{
		addPowerChecker(powerChecker);
	}
	else
	{
		return false;
	}

	return true;
}

// void APowerCheckerLogic::addTeleporter(AFGBuildable* teleporter)
// {
// 	FScopeLock ScopeLock(&eclCritical);
// 	allTeleporters.Add(teleporter);
//
// 	teleporter->OnEndPlay.Add(removeTeleporterDelegate);
// }
//
// void APowerCheckerLogic::removeTeleporter(AActor* actor, EEndPlayReason::Type reason)
// {
// 	FScopeLock ScopeLock(&eclCritical);
// 	allTeleporters.Remove(Cast<AFGBuildable>(actor));
//
// 	actor->OnEndPlay.Remove(removeTeleporterDelegate);
// }

void APowerCheckerLogic::addPowerChecker(APowerCheckerBuilding* powerChecker)
{
	FScopeLock ScopeLock(&eclCritical);
	allPowerCheckers.Add(powerChecker);

	powerChecker->OnEndPlay.Add(removePowerCheckerDelegate);
}

void APowerCheckerLogic::removePowerChecker(AActor* actor, EEndPlayReason::Type reason)
{
	FScopeLock ScopeLock(&eclCritical);
	allPowerCheckers.Remove(Cast<APowerCheckerBuilding>(actor));

	actor->OnEndPlay.Remove(removePowerCheckerDelegate);
}


void APowerCheckerLogic::setConfiguration(const FPowerChecker_ConfigStruct& in_configuration)
{
	configuration = in_configuration;

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	PC_LOG_Display(TEXT("StartupModule"));

	PC_LOG_Display(TEXT("logLevel = "), configuration.logLevel);
	PC_LOG_Display(TEXT("maximumPlayerDistance = "), configuration.maximumPlayerDistance);
	PC_LOG_Display(TEXT("spareLimit = "), configuration.spareLimit);
	PC_LOG_Display(TEXT("overflowBlinkCycle = "), configuration.overflowBlinkCycle);

	PC_LOG_Display(TEXT("==="));
}

#ifndef OPTIMIZE
#pragma optimize( "", on )
#endif
