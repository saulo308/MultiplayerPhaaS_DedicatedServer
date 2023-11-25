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
	* Get all the PSDActors inside this region. This will update the 
	* "PSDActorsOnRegion" and return it.
	* 
	* @return The list of PSDActors inside this region
	*/
	UFUNCTION(BlueprintCallable)
	TArray<class APSDActorBase*> GetAllPSDActorsOnRegion();

public:	
	/** Sets default values for this actor's properties */ 
	APhysicsServiceRegion();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

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
	
public:
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
	/** The list of PSDActors inside this region */
	TArray<class APSDActorBase*> PSDActorsOnRegion;
};
