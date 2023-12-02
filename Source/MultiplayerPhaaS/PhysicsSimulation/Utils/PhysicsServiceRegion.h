// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicsServiceRegion.generated.h"

/** 
* This is the physics service region. This represents the simulating area of a
* given physics service. If any PSDActor is whithin this area, it will be 
* simulated by the owning physics service of this region. If the PSDActor
* leaves this region, it will no longer be on the responsibility of this
* physics service's region owner (thus, will no longer be simulated by it).
*/
UCLASS()
class MULTIPLAYERPHAAS_API APhysicsServiceRegion : public AActor
{
	GENERATED_BODY()

public:
	/**
	* Spawns a new PSD Sphere. This will request the PSDActorsSpawner to
	* create a new PSDSphere as also requesting the physics service to add
	* the new sphere body on the physics system through the socket proxy.
	*
	* @param NewSphereLocation The sphere initial location to spawn
	*/
	UFUNCTION(BlueprintCallable)
	void SpawnNewPSDSphere(const FVector NewSphereLocation);

	/** */
	UFUNCTION(BlueprintCallable)
	void RemovePSDActorFromPhysicsService
		(class APSDActorBase* PSDActorToRemove);

public:	
	/** Sets default values for this actor's properties */ 
	APhysicsServiceRegion();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

public:
	/** 
	* Intitializes the physics service region. This will connect to the physics
	* service given the server ip address set on the properties. Once 
	* connected, will initialize the physics world on the service. This will be
	* done by getting all the PSD actors on this region.
	*/
	void InitializePhysicsServiceRegion();

	/** 
	* Updates all the PSDActors on this region by requesting a step physics
	* on the connected physics service. The response will be used to update all
	* the PSD actors on this region
	*/
	void UpdatePSDActorsOnRegion();

	/**
	* Clears this physics service region. This will disconnect from the physics
	* service and destroy all the PSDActors on this region.
	*/
	void ClearPhysicsServiceRegion();

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

private:
	/** 
	* Called when a new actor enters this region. Will only execute logic
	* if the actor is type of "APSDActorBase".
	*/
	UFUNCTION()
	void OnRegionEntry(UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/**
	* Called when a new actor exits this region. Will only execute logic
	* if the actor is type of "APSDActorBase".
	*/
	UFUNCTION()
	void OnRegionExited(UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/** 
	* Called when a actor has fully exited his own physics region. This is
	* used when a PSDActor enters this physics region. We will only migrate him
	* to this physics region once he has fully exited his own region
	*
	* @param ExitedActor The PSDActor that has fully exited his own physics 
	* region
	*/
	UFUNCTION()
	void OnActorFullyExitedOwnPhysicsRegion(APSDActorBase* ExitedActor);

private:
	/** 
	* Connects to a physics service serve. The server to connect is given 
	* by the server ip address on this actor's properties. 
	*/
	bool ConnectToPhysicsService();

	/** 
	* Prepares this physics service region for simulation. This will get all
	* the PSDActors inside this region as the actors to simulate with the 
	* physics service. This is also where we define the PSDActor's bodyid on
	* the physics system, constructing a TMap.
	*/
	void PreparePhysicsServiceRegionForSimulation();

	/**
	* Get all the PSDActors inside this region. This will update the
	* "PSDActorsOnRegion" and return it.
	*
	* @return The list of PSDActors inside this region
	*/
	TArray<class APSDActorBase*> GetAllPSDActorsOnRegion();

	/** 
	* Initializes this region's physics world. This will use all the PSDActors
	* gotten on the preparation phase and send a init message to the physics
	* service.
	*/
	void InitializeRegionPhysicsWorld();

public:
	/** 
	* The physics service ip address to connect this region to. This service
	* will be the one responsible for updating this region's physics actors
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PhysicsServiceIpAddr = FString();

	/** 
	* The physics service id that owns this region. This means that the
	* corresponding physics service with this id will be responsible for 
	* updating the PSDActors inside this region.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RegionOwnerPhysicsServiceId = 0;

private:
	/** 
	* The physics service region actor root component. Only used to easily
	* place the region on the world
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, 
		meta=(AllowPrivateAccess="true"))
	USceneComponent* RegionRootComponent = nullptr;

	/** 
	* The box component that collides with PSDActors. This represents the
	* physics service region itself. If any PSDActor is whithin this area, it 
	* will be simulated by the owning physics service of this region. If the 
	* PSDActor leaves this region, it will no longer be on the responsibility 
	* of this physics service's region owner (thus, will no longer be simulated
	* by it).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* PhysicsServiceRegionBoxComponent = nullptr;

private:
	/** The PSDActor spawner. Used to spawn PSDActors on this physics region */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = 
		(AllowPrivateAccess = "true"))
	class UPSDActorSpawnerComponent* PSDActorSpawner = nullptr;

private:
	/**
	* The list of PSD actors to simulate. The key is a unique Id to identify
	* it on the physics service and the value is the PSD actor reference
	* itself.
	*/
	TMap<int32, class APSDActorBase*> PSDActorsToSimulateMap;

	/**
	* The pending migration PSDActors list. This is used to add PSDActors that
	* has entered this physics region. Once they do, they will start a 
	* "pending migration" phase, which is only completed once the PSDActor
	* fully exits his own phyiscs region.
	*/
	TArray<class APSDActorBase*> PendingMigrationPSDActors;

private:
	/** */
	bool bIsPhysicsServiceRegionActive = false;

};
