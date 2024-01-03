// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsSimulation/Utils/PSDActorsSpawner.h"
#include "PhysicsSimulation/Base/PSDActorBase.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"

#include "Kismet/GameplayStatics.h"

APSDActorsSpawner::APSDActorsSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APSDActorsSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void APSDActorsSpawner::SpawnPSDActors
	(const int32 NumberOfActorsToSpawn)
{
	RPES_LOG_INFO(TEXT("Requested spawn of %d PSD Actors."),
		NumberOfActorsToSpawn);

	if (NumberOfActorsToSpawn <= 0)
	{
		RPES_LOG_ERROR(TEXT("Inform a positive number of PSD actors to \
			spawn."));
		return;
	}

	// Aux to count the number of spawned actors
	int32 NumberOfSpawnedActors = 0;

	// Initialize helper variables to spawn the actors
	float CurXPosToSpawn = 0.f;
	float CurYPosToSpawn = 0.f;
	float CurZPosToSpawn = 0.f;

	// While we are not finished spawning the required amount:
	while (NumberOfSpawnedActors < NumberOfActorsToSpawn)
	{
		// Set the current position equal to the max so we can start decreasing
		// the x-pos (thus, the first x-pos is MaxXPos)
		CurXPosToSpawn = MaxXPos;

		// Add the Z position to start a new layer (once X and Y has reached
		// the max number of spheres, will create another layer on top of
		// it and reset X and Y)
		CurZPosToSpawn += MinZPos;

		// While there's still space to spawn on this "line"
		while (CurXPosToSpawn > -950.f)
		{
			CurXPosToSpawn -= 150.f;
			CurYPosToSpawn = MaxYPos;

			// While there's still space to spawn on this "column"
			while (CurYPosToSpawn < 950.f)
			{
				CurYPosToSpawn += 150.f;

				// Create the position to spawn the PSD actor
				FVector PositionToSpawn(CurXPosToSpawn, CurYPosToSpawn, 
					CurZPosToSpawn);

				// Spawn PSD actor
				SpawnPSDActor(PositionToSpawn);

				// Increase and check if we reached the number of actors to
				// spawn
				NumberOfSpawnedActors++;
				if (NumberOfSpawnedActors == NumberOfActorsToSpawn)
				{
					break;
				}
			}

			// Check if we finished spawning
			if (NumberOfSpawnedActors == NumberOfActorsToSpawn)
			{
				break;
			}
		}
	}
}

APSDActorBase* APSDActorsSpawner::SpawnPSDActor(const FVector SpawnLocation)
{
	// Set the spawn params
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn PSD actor
	return GetWorld()->SpawnActor<APSDActorBase>(ActorToSpawn,
		SpawnLocation, FRotator::ZeroRotator, SpawnParams);
}

void APSDActorsSpawner::DestroyPSDActors()
{
	RPES_LOG_INFO(TEXT("Destroying all PSD Actors"));

	// Get all PSDActors
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorBase::StaticClass(), FoundActors);

	// Destroy each actor
	for (int i = FoundActors.Num() - 1; i >= 0; i--)
	{
		FoundActors[i]->Destroy();
	}
}
