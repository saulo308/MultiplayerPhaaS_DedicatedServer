// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsSimulation/Components/PSDActorSpawnerComponent.h"
#include "PhysicsSimulation/Base/PSDActorBase.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"


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
	(const FVector SpawnLocation, const int32 RegionOwnerPhysicsServiceId)
{
	RPES_LOG_INFO(TEXT("Spawning new PSDActor."));

	// Set the spawn params
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Set the spawn Transform
	FTransform SpawnTransform = FTransform(FRotator::ZeroRotator, 
		SpawnLocation, FVector::OneVector);

	// Spawn the PSD actor
	auto SpawnedActor = GetWorld()->SpawnActorDeferred<APSDActorBase>
		(PSDActorToSpawn, SpawnTransform, nullptr, nullptr, 
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	// Set the actor's owner physics service Id based on the region he is 
	// spawning in
	SpawnedActor->SetActorOwnerPhysicsServiceId(RegionOwnerPhysicsServiceId);

	// Finish spawning the actor
	SpawnedActor->FinishSpawning(SpawnTransform);

	return SpawnedActor;
}
