#include "PowerCheckerBuilding.h"
#include "PowerCheckerModule.h"
#include "PowerCheckerRCO.h"
#include "Util/Logging.h"

#include "FGCharacterPlayer.h"
#include "FGPowerCircuit.h"
#include "FGPowerConnectionComponent.h"

#include "GeneratedCodeHelpers.h"

#ifndef OPTIMIZE
#pragma optimize( "", off )
#endif

APowerCheckerBuilding::APowerCheckerBuilding()
{
}

void APowerCheckerBuilding::BeginPlay()
{
	Super::BeginPlay();
}

void APowerCheckerBuilding::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		auto testCircuitId = getCircuitId();

		if (!ValidateTick(testCircuitId))
		{
			return;
		}

		lastCheck = GetWorld()->GetTimeSeconds();

		auto updateMaximumPotential = false;

		if (testCircuitId != currentCircuitId)
		{
			if (APowerCheckerLogic::configuration.logInfoEnabled)
			{
				PC_LOG_Display(
					*getTagName(),
					TEXT("Recalculating because Circuit ID changed from "),
					currentCircuitId,
					TEXT(" to "),
					testCircuitId
					);
			}

			currentCircuitId = testCircuitId;
			updateMaximumPotential = true;
		}

		Server_TriggerUpdateValues(updateMaximumPotential, false);
	}
}

void APowerCheckerBuilding::SetIncludePaused(bool in_includePaused)
{
	if (HasAuthority())
	{
		Server_SetIncludePaused(in_includePaused);
	}
	else
	{
		auto rco = UPowerCheckerRCO::getRCO(GetWorld());
		if (rco)
		{
			if (APowerCheckerLogic::configuration.logInfoEnabled)
			{
				PC_LOG_Display(*getTagName(), TEXT("Calling SetCustomInjectedInput at server"));
			}

			rco->SetIncludePaused(this, in_includePaused);
		}
	}
}

void APowerCheckerBuilding::Server_SetIncludePaused(bool in_includePaused)
{
	this->includePaused = in_includePaused;
}

void APowerCheckerBuilding::SetIncludeOutOfFuel(bool in_includeOutOfFuel)
{
	if (HasAuthority())
	{
		Server_SetIncludeOutOfFuel(in_includeOutOfFuel);
	}
	else
	{
		auto rco = UPowerCheckerRCO::getRCO(GetWorld());
		if (rco)
		{
			if (APowerCheckerLogic::configuration.logInfoEnabled)
			{
				PC_LOG_Display(*getTagName(), TEXT("Calling SetCustomInjectedInput at server"));
			}

			rco->SetIncludeOutOfFuel(this, in_includeOutOfFuel);
		}
	}
}

void APowerCheckerBuilding::Server_SetIncludeOutOfFuel(bool in_includeOutOfFuel)
{
	this->includeOutOfFuel = in_includeOutOfFuel;
}

void APowerCheckerBuilding::TriggerUpdateValues(bool updateMaximumPotential, bool withDetails, PowerCheckerFilterType filterType)
{
	if (HasAuthority())
	{
		Server_TriggerUpdateValues(updateMaximumPotential, withDetails, filterType);
	}
	else
	{
		auto rco = UPowerCheckerRCO::getRCO(GetWorld());
		if (rco)
		{
			if (APowerCheckerLogic::configuration.logInfoEnabled)
			{
				PC_LOG_Display(*getTagName(), TEXT("Calling SetCustomInjectedInput at server"));
			}

			rco->TriggerUpdateValues(this, updateMaximumPotential, withDetails, filterType);
		}
	}
}

void APowerCheckerBuilding::Server_TriggerUpdateValues(bool updateMaximumPotential, bool withDetails, PowerCheckerFilterType filterType)
{
	auto powerConnection = FindComponentByClass<UFGPowerConnectionComponent>();

	auto powerCircuit = powerConnection ? powerConnection->GetPowerCircuit() : nullptr;

	FPowerCircuitStats circuitStats;
	if (powerCircuit)
	{
		powerCircuit->GetStats(circuitStats);
	}

	TArray<FPowerDetail> generatorDetails;
	TArray<FPowerDetail> powerStorageDetails;
	TArray<FPowerDetail> consumerDetails;

	if (updateMaximumPotential)
	{
		APowerCheckerLogic::GetMaximumPotentialWithDetails(
			powerConnection,
			calculatedMaximumPotential,
			includePaused,
			includeOutOfFuel,
			withDetails,
			generatorDetails,
			powerStorageDetails,
			consumerDetails,
			filterType
			);
	}
	else if (filterType != PowerCheckerFilterType::Any)
	{
		float _dummyValue;
		APowerCheckerLogic::GetMaximumPotentialWithDetails(
			powerConnection,
			_dummyValue,
			includePaused,
			includeOutOfFuel,
			withDetails,
			generatorDetails,
			powerStorageDetails,
			consumerDetails,
			filterType
			);
	}

	isOverflow = circuitStats.PowerProductionCapacity > 0 && circuitStats.PowerProductionCapacity < calculatedMaximumPotential;

	if (circuitStats.PowerProductionCapacity == 0 || circuitStats.PowerProductionCapacity < calculatedMaximumPotential)
	{
		productionStatus = EProductionStatus::IS_ERROR;
	}
	else if (circuitStats.PowerProductionCapacity - APowerCheckerLogic::configuration.spareLimit < calculatedMaximumPotential)
	{
		productionStatus = EProductionStatus::IS_STANDBY;
	}
	else
	{
		productionStatus = EProductionStatus::IS_PRODUCING;
	}

	// PC_LOG_Display(getTagName(), TEXT("Built-in maximum consumption: "), circuitStats.MaximumPowerConsumption)
	// PC_LOG_Display(getTagName(), TEXT("Calculated maximum: "), calculatedMaximumPotential)

	if (withDetails)
	{
		UpdateValuesWithDetails(generatorDetails, powerStorageDetails, consumerDetails);
	}
	else
	{
		UpdateValues();
	}
}

