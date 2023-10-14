// Fill out your copyright notice in the Description page of Project Settings.


#include "PSDActorsSpawner.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorBase.h"
#include "Kismet/GameplayStatics.h"

APSDActorsSpawner::APSDActorsSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APSDActorsSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void APSDActorsSpawner::SpawnPSDActors(const int32 NumberOfActorsToSpawn)
{
	if (NumberOfActorsToSpawn <= 0 || NumberOfActorsToSpawn > 2000)
	{
		UE_LOG(LogTemp, Error, TEXT("Should spawn only 0 to 2000 actors!"));
		return;
	}

	int32 NumberOfSpawnedActors = 0;
	float CurXPosToSpawn = 0.f;
	float CurYPosToSpawn = 0.f;
	float CurZPosToSpawn = 0.f;

	while (NumberOfSpawnedActors < NumberOfActorsToSpawn)
	{
		CurXPosToSpawn = MaxXPos;
		CurZPosToSpawn += MinZPos;
		while (CurXPosToSpawn > -950.f)
		{
			CurXPosToSpawn -= 150.f;
			CurYPosToSpawn = MaxYPos;

			while (CurYPosToSpawn < 950.f)
			{
				CurYPosToSpawn += 150.f;

				FVector PositionToSpawn(CurXPosToSpawn, CurYPosToSpawn, 
					CurZPosToSpawn);

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = 
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				GetWorld()->SpawnActor<APSDActorBase>(ActorToSpawn,
					PositionToSpawn, FRotator::ZeroRotator, SpawnParams);
				NumberOfSpawnedActors++;

				if (NumberOfSpawnedActors == NumberOfActorsToSpawn)
				{
					break;
				}
			}

			if (NumberOfSpawnedActors == NumberOfActorsToSpawn)
			{
				break;
			}
		}
	}
}

void APSDActorsSpawner::DestroyPSDActors()
{
	// Get all PSDActors
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorBase::StaticClass(), FoundActors);

	for (int i = FoundActors.Num() - 1; i >= 0; i--)
	{
		FoundActors[i]->Destroy();
	}
}
