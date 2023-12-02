// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PSDActorSpawnerComponent.generated.h"

/**
* The PSDActor spawner component. This is a ActorComponent used to spawn 
* PSDActors. This will usually be used with a physics service region to spawn
* PSDActors inside of it.
* 
* @see APhysicsServiceRegion
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTIPLAYERPHAAS_API UPSDActorSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	* Spawns a PSDActor on a given location.
	*
	* @param SpawnLocation The location to spawn the PSD actor
	* @param RegionOwnerPhysicsServiceId The region's owner physics service Id
	* that this PSDActor will spawn
	*
	* @return The spawned actor
	*/
	UFUNCTION(BlueprintCallable)
	class APSDActorBase* SpawnPSDActor(const FVector SpawnLocation, 
		const int32 RegionOwnerPhysicsServiceId);

public:	
	/** Sets default values for this component's properties */
	UPSDActorSpawnerComponent();

	/** Called every frame */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

private:
	/** The PSD actor reference to spawn on the area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, 
		meta=(AllowPrivateAccess="true"))
	TSubclassOf<class APSDActorBase> PSDActorToSpawn;
};
