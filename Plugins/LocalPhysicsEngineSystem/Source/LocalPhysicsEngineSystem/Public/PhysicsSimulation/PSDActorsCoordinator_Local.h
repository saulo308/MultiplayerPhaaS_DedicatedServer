// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PSDActorsCoordinator_Local.generated.h"

UCLASS()
class LOCALPHYSICSENGINESYSTEM_API APSDActorsCoordinator_Local : public AActor
{
	GENERATED_BODY()

public:
	/**
	* Getter to the flag "bIsSimulating" to indicate if the PSD actors are
	* currently simulating physics.
	*
	* @return True if PSD actors are simulating physics. False otherwise
	*/
	UFUNCTION(BlueprintCallable)
	bool IsSimulating() const { return bIsSimulatingPhysics; }

	/**
	* Starts the PSD actors simulation. First will open a connection with the
	* physics service servers (given their IpAddr), intialize the physics world
	* with all the PSD actors to simulate and set the flag "bIsSimulating" to
	* start simulating on every tick.
	*
	* @param SocketServerIpAddrList The physics service ip addresses to connect
	* and request physics updates
	*
	* @note That we receive a array of server ip as we may have multiple
	* physics services to distribute the workload
	*/
	UFUNCTION(BlueprintCallable)
	void StartPSDActorsSimulation(const TArray<FString>&
		SocketServerIpAddrList);

	/**
	* Starts a PSD actor simulation during 30 seconds. This should be used for
	* testing-purposes only.
	*
	* @param SocketServerIpAddrList The physics service ip addresses to connect
	* and request physics updates
	* @param TestDurationInSeconds The test duration in seconds
	*
	* @note This should not be used for game-purposes, as the physics should
	* always be simulating once the BeginPlay() is called. This is useful for
	* BouncingSpheres test map
	*/
	UFUNCTION(BlueprintCallable)
	void StartPSDActorsSimulationTest(const TArray<FString>&
		SocketServerIpAddrList, float TestDurationInSeconds = 30.f);

	/**
	* Stops the PSD actors simulation. Will set the flag and close the socket
	* connection.
	*/
	UFUNCTION(BlueprintCallable)
	void StopPSDActorsSimulation();

public:
	/** Sets default values for this actor's properties */
	APSDActorsCoordinator_Local();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/**
	* Called when the play is over. Will stop PSD actor simulation if it is
	* active. This should avoid leaving a socket connection open.
	*/
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** */
	virtual void GetLifetimeReplicatedProps
		(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

private:
	UFUNCTION(NetMulticast, Reliable)
	void SaveDeltaTimeMeasurementToFile() const;

	UFUNCTION(NetMulticast, Reliable)
	void SaveStepPhysicsTimeMeasureToFile() const;

	UFUNCTION(NetMulticast, Reliable)
	void GetRamMeasurement();

	/** */
	UFUNCTION(NetMulticast, Reliable)
	void SaveUsedRamMeasurements() const;

	/** */
	UFUNCTION(NetMulticast, Reliable)
	void SaveAllocatedRamMeasurements() const;

private:
	/**
	* Updates the PSD actors Transform. This will request the physics service
	* server physics update and await its response. Once returned, will parse
	* the result to update each PSD actor Transform.
	*/
	void UpdatePSDActors();

	void InitializePhysicsWorld();

private:
	/**
	* Flag that indicates if this PSD actor coordinator is currently updating
	* the PSD actors
	*/
	UPROPERTY(Replicated)
	bool bIsSimulatingPhysics = false;

private:
	/**
	* The TimerHandle that handles the PSD actors test (test-purposes only).
	*/
	FTimerHandle PSDActorsTestTimerHandle;

	/** Counts the amount of step the physics system */
	uint32 StepPhysicsCounter = 0;

	/** */
	TMap<uint32, class APSDActorBase*> PSDActorMap;

private:
	/** */
	FString DeltaTimeMeasurement = FString();

	/** */
	FString StepPhysicsTimeTimeMeasure = FString();

	/** */
	FString UsedRamMeasurement = FString();

	/** */
	FString AllocatedRamMeasurement = FString();

	/** */
	FString CPUUsageMeasurement = FString();

	/** */
	bool bHasMeasuredCpuAndRamForSimulation = false;

	/** */
	class FPhysicsServiceImpl* PhysicsServiceLocalImpl = nullptr;
};
