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
	* physics service server (given its IpAddr), intialize the physics world
	* with all the PSD actors to simulate and set the flag "bIsSimulating" to
	* start simulating on every tick.
	* 
	* @param SockerServerIpAddr The physics service ip address to connect and
	* request physics updates
	*/
	UFUNCTION(BlueprintCallable)
	void StartPSDActorsSimulation(const FString& SocketServerIpAddr);

	/** 
	* Stops the PSD actors simulation. Will set the flag and close the socket 
	* connection. 
	*/
	UFUNCTION(BlueprintCallable)
	void StopPSDActorsSimulation();

	/**
	* Spawns a new PSD Sphere. This will request the PSDActorsSpawner to 
	* create a new PSDSphere as also requesting the physics service to add
	* the new sphere body on the physics system through the socket proxy.
	* 
	* @param NewSphereLocation The sphere initial location to spawn
	*/
	UFUNCTION(BlueprintCallable)
	void SpawnNewPSDSphere(const FVector NewSphereLocation);

	/** 
	* Getter to the flag "bIsSimulating" to indicate if the PSD actors are
	* currently simulating physics.
	* 
	* @return True if PSD actors are simulating physics. False otherwise
	*/
	UFUNCTION(BlueprintCallable)
	bool IsSimulating() { return bIsSimulating; }

public:
	/** 
	* Starts a PSD actor simulation during 30 seconds. This should be used for
	* testing-purposes only. 
	* 
	* @param SocketServerIpAddr The physics service ip address to connect and
	* request physics updates
	* @param TestDurationInSeconds The test duration in seconds
	* 
	* @note This should not be used for game-purposes, as the physics should
	* always be simulating once the BeginPlay() is called. This is useful for
	* BouncingSpheres test map
	*/
	UFUNCTION(BlueprintCallable)
	void StartPSDActorsSimulationTest(const FString& SocketServerIpAddr,
		float TestDurationInSeconds = 30.f);

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
	* The list of PSD actors to simulate. The key is a unique Id to identify
	* it on the physics service and the value is the PSD actor reference 
	* itself.
	*/
	TMap<uint32, class APSDActorBase*> PSDActorMap;

	/** The PSDActors Spawner reference to request PSD Actors spawn */
	class APSDActorsSpawner* PSDActorsSpanwer = nullptr;

	/** 
	* Flag that indicates if this PSD actor coordinator is currently updating 
	* the PSD actors
	*/
	bool bIsSimulating = false;

private:
	/**
	* The TimerHandle that handles the PSD actors test (test-purposes only).
	*/
	FTimerHandle PSDActorsTestTimerHandle;
};
