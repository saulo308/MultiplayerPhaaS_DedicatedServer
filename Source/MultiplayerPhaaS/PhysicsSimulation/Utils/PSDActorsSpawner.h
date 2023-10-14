// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PSDActorsSpawner.generated.h"

/** 
* This class deals with spawning PSD actors in a level. This should be used for
* testing-purporses only. This will take a square are starting from (0,0,0) 
* to (MaxXPos, MaxYPos, MaxZPos) to spawn PSD actors with an offset among them. 
* When spawning, if the area is full with PSD actors, will spawn on top of the
* last layer with an Y-offset.
* 
* Moreover, this is used with "PSDActorsCoordinator" "StartPSDActorsTest()" 
* method to test the PSD actors physics simulation logic.
* 
* @see PSDActorsCoordinator
*/
UCLASS()
class MULTIPLAYERPHAAS_API APSDActorsSpawner : public AActor
{
	GENERATED_BODY()

public:
	/** 
	* Spawns the given number of PSD actors on a square area starting from
	* (0, 0, 0) to (MaxXPos, MaxYPos, MaxZPos). Then, should use a start 
	* PSD actors simulation to start simulating these actors.
	* 
	* @param NumberOfActorsToSpawn The number of actors to spawn on the area.
	*/
	UFUNCTION(BlueprintCallable)
	void SpawnPSDActors(const int32 NumberOfActorsToSpawn);

	/** Destroys all the PSD actors on the level. */
	UFUNCTION(BlueprintCallable)
	void DestroyPSDActors();

public:	
	/** Sets default values for this actor's properties */
	APSDActorsSpawner();

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

public:
	/** The PSD actor reference to spawn on the area. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<class APSDActorBase> ActorToSpawn;

	/**
	* The max X position the spawned PSD actors can have. If reaches this
	* limit, will create another layer on top of the last one.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxXPos = 1100.f;

	/**
	* The max Y position the spawned PSD actors can have. If reaches this
	* limit, will create another layer on top of the last one.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxYPos = -1100.f;

	/**
	* The max Z position the spawned PSD actors can have. If reaches this
	* limit, will create another layer on top of the last one.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MinZPos = 220.f;
};
