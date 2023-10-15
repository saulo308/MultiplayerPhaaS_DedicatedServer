// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "BouncingSpheresMainW.h"
#include "Kismet/GameplayStatics.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorsCoordinator.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Utils/PSDActorsSpawner.h"

void UBouncingSpheresMainW::NativeConstruct()
{
	// Get all actors from the class "APSDActorsCoordinator"
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorsCoordinator::StaticClass(), FoundActors);

	// Check if found at least one
	check(FoundActors.Num() > 0);

	// Cast the found actor to "APSDActorsCoordinator"
	PSDActorCoordinator = Cast<APSDActorsCoordinator>(FoundActors[0]);

	// Get all actors from the class "APSDActorsSpawner"
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorsSpawner::StaticClass(), FoundActors);

	// Check if found at least one
	check(FoundActors.Num() > 0);

	// Cast the found actor to "APSDActorsSpawner"
	PSDActorSpawner = Cast<APSDActorsSpawner>(FoundActors[0]);
}

void UBouncingSpheresMainW::StartPSDActorsSimulation(const FString& ServerIpAddress)
{
	// Check if the coordinator is valid
	check(PSDActorCoordinator.Get());

	// Request start simulation
	PSDActorCoordinator->StartPSDActorsSimulation(ServerIpAddress);
}

void UBouncingSpheresMainW::StopPSDActorsSimulation()
{
	// Check if the coordinator is valid
	check(PSDActorCoordinator.Get());

	// Request stop simulation
	PSDActorCoordinator->StopPSDActorsSimulation();
}

void UBouncingSpheresMainW::SpawnPSDActors(const int32 NumberOfActorsToSpawn)
{
	// Check if the spawner is valid
	check(PSDActorSpawner.Get());

	// Request spawner to spawn the number of actors. This is a RPC call to
	// the server
	PSDActorSpawner->SpawnPSDActors(NumberOfActorsToSpawn);
}

void UBouncingSpheresMainW::DestroyAllPSDActors()
{
	// Check if the spawner is valid
	check(PSDActorSpawner.Get());

	// Request spawner to destroy the number of actors. This is a RPC call to
	// the server
	PSDActorSpawner->DestroyPSDActors();
}

void UBouncingSpheresMainW::StartPSDActorsTest(const FString& ServerIpAddress, 
	const int32 NumberOfActorsToSpawn)
{
	// First, destroy all PSD actors on level
	DestroyAllPSDActors();

	// Spawn the number of actors requested
	SpawnPSDActors(NumberOfActorsToSpawn);

	// Check if the coordinator is valid
	check(PSDActorCoordinator.Get());

	// Start the PSD actors test (for 30 seconds)
	PSDActorCoordinator->StartPSDActorsSimulationTest(ServerIpAddress, 30.f);
}
