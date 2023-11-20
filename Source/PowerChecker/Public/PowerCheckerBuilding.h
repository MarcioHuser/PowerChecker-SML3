#pragma once

#include "UObject/CoreNet.h"

#include "FGPowerCircuit.h"
#include "Buildables/FGBuildableFactory.h"
#include "Logic/PowerCheckerLogic.h"
#include "PowerCheckerBuilding.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUpdateValues,
	const FSimplePowerCircuitStats&,
	powerCircuitStats,
	float,
	sumPowerStore
	);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FUpdateValuesWithDetails,
	const FSimplePowerCircuitStats&,
	powerCircuitStats,
	float,
	sumPowerStore,
	const TArray<FPowerDetail>&,
	generatorDetails,
	const TArray<FPowerDetail>&,
	powerStorageDetails,
	const TArray<FPowerDetail>&,
	consumerDetails
	);

UCLASS(Blueprintable)
class POWERCHECKER_API APowerCheckerBuilding : public AFGBuildableFactory
{
	GENERATED_BODY()

	FString _TAG_NAME = TEXT("PowerCheckerBuilding: ");

	inline FString
	getTagName() const
	{
		return TEXT(" ") + _TAG_NAME;
	}

public:
	APowerCheckerBuilding();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="PowerChecker", DisplayName="TriggerUpdateValues")
	virtual void TriggerUpdateValues(bool updateMaximumPotential = false, bool withDetails = false, PowerCheckerFilterType filterType = PowerCheckerFilterType::Any);
	virtual void Server_TriggerUpdateValues(bool updateMaximumPotential = false, bool withDetails = false, PowerCheckerFilterType filterType = PowerCheckerFilterType::Any);

	UFUNCTION(BlueprintCallable, Category="PowerChecker", DisplayName="SetIncludePaused")
	virtual void SetIncludePaused(bool in_includePaused = true);
	virtual void Server_SetIncludePaused(bool in_includePaused = true);

	UFUNCTION(BlueprintCallable, Category="PowerChecker", DisplayName="SetIncludeOutOfFuel")
	virtual void SetIncludeOutOfFuel(bool in_includeOutOfFuel = true);
	virtual void Server_SetIncludeOutOfFuel(bool in_includeOutOfFuel = true);

	UPROPERTY(BlueprintAssignable, Category = "PowerChecker")
	FUpdateValues OnUpdateValues;

	UFUNCTION(BlueprintCallable, Category = "PowerChecker", NetMulticast, Reliable)
	virtual void UpdateValues(const FSimplePowerCircuitStats& circuitStats, float batterySumPowerStore);

	UPROPERTY(BlueprintAssignable, Category = "PowerChecker")
	FUpdateValuesWithDetails OnUpdateValuesWithDetails;

	UFUNCTION(BlueprintCallable, Category = "PowerChecker", NetMulticast, Reliable)
	virtual void UpdateValuesWithDetails
	(
		const FSimplePowerCircuitStats& circuitStats,
		float batterySumPowerStore,
		const TArray<FPowerDetail>& generatorDetails,
		const TArray<FPowerDetail>& powerStorageDetails,
		const TArray<FPowerDetail>& consumerDetails
	);

	virtual EProductionStatus GetProductionIndicatorStatus() const override;

	virtual bool ValidateTick(int testCircuitId);
	virtual bool checkPlayerIsNear();
	virtual int getCircuitId();
	virtual int getCircuitGroupId();

	UPROPERTY(BlueprintReadWrite, Replicated)
	EProductionStatus productionStatus = EProductionStatus::IS_NONE;

	UPROPERTY(BlueprintReadWrite, Replicated)
	bool isOverflow = false;

	UPROPERTY(BlueprintReadWrite, Replicated)
	int currentCircuitId = 0;

	UPROPERTY(BlueprintReadWrite, Replicated)
	float calculatedMaximumPotential = 0;

	UPROPERTY(BlueprintReadWrite, Replicated, SaveGame)
	bool includePaused = true;

	UPROPERTY(BlueprintReadWrite, Replicated, SaveGame)
	bool includeOutOfFuel = true;

	float lastCheck = 0;
};
