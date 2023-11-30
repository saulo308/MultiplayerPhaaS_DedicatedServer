// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PSDActorSpawnerComponent.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorBase.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"


UPSDActorSpawnerComponent::UPSDActorSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPSDActorSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPSDActorSpawnerComponent::TickComponent(float DeltaTime, 
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

APSDActorBase* UPSDActorSpawnerComponent::SpawnPSDActor
	(const FVector SpawnLocation)
{
	MPHAAS_LOG_INFO(TEXT("Spawning new PSDActor."));

	// Set the spawn params
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn PSD actor
	return GetWorld()->SpawnActor<APSDActorBase>(PSDActorToSpawn,
		SpawnLocation, FRotator::ZeroRotator, SpawnParams);
}
