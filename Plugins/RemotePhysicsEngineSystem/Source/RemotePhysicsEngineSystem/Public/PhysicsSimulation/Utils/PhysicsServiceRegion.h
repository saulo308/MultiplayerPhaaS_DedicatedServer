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
class REMOTEPHYSICSENGINESYSTEM_API APhysicsServiceRegion : public AActor
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

	/** 
	* Remvoes a PSDActor from the physics service. This will send a message to
	* this region's owning physics service requesting the removal of a given
	* PSDActor. 
	* 
	* @param PSDActorToRemove The PSDActor on this region to remove from the
	* owning physics service
	*/
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
	* on the connected physics service. The param is the physics simulation
	* result. This should be given, as the coordinator will thread the physics
	* service update and syncronize the update
	* 
	* @param PhysicsSimulationResultStr The physics simulation response for 
	* this given step to update the PSDActors on this physics region
	*/
	void UpdatePSDActorsOnRegion(const FString& PhysicsSimulationResultStr);

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
	* Gets all the dynamic PSDActors on this region. Thus, this will get all
	* the PSDActors inside this region that will be constantly be updated by 
	* the physics service at each step.
	*/
	void GetAllDynamicPSDActorOnRegion();

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

private:
	/** 
	* Adds a PSDActor clone on the physics service. This is needed once a 
	* PSDActor enters this region. Once this happens, we spawn a clone to 
	* represent entered PSDActor so the PSDActors on this region can collide
	* with it. Evetually, we may spawn this PSDActor clone on the physics
	* region, if he exits his previous physics region.
	* 
	* @note The PSDActor will not acctually spawn on this physics region, but
	* only on the physics service.
	* 
	* @param PSDActorToClone The PSDActor to clone on the physics service
	*/
	void AddPSDActorCloneOnPhysicsService
		(const APSDActorBase* PSDActorToClone);

	/** 
	* Spawns a PSDActor from a physics service clone. This happens once a 
	* PSDActor exits his previous PhysicsServiceRegion. When this happens, we
	* may spawn the PSDActor in this region, finishing the PSDActor migration.
	* 
	* @note It is important to note that the PSDActor's clone has to
	* previously exist as a clone on this physics region's phyiscs service
	* 
	* @param TargetClonedPSDActor The PSDActor that will be spawned from the
	* physics service
	*/
	void SpawnPSDActorFromPhysicsServiceClone
		(const APSDActorBase* TargetClonedPSDActor);

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
	* The list of dynamic PSDActors. Theses are the PSDActors that will be 
	* updated on each physics step. The key is a unique Id to identify it on 
	* the physics service and the value is the PSD actor reference itself.
	*/
	TMap<int32, class APSDActorBase*> DynamicPSDActorsOnRegion;

	/**
	* The pending migration PSDActors list. This is used to add PSDActors that
	* has entered this physics region. Once they do, they will start a 
	* "pending migration" phase, which is only completed once the PSDActor
	* fully exits his own phyiscs region.
	*/
	TArray<class APSDActorBase*> PendingMigrationPSDActors;

private:
	/** 
	* Flag that indicates if this physics service region is currently active.
	* I.e. if it contains PSDActors and is updating them
	*/
	bool bIsPhysicsServiceRegionActive = false;
};
