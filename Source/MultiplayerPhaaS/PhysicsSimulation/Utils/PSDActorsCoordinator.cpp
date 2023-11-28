// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#include "PSDActorsCoordinator.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Utils/PSDActorsSpawner.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Utils/PhysicsServiceRegion.h"
#include "MultiplayerPhaaS/ExternalCommunication/Sockets/SocketClientProxy.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"
#include "Kismet/GameplayStatics.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorBase.h"
#include "Net/UnrealNetwork.h"

#include <string>
#include <iostream>
#include <sstream>
#include <chrono>

const int32 DefaultServerId = 0;

APSDActorsCoordinator::APSDActorsCoordinator()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APSDActorsCoordinator::BeginPlay()
{
	Super::BeginPlay();

	// Get all physics service region on the map
	GetAllPhysicsServiceRegions();
}

void APSDActorsCoordinator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPSDActorsSimulation();
}

void APSDActorsCoordinator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSimulatingPhysics && HasAuthority())
	{
		// Update PSD actors by simulating physics on the service
		// and parsing it's results with the new actor position
		UpdatePSDActors();
	}
}

void APSDActorsCoordinator::StartPSDActorsSimulation
	(const TArray<FString>& SocketServerIpAddrList)
{
	MPHAAS_LOG_INFO(TEXT("Starting PSD actors simulation."));

	// For each physics service region on the world, initialize it
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		PhysicsServiceRegion->InitializePhysicsServiceRegion();
	}

	// Set the flag to start simulating on each tick
	bIsSimulatingPhysics = true;

	MPHAAS_LOG_INFO(TEXT("PSD actors started simulating."));
}

void APSDActorsCoordinator::StopPSDActorsSimulation()
{
	if (!bIsSimulatingPhysics)
	{
		return;
	}

	MPHAAS_LOG_INFO(TEXT("Stopping PSD actors simulation."));
	
	// Set the flag to false to stop ticking PSDActors' update
	bIsSimulatingPhysics = false;

	// For each physics service region on the world, clear it
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		PhysicsServiceRegion->ClearPhysicsServiceRegion();
	}

	MPHAAS_LOG_INFO(TEXT("PSD actors simulation has been stopped."));
}

void APSDActorsCoordinator::StartPSDActorsSimulationTest
	(const TArray<FString>& SocketServerIpAddrList, 
	float TestDurationInSeconds/*=30.f*/)
{
	// Start the PSD actors simulation
	StartPSDActorsSimulation(SocketServerIpAddrList);

	// Start timer to stop the simulation after 30 seconds passed
	GetWorld()->GetTimerManager().SetTimer(PSDActorsTestTimerHandle, this,
		&APSDActorsCoordinator::StopPSDActorsSimulation, TestDurationInSeconds, 
		false);
}

void APSDActorsCoordinator::UpdatePSDActors()
{
	// Check if we are simulating
	if (!bIsSimulatingPhysics)
	{
		return;
	}

	MPHAAS_LOG_INFO(TEXT("Updating PSD actors for this frame."));
	
	// Foreach physics service region on the world, update the PSDActors on it
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		PhysicsServiceRegion->UpdatePSDActorsOnRegion();
	}

	MPHAAS_LOG_INFO(TEXT("Physics updated for this frame."));
}

void APSDActorsCoordinator::GetAllPhysicsServiceRegions()
{
	// Get all actors from "APhysicsServiceRegion"
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APhysicsServiceRegion::StaticClass(), FoundActors);

	// Foreach found actor, cast it to "APhysicsServiceRegion"
	for (auto& FoundActor : FoundActors)
	{
		// Cast the found actor
		const auto FoundPhysicsServiceRegion = 
			Cast<APhysicsServiceRegion>(FoundActor);

		// Add to list
		PhysicsServiceRegionList.Add(FoundPhysicsServiceRegion);
	}
}