void APowerCheckerBuilding::UpdateValues_Implementation()
{
	// if (APowerCheckerLogic::configuration.logInfoEnabled)
	// {
	// 	PC_LOG_Display(
	// 		*getTagName(),
	// 		TEXT("UpdateValues_Implementation: authority = "),
	// 		HasAuthority() ? TEXT("true") : TEXT("false"),
	// 		TEXT(" / maximumProduction = "),
	// 		maximumProduction,
	// 		TEXT(" / currentConsumption = "),
	// 		currentConsumption,
	// 		TEXT(" / maximumConsumption = "),
	// 		maximumConsumption
	// 		);
	// }

	OnUpdateValues.Broadcast();
}

void APowerCheckerBuilding::UpdateValuesWithDetails_Implementation
(
	const TArray<FPowerDetail>& generatorDetails,
	const TArray<FPowerDetail>& powerStorageDetails,
	const TArray<FPowerDetail>& consumerDetails
)
{
	// if (APowerCheckerLogic::configuration.logInfoEnabled)
	// {
	// 	PC_LOG_Display(
	// 		*getTagName(),
	// 		TEXT("UpdateValuesWithDetails_Implementation: authority = "),
	// 		HasAuthority() ? TEXT("true") : TEXT("false"),
	// 		TEXT(" / maximumProduction = "),
	// 		maximumProduction,
	// 		TEXT(" / currentConsumption = "),
	// 		currentConsumption,
	// 		TEXT(" / maximumConsumption = "),
	// 		maximumConsumption,
	// 		TEXT(" / powerDetails = "),
	// 		powerDetails.Num(),
	// 		TEXT(" items")
	// 		);
	// }

	OnUpdateValuesWithDetails.Broadcast(generatorDetails, powerStorageDetails, consumerDetails);
}

EProductionStatus APowerCheckerBuilding::GetProductionIndicatorStatus() const
{
	// // auto baseStatus = Super::GetProductionIndicatorStatus();
	//
	// auto powerInfo = GetPowerInfo();
	//
	// if (!powerInfo)
	// {
	//     return EProductionStatus::IS_NONE;
	// }
	// else if (powerInfo->IsFuseTriggered())
	// {
	//     return EProductionStatus::IS_ERROR;
	// }

	return productionStatus == EProductionStatus::IS_ERROR &&
	       isOverflow &&
	       (int)(GetWorld()->GetTimeSeconds() / (APowerCheckerLogic::configuration.overflowBlinkCycle / 2)) % 2 == 0
		       ? EProductionStatus::IS_NONE
		       : productionStatus;
}

bool APowerCheckerBuilding::ValidateTick(int testCircuitId)
{
	return (lastCheck + 1 <= GetWorld()->GetTimeSeconds() || testCircuitId != currentCircuitId) && checkPlayerIsNear();
}

bool APowerCheckerBuilding::checkPlayerIsNear()
{
	auto playerIt = GetWorld()->GetPlayerControllerIterator();
	for (; playerIt; ++playerIt)
	{
		const auto player = Cast<AFGCharacterPlayer>((*playerIt)->GetPawn());
		if (!player)
		{
			continue;
		}

		auto playerTranslation = player->GetActorLocation();

		auto playerState = player->GetPlayerState();

		auto distance = FVector::Dist(playerTranslation, GetActorLocation());

		if (distance <= APowerCheckerLogic::configuration.maximumPlayerDistance)
		{
			// if (APowerCheckerLogic::configuration.logInfoEnabled)
			// {
			// 	PC_LOG_Display(
			// 		*getTagName(),
			// 		TEXT("Player "),
			// 		*playerState->GetPlayerName(),
			// 		TEXT(" location = "),
			// 		*playerTranslation.ToString(),
			// 		TEXT(" / build location = "),
			// 		*GetActorLocation().ToString(),
			// 		TEXT(" / distance = "),
			// 		distance
			// 		);
			// }

			return true;
		}
	}

	return false;
}

void APowerCheckerBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APowerCheckerBuilding, productionStatus);
	DOREPLIFETIME(APowerCheckerBuilding, isOverflow);
	DOREPLIFETIME(APowerCheckerBuilding, currentCircuitId);
	DOREPLIFETIME(APowerCheckerBuilding, calculatedMaximumPotential);
	DOREPLIFETIME(APowerCheckerBuilding, includePaused);
	DOREPLIFETIME(APowerCheckerBuilding, includeOutOfFuel);
}

int APowerCheckerBuilding::getCircuitId()
{
	auto powerConnection = FindComponentByClass<UFGPowerConnectionComponent>();
	if (!powerConnection)
	{
		return 0;
	}

	auto powerCircuit = powerConnection->GetPowerCircuit();
	if (!powerCircuit)
	{
		return 0;
	}

	return powerCircuit->GetCircuitID();
}

int APowerCheckerBuilding::getCircuitGroupId()
{
	auto powerConnection = FindComponentByClass<UFGPowerConnectionComponent>();
	if (!powerConnection)
	{
		return -1;
	}

	auto powerCircuit = powerConnection->GetPowerCircuit();
	if (!powerCircuit)
	{
		return -1;
	}

	return powerCircuit->GetCircuitGroupID();
}

#ifndef OPTIMIZE
#pragma optimize( "", on )
#endif
