// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#include "PhysicsSimulation/Utils/Actors/PSDActorsCoordinator.h"
#include "PhysicsSimulation/Utils/Actors/PSDActorsSpawner.h"
#include "PhysicsSimulation/Utils/Actors/PhysicsServiceRegion.h"
#include "ExternalCommunication/Sockets/SocketClientProxy.h"
#include "PhysicsSimulation/PSDActors/Base/PSDActorBase.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

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

	// Foreach physics service region, create a thread for its communication
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		// Get the physics region physics service id
		const int32 RegionPhysicsServiceId =
			PhysicsServiceRegion->RegionOwnerPhysicsServiceId;

		// Create a new worker for this given region 
		FSocketClientThreadWorker NewSocketClientWorker
			(RegionPhysicsServiceId);

		// Create the new thread for the worker
		FRunnableThread* NewSocketClientThread = FRunnableThread::Create
			(&NewSocketClientWorker, TEXT("SocketClientWorkerThread"));

		// Create a new thread pair for the given work and thread
		auto NewSocketClientThreadInfoPair = 
			TPair<FSocketClientThreadWorker, FRunnableThread*>
			(NewSocketClientWorker, NewSocketClientThread);

		// Add the threading info the the list
		SocketClientThreadsInfoList.Add(RegionPhysicsServiceId,
			NewSocketClientThreadInfoPair);
	}
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
	RPES_LOG_INFO(TEXT("Starting PSD actors simulation."));

	// For each physics service region on the world, initialize it
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		PhysicsServiceRegion->InitializePhysicsServiceRegion();
	}

	// Set the flag to start simulating on each tick
	bIsSimulatingPhysics = true;

	RPES_LOG_INFO(TEXT("PSD actors started simulating."));
}

void APSDActorsCoordinator::StopPSDActorsSimulation()
{
	if (!bIsSimulatingPhysics)
	{
		return;
	}

	RPES_LOG_INFO(TEXT("Stopping PSD actors simulation."));
	
	// Set the flag to false to stop ticking PSDActors' update
	bIsSimulatingPhysics = false;

	// For each physics service region on the world, clear it
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		PhysicsServiceRegion->ClearPhysicsServiceRegion();
	}

	RPES_LOG_INFO(TEXT("PSD actors simulation has been stopped."));
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

	RPES_LOG_WARNING(TEXT("Stepping: %d"), StepPhysicsCounter++);
	RPES_LOG_INFO(TEXT("Updating PSD actors for this frame."));

	// For each socket client thread info, set the message to "step" and send
	// for each physics service region (we know that each thread represents
	// a given physics region)
	for (auto& SocketClientThreadInfo : SocketClientThreadsInfoList)
	{
		// Get the thread info and the thread worker
		auto& ThreadInfoPair = SocketClientThreadInfo.Value;
		auto& ThreadWoker = ThreadInfoPair.Key;

		// Set the message to send on the worker
		ThreadWoker.SetMessageToSend("Step\nMessageEnd\n");

		// Toggle the run flag and run thread
		ThreadInfoPair.Key.ToggleShouldRun();
		ThreadInfoPair.Key.Run();
	}

	// For each socket client thread info, await its completion and get the
	// physics service response so we can delegate the update to the 
	// corresponding physics service
	for (auto& SocketClientThreadInfo : SocketClientThreadsInfoList)
	{
		// Get the thread info
		auto& ThreadInfoPair = SocketClientThreadInfo.Value;

		// Get the worker and the corresponding thread
		auto& ThreadWorker = ThreadInfoPair.Key;
		auto& Thread = ThreadInfoPair.Value;

		// Await the thread completion
		Thread->WaitForCompletion();

		// Once completed, get the response on the worker
		FString Result = ThreadWorker.GetResponse();

		// Get the region physics service id this thread represents so we can
		// find and update it
		const int32 RegionPhysicsServiceId = SocketClientThreadInfo.Key;
		
		// Foreach physics service region on the world, update the PSDActors 
		// on it
		for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
		{
			// Check if the physics service region is the same as the thread's
			// corresponding ID
			const bool bIsTargetPhysicsServiceRegion = 
				(RegionPhysicsServiceId ==
				PhysicsServiceRegion->RegionOwnerPhysicsServiceId);

			// If found the physics service region, update it
			if (bIsTargetPhysicsServiceRegion)
			{
				PhysicsServiceRegion->UpdatePSDActorsOnRegion(Result);
			}
		}
	}

	RPES_LOG_INFO(TEXT("Physics updated for this frame."));
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
