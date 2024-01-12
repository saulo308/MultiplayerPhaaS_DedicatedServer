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

		// Get all the dynamic PSDActors on this region
		const auto DynamicPSDActorsOnRegion = PhysicsServiceRegion->
			GetCachedDynamicPSDActorsOnRegion();

		// For each dynamic PSDActor, bind the OnEnteredPhysicsRegion and
		// OnExitedPhysicsRegion so the coordinator knows once it occurs
		for (const auto& DynamicPSDActorOnRegion : DynamicPSDActorsOnRegion)
		{
			// Get the PSDActor from the TMap
			const auto& PSDActor = DynamicPSDActorOnRegion.Value;

			// Bind the delegates
			PSDActor->OnActorEnteredPhysicsRegion.AddDynamic(this, 
				&APSDActorsCoordinator::OnPSDActorEnteredPhysicsRegion);
			PSDActor->OnActorExitedPhysicsRegion.AddDynamic(this,
				&APSDActorsCoordinator::OnPSDActorExitPhysicsRegion);
		}
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

void APSDActorsCoordinator::OnPSDActorEnteredPhysicsRegion
	(APSDActorBase* EnteredPSDActor, int32 EnteredPhysicsRegionId)
{
	RPES_LOG_WARNING(TEXT("PSDActor \"%s\" has entered region with id: %d."
		"Entried pos: %s."), *EnteredPSDActor->GetName(),
		EnteredPhysicsRegionId,
		*EnteredPSDActor->GetActorLocation().ToString());

	// Remove the bind to avoid it being called twice
	EnteredPSDActor->OnActorExitedPhysicsRegion.RemoveDynamic(this,
		&APSDActorsCoordinator::OnPSDActorEnteredPhysicsRegion);

	// Get the PSDActor owner physics service Id
	const int32 EnteredPSDActorCurrentOwnerPhysicsServiceId =
		EnteredPSDActor->GetActorOwnerPhysicsServiceId();

	// If the owner physics service id is the same as the one he has entered
	// just ignore, we want only PSDActors that entered a shared region
	if (EnteredPSDActorCurrentOwnerPhysicsServiceId == EnteredPhysicsRegionId)
	{
		return;
	}

	// If not the same, the actor has entered a different physics service 
	// region ID, and therefore, is on a shared region

	// Update the PSDActor's region status to inform he is on a shared region
	// (this may be already set as is if the PSDActor is entering a third
	// physics region)
	EnteredPSDActor->UpdatePSDActorStatusOnRegion
		(EPSDActorPhysicsRegionStatus::SharedRegion);

	// Check if the physics service region does exist
	if (!PhysicsServiceRegionList.IsValidIndex(EnteredPhysicsRegionId))
	{
		RPES_LOG_ERROR(TEXT("No physics region with id: %d on the coordinator "
			"list."), EnteredPhysicsRegionId);
		return;
	}

	// Create a clone on the entered physics service region
	PhysicsServiceRegionList[EnteredPhysicsRegionId]->
		AddPSDActorCloneOnPhysicsService(EnteredPSDActor);

	// Check if the PSDActor is already on the SharedRegionsPSDActors Map (this
	// will happen when this PSDActor enters a third physics region 
	// simultaneously)
	auto EnteredPSDActorFootprintInfo = 
		SharedRegionsPSDActors.Find(EnteredPSDActor);

	// If not, create a new entry for it
	if (!EnteredPSDActorFootprintInfo)
	{
		// We will have two entries: One for the current owner physics region
		// and another for the region the PSDActor has just entered
		FPSDActorPhysicsServiceRegionFootprint PrimaryFootprint;

		// Create the data for the current owner physics region
		PrimaryFootprint.PhysicsServiceRegionId = 
			EnteredPSDActorCurrentOwnerPhysicsServiceId;
		PrimaryFootprint.BodyTypeOnPhysicsServiceRegion = 
			EPSDActorBodyTypeOnPhysicsServiceRegion::Primary;

		// Add the PSDActor to the shared region PSDActors
		SharedRegionsPSDActors.Add(EnteredPSDActor, { PrimaryFootprint }); 
		
		// Set the enteres PSDActor footprint info
		EnteredPSDActorFootprintInfo = 
			SharedRegionsPSDActors.Find(EnteredPSDActor);
	}

	// For correctenss-sake, check if this is valid (should always be valid
	// at this point)
	check(EnteredPSDActorFootprintInfo);

	// Create the clone footprint
	FPSDActorPhysicsServiceRegionFootprint CloneFootprint;

	// Create the data for the region he has just entered
	CloneFootprint.PhysicsServiceRegionId = EnteredPhysicsRegionId;
	CloneFootprint.BodyTypeOnPhysicsServiceRegion =
		EPSDActorBodyTypeOnPhysicsServiceRegion::Clone;

	// Add the clone footprint to the TArray for this actor
	SharedRegionsPSDActors[EnteredPSDActor].Add(CloneFootprint);

	RPES_LOG_WARNING(TEXT("PSDActorsCoordinator has registered PSDActor on the"
		" shared regions actors."));
}

