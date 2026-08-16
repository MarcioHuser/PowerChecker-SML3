#include "PowerCheckerRCO.h"
#include "PowerCheckerBuilding.h"
#include "Util/PCOptimize.h"

#include "FGPlayerController.h"
#include "Net/UnrealNetwork.h"

#ifndef OPTIMIZE
#pragma optimize("", off )
#endif

void UPowerCheckerRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPowerCheckerRCO, dummy);
}

UPowerCheckerRCO* UPowerCheckerRCO::getRCO(UWorld* world)
{
	return Cast<UPowerCheckerRCO>(
		Cast<AFGPlayerController>(world->GetFirstPlayerController())->GetRemoteCallObjectOfClass(UPowerCheckerRCO::StaticClass())
		);
}

void UPowerCheckerRCO::SetIncludePaused_Implementation(APowerCheckerBuilding* powerChecker, bool includePaused)
{
	if (IsValid(powerChecker) && powerChecker->HasAuthority())
	{
		powerChecker->Server_SetIncludePaused(includePaused);
	}
}

bool UPowerCheckerRCO::SetIncludePaused_Validate(APowerCheckerBuilding* powerChecker, bool includePaused)
{
	return IsValid(powerChecker);
}

void UPowerCheckerRCO::SetIncludeOutOfFuel_Implementation(APowerCheckerBuilding* powerChecker, bool includeOutOfFuel)
{
	if (IsValid(powerChecker) && powerChecker->HasAuthority())
	{
		powerChecker->Server_SetIncludeOutOfFuel(includeOutOfFuel);
	}
}

bool UPowerCheckerRCO::SetIncludeOutOfFuel_Validate(APowerCheckerBuilding* powerChecker, bool includeOutOfFuel)
{
	return IsValid(powerChecker);
}

void UPowerCheckerRCO::TriggerUpdateValues_Implementation(APowerCheckerBuilding* powerChecker, bool updateMaximumPotential, bool withDetails, PowerCheckerFilterType filterType)
{
	if (IsValid(powerChecker) && powerChecker->HasAuthority())
	{
		powerChecker->Server_TriggerUpdateValues(updateMaximumPotential, withDetails, filterType);
	}
}

bool UPowerCheckerRCO::TriggerUpdateValues_Validate(APowerCheckerBuilding* powerChecker, bool updateMaximumPotential, bool withDetails, PowerCheckerFilterType filterType)
{
	return IsValid(powerChecker);
}

void UPowerCheckerRCO::SetProductionPaused_Implementation(class AFGBuildableFactory* factory, bool isProductionPaused)
{
	if (IsValid(factory) && factory->HasAuthority())
	{
		factory->SetIsProductionPaused(isProductionPaused);
	}
}

bool UPowerCheckerRCO::SetProductionPaused_Validate(class AFGBuildableFactory* factory, bool productionIsPaused)
{
	return IsValid(factory);
}

void UPowerCheckerRCO::SetPendingPotential_Implementation(class AFGBuildableFactory* factory, float pendingPotential)
{
	if (IsValid(factory) && factory->HasAuthority())
	{
		pendingPotential = FMath::Clamp(pendingPotential, factory->GetCurrentMinPotential(), factory->GetCurrentMaxPotential());
		
		factory->SetPendingPotential(pendingPotential);
	}
}

bool UPowerCheckerRCO::SetPendingPotential_Validate(class AFGBuildableFactory* factory, float pendingPotential)
{
	return IsValid(factory);
}

#ifndef OPTIMIZE
#pragma optimize("", on )
#endif
