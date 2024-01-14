// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExternalCommunication/Sockets/SocketClientThreadWorker.h"
#include "PSDActorsCoordinator.generated.h"

/** 
* This enum defines the PSDActor's body type on the physics region. It may
* have two types:
* 
* Primary: The PSDActor is currently being driven by the physics service region.
* Thus, the region is currently updating the PSDActor's Transform
* 
* Clone: The PSDActor exists only on the physics service, and its not being
* currently driven by the given physics service region. Thus, the region is NOT
* updating its Transform. A clone needs to exist on the physics service region
* on shared regions, but we cannot have two services updating its Transform.
*/
enum class EPSDActorBodyTypeOnPhysicsServiceRegion : uint8
{
	Primary,
	Clone
};

/** 
* The PSDActor physics service region footprint. This will keep the data of a 
* given PSDActor inside a physics service region. Thus, it will have the 
* physics service region id he is in and also the body type on the given
* physics service region.
* 
* A PSDActor may be inside a shared region, in which on a given region it may
* have a "Primary" body type (being driven by this physics service region),
* while on another it may have a "Clone" body type, meaning that the physics
* service only has a clone of the given PSDActor
* 
* @see EPSDActorBodyTypeOnPhysicsServiceRegion
*/
struct FPSDActorPhysicsServiceRegionFootprint
{
	/** The physics service region id that this PSDActor is currently in */
	int32 PhysicsServiceRegionId = 0;

	/** 
	* The body type on the physics service of this PSDActor on the given
	* physics service region.
	*/
	EPSDActorBodyTypeOnPhysicsServiceRegion BodyTypeOnPhysicsServiceRegion;
};

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
class REMOTEPHYSICSENGINESYSTEM_API APSDActorsCoordinator : public AActor
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
	bool IsSimulating() { return bIsSimulatingPhysics; }

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
	* Called once a PSDActor has entered a physics service region.
	*
	* @param EnteredPSDActor The PSDActor that has entered the given region
	* @param EnteredPhysicsRegionId The id from the physics service region the
	* PSDActor has entered
	*/
	UFUNCTION()
	void OnPSDActorEnteredPhysicsRegion(APSDActorBase* EnteredPSDActor,
		int32 EnteredPhysicsRegionId);

	/** 
	* Called once a PSDActor has exited a physics service region.
	*
	* @param ExitedPSDActor The PSDActor that has exited the given region
	* @param ExitedPhysicsRegionId The id from the physics service region the
	* PSDActor has exited
	*/
	UFUNCTION()
	void OnPSDActorExitPhysicsRegion(APSDActorBase* ExitedPSDActor,
		int32 ExitedPhysicsRegionId);

private:
	/** Gets all the physics services regions on the world. */
	void GetAllPhysicsServiceRegions();

	/** 
	* Updates the PSD actors Transform. This will request the physics service
	* server physics update and await its response. Once returned, will parse 
	* the result to update each PSD actor Transform.
	*/
	void UpdatePSDActors();

private:
	/**
	* Flag that indicates if this PSD actor coordinator is currently updating
	* the PSD actors
	*/
	bool bIsSimulatingPhysics = false;

	/** The list of physics service regions on the world. */
	TArray<class APhysicsServiceRegion*> PhysicsServiceRegionList;

	/** The PSDActors Spawner reference to request PSD Actors spawn */
	class APSDActorsSpawner* PSDActorsSpanwer = nullptr;

	/**
	* The TimerHandle that handles the PSD actors test (test-purposes only).
	*/
	FTimerHandle PSDActorsTestTimerHandle;

	/**
	* The map that stores the info of PSDActors that are inside shared 
	* physics service regions. As key we have the reference to the PSDActor
	* and as value we have a list of FPSDActorPhysicsServiceRegionFootprint
	* This will store the data of the PSDActor on the given physics service
	* region.
	* 
	* @see FPSDActorPhysicsServiceRegionFootprint
	*/
	TMap<class APSDActorBase*, TArray<FPSDActorPhysicsServiceRegionFootprint>> 
		SharedRegionsPSDActors;

	/** 
	* The socket client threads info list. This contains all the threads and
	* workers that implement each physics service region socket communication.
	* This is used to thread the physics update
	*/
	TMap<int32, TPair<class FSocketClientThreadWorker, class FRunnableThread*>>
		SocketClientThreadsInfoList;

	/** Counts the amount of step the physics system */
	uint32 StepPhysicsCounter = 0;
};