void APSDActorsCoordinator::OnPSDActorExitPhysicsRegion
	(APSDActorBase* ExitedPSDActor, int32 ExitedPhysicsRegionId)
{
	RPES_LOG_WARNING(TEXT("PSDActor \"%s\" has exited region with id: %d. "
		"Exited pos: %s."), *ExitedPSDActor->GetName(), ExitedPhysicsRegionId,
		*ExitedPSDActor->GetActorLocation().ToString());

	// Remove the bind to avoid it being called twice
	ExitedPSDActor->OnActorExitedPhysicsRegion.RemoveDynamic(this, 
		&APSDActorsCoordinator::OnPSDActorExitPhysicsRegion);

	// Get the PSDActor owner physics service Id
	const int32 ExitedPSDActorCurrentOwnerPhysicsServiceId =
		ExitedPSDActor->GetActorOwnerPhysicsServiceId();

	// If the owner physics service id is the same as the one he has exited,
	// it means he is exiting his owner:
	if (ExitedPSDActorCurrentOwnerPhysicsServiceId == ExitedPhysicsRegionId)
	{
		// Check if the PSDActor is already on the SharedRegionsPSDActors Map
		auto* ExitedPSDActorFootprintInfo = 
			SharedRegionsPSDActors.Find(ExitedPSDActor);

		// If not shared region info for this PSDActor, this PSDActor has 
		// exited every region of the world. Therefore, will be destroyed
		if (!ExitedPSDActorFootprintInfo)
		{
			// Check if the physics service region does exist
			if (!PhysicsServiceRegionList.IsValidIndex(ExitedPhysicsRegionId))
			{
				RPES_LOG_ERROR(TEXT("No physics region with id: %d on the "
					"coordinator list."), ExitedPhysicsRegionId);
				return;
			}

			// Request the physics region to destroy this PSDActor as he is 
			// no longer in any region
			PhysicsServiceRegionList[ExitedPhysicsRegionId]->
				DestroyPSDActorOnPhysicsRegion(ExitedPSDActor);

			RPES_LOG_WARNING(TEXT("Destroyed PSDActor as he is no longer "
				"is any physics region service."));
			return;
		}

		// If the shared region info does exists, then we should transfer this
		// PSDActor to another physics service region (migrate)

		// check if there are at list two footprints. If not, something is 
		// wrong. The PSDActor should be at least on two regions to be on the
		// shared regions PSDActors map
		check(ExitedPSDActorFootprintInfo->Num() >= 2);

		// Get the first clone physics service region id (the one that will be
		// promoted to primary region for this PSDActor)
		const int32 FirstClonePhysicsServiceRegionId = 
			(*ExitedPSDActorFootprintInfo)[1].PhysicsServiceRegionId;

		// Check if the physics service region does exist
		if (!PhysicsServiceRegionList.IsValidIndex
			(FirstClonePhysicsServiceRegionId))
		{
			RPES_LOG_ERROR(TEXT("No physics region with id: %d on the "
				"coordinator list to spawn PSDActor from clone."), 
				ExitedPhysicsRegionId);
			return;
		}

		// Request the new region to spawn the PSDActor from its clone
		const auto SpawnedPSDActor = 
			PhysicsServiceRegionList[FirstClonePhysicsServiceRegionId]->
			SpawnPSDActorFromPhysicsServiceClone(ExitedPSDActor);

		RPES_LOG_WARNING(TEXT("Spawned a PSDActor from clone on region "
			"(id:%d)."), FirstClonePhysicsServiceRegionId);

		// Bind the delegates
		SpawnedPSDActor->OnActorEnteredPhysicsRegion.AddDynamic(this,
			&APSDActorsCoordinator::OnPSDActorEnteredPhysicsRegion);
		SpawnedPSDActor->OnActorExitedPhysicsRegion.AddDynamic(this,
			&APSDActorsCoordinator::OnPSDActorExitPhysicsRegion);

		// If the TArray of shared footprint only has two info, it means now
		// that he will only have one, and therefore, can be excluded from the
		// coordinator shared regions PSDActors map
		if (ExitedPSDActorFootprintInfo->Num() == 2)
		{
			SharedRegionsPSDActors.Remove(ExitedPSDActor);
		}
		else
		{
			// If not, we will create a new entry for the new primary PSDActor
			
			// Update the shared regions map TArray to remove the first region
			// (which should be the primary - now exited)
			ExitedPSDActorFootprintInfo->RemoveAt(0);

			// Get the second region, which should be promoted to primary now
			// as it will be owning the PSDActor
			auto& ClonePhysicsServiceRegion = 
				(*ExitedPSDActorFootprintInfo)[0];

			// Update the clone physics region to primary as now he will be 
			// the new owner of the PSDActor
			ClonePhysicsServiceRegion.BodyTypeOnPhysicsServiceRegion =
				EPSDActorBodyTypeOnPhysicsServiceRegion::Primary;

			// Get a copy of this footprint TArray
			TArray<FPSDActorPhysicsServiceRegionFootprint> 
				NewActorFootprint = SharedRegionsPSDActors[ExitedPSDActor];

			// Create a new shared PSDActor info on the map with the newly
			// spawned PSDActor
			SharedRegionsPSDActors.Add(SpawnedPSDActor, NewActorFootprint);

			// Remove the old footprint info
			SharedRegionsPSDActors.Remove(ExitedPSDActor);
		}

		// Destroy the exited PSDActor, as he will no longer be owned by his
		// current physics service region (we have spawned a new one with a
		// new owner physics service region)
		PhysicsServiceRegionList[ExitedPhysicsRegionId]->
			DestroyPSDActorOnPhysicsRegion(ExitedPSDActor);

		RPES_LOG_WARNING(TEXT("Destroyed the PSDActor that exited region "
			"(id:%d)."), ExitedPhysicsRegionId);

		return;
	}

	// If the owner physics service id is NOT the same as the one he has 
	// exited, it means he is exiting a physics service region that has him
	// as a clone

	// Check if the physics service region does exist
	if (!PhysicsServiceRegionList.IsValidIndex(ExitedPhysicsRegionId))
	{
		RPES_LOG_ERROR(TEXT("No physics region with id: %d on the coordinator "
			"list."), ExitedPhysicsRegionId);
		return;
	}

	// Request the removal of the clone from the physics service
	PhysicsServiceRegionList[ExitedPhysicsRegionId]->
		RemovePSDActorFromPhysicsService(ExitedPSDActor);

	RPES_LOG_WARNING(TEXT("Removed PSDActor clone from phyhsics service "
		"region (id:%d)."), ExitedPhysicsRegionId);
			
	// Get the footprint info from the SharedRegionsPSDActors Map
	auto* ExitedPSDActorFootprintInfo =
		SharedRegionsPSDActors.Find(ExitedPSDActor);
	
	// Should be valid here, as he is exiting a outer physics service region.
	// Thus, should be on the shared regions PSDActors map
	check(ExitedPSDActorFootprintInfo);

	// check if there are at list two footprints. If not, something is 
	// wrong. The PSDActor should be at least on two regions to be on the
	// shared regions PSDActors map
	check(ExitedPSDActorFootprintInfo->Num() >= 2);

	// For each footprint info, find the one with the current exited physics
	// service region id and remove it
	for (int32 i = ExitedPSDActorFootprintInfo->Num() - 1; i >= 0; i--)
	{
		if ((*ExitedPSDActorFootprintInfo)[i].PhysicsServiceRegionId
			== ExitedPhysicsRegionId)
		{
			ExitedPSDActorFootprintInfo->RemoveAt(i);

			RPES_LOG_WARNING(TEXT("Removed PSDActor footprint (from region "
				"with id: %d) from SharedRegionsPSDActors"),
				ExitedPhysicsRegionId);
			break;
		}
	}

	// If there is only one footprint left, then the PSDActor does not need to 
	// be on the shared regions PSDActors map
	if (ExitedPSDActorFootprintInfo->Num() == 1)
	{
		SharedRegionsPSDActors.Remove(ExitedPSDActor);
	}
}