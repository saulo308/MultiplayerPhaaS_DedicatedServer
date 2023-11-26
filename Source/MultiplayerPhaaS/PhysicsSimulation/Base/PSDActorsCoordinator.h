// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PSDActorsCoordinator.generated.h"

/** 
* This actor handles the coordination of the Physics-Service-Drive (PSD) 
* actors. Thus, this will communicates with the physics service through a 
* socket to initialize and request physics updates. Once the response reaches
* the game, the coordinator will update every PSD actor Transform (location
* and rotation).
* 
* @see PSDActorBase
*/
UCLASS()
class MULTIPLAYERPHAAS_API APSDActorsCoordinator : public AActor
{
	GENERATED_BODY()

public:
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
	* Stops the PSD actors simulation. Will set the flag and close the socket 
	* connection. 
	*/
	UFUNCTION(BlueprintCallable)
	void StopPSDActorsSimulation();

	/** 
	* Getter to the flag "bIsSimulating" to indicate if the PSD actors are
	* currently simulating physics.
	* 
	* @return True if PSD actors are simulating physics. False otherwise
	*/
	UFUNCTION(BlueprintCallable)
	bool IsSimulating() { return bIsSimulatingPhysics; }

public:
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

public:
	/** Sets default values for this actor's properties */
	APSDActorsCoordinator();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** 
	* Called when the play is over. Will stop PSD actor simulation if it is
	* active. This should avoid leaving a socket connection open.
	*/
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

private:
	/** 
	* Initializes the physics world on the physics service. This will send
	* a message containing all the PSD actors unique ID and Transform. This 
	* will be the physics world's first state to begin simulating.
	*/
	void InitializePhysicsWorld();

	/** 
	* Updates the PSD actors Transform. This will request the physics service
	* server physics update and await its response. Once returned, will parse 
	* the result to update each PSD actor Transform.
	*/
	void UpdatePSDActors();

private:
	/** 
	* Connects to a list of physics services. The server ip addresses are 
	* given by the parameter. 
	* 
	* @param SocketServerIpAddrList The physics service ip addresses to connect 
	* to
	* 
	* @return True if could connect to all server ip addresses. False otherwise
	*/
	bool ConnectToPhysicsServices
		(const TArray<FString>& SocketServerIpAddrList);

	/** 
	* Gets all PSDActors that will be simulated. This will be given by the
	* list of physics service regions
	*/
	void GetAllPSDActorsToSimulate();

	/** Gets all the physics services regions on the world. */
	void GetAllPhysicsServiceRegions();

	/** 
	* Gets the PSDActor spawner on the world. This is used to spawn PSDActors.
	* 
	* @note TODO This is only used to spawn PSD speheres. However, this should 
	* be refactored to reside inside the "APhysicsServiceRegion".
	* 
	* @see SpawnNewPSDSphere
	*/
	void GetPSDActorsSpanwer();

private:
	/** 
	* The list of PSD actors to simulate. The key is a unique Id to identify
	* it on the physics service and the value is the PSD actor reference 
	* itself.
	*/
	TMap<uint32, class APSDActorBase*> PSDActorsToSimulateMap;

	/** The list of physics service regions on the world. */
	TArray<class APhysicsServiceRegion*> PhysicsServiceRegionList;

	/** The PSDActors Spawner reference to request PSD Actors spawn */
	class APSDActorsSpawner* PSDActorsSpanwer = nullptr;

	/** 
	* Flag that indicates if this PSD actor coordinator is currently updating 
	* the PSD actors
	*/
	bool bIsSimulatingPhysics = false;

private:
	/**
	* The TimerHandle that handles the PSD actors test (test-purposes only).
	*/
	FTimerHandle PSDActorsTestTimerHandle;
};
