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

	/**
	* Spawns a new PSD Sphere. This will request the PSDActorsSpawner to
	* create a new PSDSphere as also requesting the physics service to add
	* the new sphere body on the physics system through the socket proxy.
	*
	* @param NewSphereLocation The sphere initial location to spawn
	*/
	UFUNCTION(BlueprintCallable)
	void SpawnNewPSDSphere(const FVector NewSphereLocation, const FVector
		NewSphereLinearVelocity, const FVector
		NewSphereAngularVelocity);

public:	
	/** Sets default values for this actor's properties */ 
	APhysicsServiceRegion();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

public:
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
	* Clears this physics service region. This will disconnect from the physics
	* service and destroy all the PSDActors on this region.
	*/
	void ClearPhysicsServiceRegion();

	/**
	* Destroys a given PSDActor on this physics region. This will destroy the
	* PSDActor as well as remove it from the physics service.
	*
	* @param PSDActorToDestroy The PSDActor to destroy
	*/
	void DestroyPSDActorOnPhysicsRegion(APSDActorBase* PSDActorToDestroy);

	/** 
	* Getter to the cached dynamic PSDActors on the region. In constrats with
	* the "GetAllDynamicPSDActorOnRegion()" method, this will only return the 
	* TMap that should already be filled, instead of actually getting all 
	* actors inside of the region. 
	* 
	* @note This is done to avoid processing getting all actors again. Thus, 
	* make sure that the TMap is already defined before using this method.
	* 
	* @return The DynamicPSDActorsOnRegion TMap already filled previously
	*/
	TMap<int32, class APSDActorBase*> GetCachedDynamicPSDActorsOnRegion()
		{ return DynamicPSDActorsOnRegion; }

	/** 
	* Getter to the physics service region id.
	* 
	* @return The physics service region id
	*/
	int32 GetPhysicsServiceRegionId() const 
		{ return RegionOwnerPhysicsServiceId; }

	/**
	* Intitializes the physics service region. This will connect to the physics
	* service given the server ip address set on the properties. Once
	* connected, will initialize the physics world on the service. This will be
	* done by getting all the PSD actors on this region.
	* 
	* @param RegionPhysicsServiceIpAddr The physics service ip addr to connect the
	* region to. All physics simulation inside this region will be driven by
	* such physics service
	*/
	void InitializePhysicsServiceRegion(const FString& 
		RegionPhysicsServiceIpAddr);

	/**
	* Removes the ownership of this region from a given PSDActor. The process
	* of removing the ownership means that this physics service region will no
	* longer update the PSDActor's Transform on the update phase. Thus, this
	* will remove the PSDActor from the DynamicPSDActorsOnRegion.
	*
	* @param TargetPSDActor The PSDActor reference to remove the ownership from
	* this physics service region
	*/
	void RemovePSDActorOwnershipFromRegion
		(class APSDActorBase* TargetPSDActor);

	/**
	* Updates a given PSDActor body type on this region. This will send a 
	* message to the physics service to update the body type.
	* 
	* @param TargetPSDActor The PSDActor to update the body type
	* @param NewBodyType The new body type for the given PSDActor 
	* ("clone" or "primary")
	*/
	void UpdatePSDActorBodyType(const APSDActorBase* TargetPSDActor,
		const FString& NewBodyType);

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
	* Adds the ownership of this region to a given PSDActor. The process of
	* adding the ownership means that this physics service region will update
	* the PSDActor's Transform on the update phase. Thus, will add the PSDActor
	* to the DynamicPSDActors on region and update the PSDActor's owner physics
	* service region id
	*
	* @param TargetPSDActor The PSDActor reference to add the ownership to this
	* physics service region
	*/
	void SetPSDActorOwnershipToRegion(class APSDActorBase* TargetPSDActor);

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

private:
	/** 
	* Connects to a physics service serve. The server to connect is given 
	* by the server ip address on this actor's properties. 
	*/
	bool ConnectToPhysicsService();

	/** 
	* Gets all the dynamic PSDActors on this region. This will get all the 
	* PSDActors inside this region that will be constantly be updated by the
	* physics service at each step.
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

public:
	/** 
	* The physics service ip address to connect this region to. This service
	* will be the one responsible for updating this region's physics actors.
	* 
	* @note This will be set by the PSDActorsCoodinator once the simulation 
	* starts.
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

	/** The PSDActor spawner. Used to spawn PSDActors on this physics region */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta =
		(AllowPrivateAccess = "true"))
	class UPSDActorSpawnerComponent* PSDActorSpawner = nullptr;

	/** 
	* The physics service region actor root component. Only used to easily
	* place the region on the world
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, 
		meta=(AllowPrivateAccess="true"))
	USceneComponent* RegionRootComponent = nullptr;

private:
	/**
	* Flag that indicates if this physics service region is currently active.
	* I.e. if it contains PSDActors and is updating them
	*/
	bool bIsPhysicsServiceRegionActive = false;

	/**
	* The list of dynamic PSDActors. Theses are the PSDActors that will be 
	* updated on each physics step. The key is a unique Id to identify it on 
	* the physics service and the value is the PSD actor reference itself.
	*/
	TMap<int32, class APSDActorBase*> DynamicPSDActorsOnRegion;
};
