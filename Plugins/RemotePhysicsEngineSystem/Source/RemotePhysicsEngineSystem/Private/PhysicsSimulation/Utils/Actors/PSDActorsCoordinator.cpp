// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#include "PhysicsSimulation/Utils/Actors/PSDActorsCoordinator.h"
#include "PhysicsSimulation/Utils/Actors/PSDActorsSpawner.h"
#include "PhysicsSimulation/Utils/Actors/PhysicsServiceRegion.h"
#include "ExternalCommunication/Sockets/SocketClientProxy.h"
#include "PhysicsSimulation/PSDActors/Base/PSDActorBase.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "GenericPlatform/GenericPlatformMemory.h"

#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

constexpr char GetCpuUsageCommand[] = "WMIC CPU GET LoadPercentage | findstr [0-9]";
const int32 DefaultServerId = 0;

APSDActorsCoordinator::APSDActorsCoordinator()
{
	PrimaryActorTick.bCanEverTick = true;

	// Replication setup...
	bReplicates = true;
	bAlwaysRelevant = true;
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

	if (bIsSimulatingPhysics)
	{
		if (HasAuthority())
		{
			// Check if the simulation timer remaining is less then 15s. If so,
			// measure the CPU and RAM
			if (!bHasMeasuredCpuAndRamForSimulation &&
				(GetWorld()->GetTimerManager().GetTimerRemaining
				(PSDActorsTestTimerHandle) <= 15.f))
			{
				// Set the flag as we will measure the CPU and RAM
				bHasMeasuredCpuAndRamForSimulation = true;

				// Get memory measurement (will muiltcast to clients)
				GetRamMeasurement();

				// Get and store the cpu percentage usage (will muiltcast to 
				// clients)
				GetCPUMeasurement();
			}
			else
			{
				// If not, just measure the DeltaTime. Here we do this as
				// the CPU and RAM usage will affect the delta time. So we 
				// don't want to measure the DeltaTime during it
				DeltaTimeMeasurement += FString::Printf(TEXT("%f\n"),
					DeltaTime);
				
			}
		}
		else
		{
			// Measure the delta time on client
			DeltaTimeMeasurement += FString::Printf(TEXT("%f\n"),
				DeltaTime);
		}
	}

	if (bIsSimulatingPhysics && HasAuthority())
	{
		// Update PSD actors by simulating physics on the service
		// and parsing it's results with the new actor position
		UpdatePSDActors();
	}
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

	// Sort the physics service region list by their id
	PhysicsServiceRegionList.Sort([](const APhysicsServiceRegion& RegionA, 
		const APhysicsServiceRegion& RegionB)
	{
		return RegionA.GetPhysicsServiceRegionId() 
			< RegionB.GetPhysicsServiceRegionId();
	});
}

void APSDActorsCoordinator::GetLifetimeReplicatedProps
	(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate simualting physics 
	DOREPLIFETIME(APSDActorsCoordinator, bIsSimulatingPhysics);
}

void APSDActorsCoordinator::OnPSDActorEnteredPhysicsRegion
	(APSDActorBase* EnteredPSDActor, int32 EnteredPhysicsRegionId)
{
	RPES_LOG_WARNING(TEXT("PSDActor \"%s\" has entered region with id: %d."
		"Entried pos: %s. PSDActor owner region id: %d"), 
		*EnteredPSDActor->GetName(), EnteredPhysicsRegionId,
		*EnteredPSDActor->GetActorLocation().ToString(),
		EnteredPSDActor->GetActorOwnerPhysicsServiceRegionId());

	// Get the PSDActor owner physics service region id
	const int32 EnteredPSDActorCurrentOwnerPhysicsServiceRegionId =
		EnteredPSDActor->GetActorOwnerPhysicsServiceRegionId();

	// If the owner physics service id is the same as the one he has entered,
	// just ignore, we want only PSDActors that entered a outer region.
	if (EnteredPSDActorCurrentOwnerPhysicsServiceRegionId ==
		EnteredPhysicsRegionId)
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

		// Create the data for the current owner physics service region
		PrimaryFootprint.PhysicsServiceRegionId =
			EnteredPSDActorCurrentOwnerPhysicsServiceRegionId;
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

	int32 i = 0;
	for (const auto Footprint : SharedRegionsPSDActors[EnteredPSDActor])
	{
		FString type = FString();
		switch (Footprint.BodyTypeOnPhysicsServiceRegion)
		{
			case EPSDActorBodyTypeOnPhysicsServiceRegion::Clone:
				type = "clone";
				break;
			case EPSDActorBodyTypeOnPhysicsServiceRegion::Primary:
				type = "primary";
				break;
		}

		RPES_LOG_WARNING(TEXT("(%d) Id: %d; Type: %s "), i++, 
			Footprint.PhysicsServiceRegionId, *type);
	}

	RPES_LOG_INFO(TEXT("PSDActorsCoordinator has registered PSDActor on the"
		" shared regions actors."));
}

void APSDActorsCoordinator::OnPSDActorExitPhysicsRegion
	(APSDActorBase* ExitedPSDActor, int32 ExitedPhysicsRegionId)
{
	RPES_LOG_WARNING(TEXT("PSDActor \"%s\" has exited region with id: %d. "
		"Exited pos: %s. PSDActor owner region id: %d"), 
		*ExitedPSDActor->GetName(), ExitedPhysicsRegionId,
		*ExitedPSDActor->GetActorLocation().ToString(), 
		ExitedPSDActor->GetActorOwnerPhysicsServiceRegionId());

	// Get the PSDActor owner physics service region id
	const int32 ExitedPSDActorCurrentOwnerPhysicsServiceRegionId =
		ExitedPSDActor->GetActorOwnerPhysicsServiceRegionId();

	// If the owner physics service id is the same as the one he has exited,
	// it means he is exiting his owning physics region:
	if (ExitedPSDActorCurrentOwnerPhysicsServiceRegionId ==
		ExitedPhysicsRegionId)
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

			RPES_LOG_INFO(TEXT("Destroyed PSDActor as he is no longer "
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

		// Request the removal of the PSDActor from the physics service he is
		// exiting
		PhysicsServiceRegionList[ExitedPhysicsRegionId]->
			RemovePSDActorFromPhysicsService(ExitedPSDActor);

		// Remove the ownership from the exited physics region (which we know
		// here that it owns this PSDActor)
		PhysicsServiceRegionList[ExitedPhysicsRegionId]->
			RemovePSDActorOwnershipFromRegion(ExitedPSDActor);

		// Transfer the ownership to the first clone physics service region on
		// the footprint info (which will now be the primary region for this 
		// PSDActor)
		PhysicsServiceRegionList[FirstClonePhysicsServiceRegionId]->
			SetPSDActorOwnershipToRegion(ExitedPSDActor);

		// Update the PSDActor body type from "clone" to "primary" on the 
		// physics service
		PhysicsServiceRegionList[FirstClonePhysicsServiceRegionId]->
			UpdatePSDActorBodyType(ExitedPSDActor, "primary");

		RPES_LOG_INFO(TEXT("Transfered PSDActor \"%s\" ownership from "
			"region with id: %d to region with id: %d"),
			*ExitedPSDActor->GetName(), ExitedPhysicsRegionId,
			FirstClonePhysicsServiceRegionId);

		// If the TArray of shared footprint only has two info, it means now
		// that he will only have one, and therefore, can be excluded from the
		// coordinator shared regions PSDActors map (The PSDActor is no longer
		// on a shared region)
		if (ExitedPSDActorFootprintInfo->Num() == 2)
		{
			// Remove the PSDActor from the shared region PSDActors map
			SharedRegionsPSDActors.Remove(ExitedPSDActor);

			// Update the PSDActor status as we know he is only on a single
			// region
			ExitedPSDActor->UpdatePSDActorStatusOnRegion
				(EPSDActorPhysicsRegionStatus::InsideRegion);
		}
		else
		{
			// If not, we will update the PSDActor's footprint (excluding the
			// owning region - The PSDActor is still on a shared region with
			// a third region)

			// Remove the first region from the footprint info
			// (which should be the primary - now exited)
			ExitedPSDActorFootprintInfo->RemoveAt(0);

			// Get the second region, which should be promoted to primary now
			// as it will be owning the PSDActor (here we take the 0 as the
			// primary was already removed)
			auto& ClonePhysicsServiceRegion =
				(*ExitedPSDActorFootprintInfo)[0];

			// Update the clone physics region to primary as now he will be 
			// the new owner of the PSDActor
			ClonePhysicsServiceRegion.BodyTypeOnPhysicsServiceRegion =
				EPSDActorBodyTypeOnPhysicsServiceRegion::Primary;
		}

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

	RPES_LOG_INFO(TEXT("Removed PSDActor clone from phyhsics service "
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

			RPES_LOG_INFO(TEXT("Removed PSDActor footprint (from region "
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

void APSDActorsCoordinator::UpdatePSDActors()
{
	// Check if we are simulating
	if (!bIsSimulatingPhysics)
	{
		return;
	}

	RPES_LOG_WARNING(TEXT("Stepping: %d"), StepPhysicsCounter++);

	// Get pre step physics time (time spent updating physics on services)
	std::chrono::steady_clock::time_point preStepPhysicsTime =
		std::chrono::steady_clock::now();

	// For each socket client thread info, set the message to "step" and send
	// for each physics service region (we know that each thread represents
	// a given physics region)
	for (auto& SocketClientThreadInfo : SocketClientThreadsInfoList)
	{
		// Get the thread info and the thread worker
		auto& ThreadInfoPair = SocketClientThreadInfo.Value;
		auto& ThreadWoker = ThreadInfoPair.Key;

		// Set the message to send on the worker
		ThreadWoker->SetMessageToSend("Step\nMessageEnd\n");
	}

	while (true)
	{
		int32 ReadySocketsToConsumeResponse = 0;

		// Check for responses on each socket
		for (auto& SocketClientThreadInfo : SocketClientThreadsInfoList)
		{
			// Get the thread info
			auto& ThreadInfoPair = SocketClientThreadInfo.Value;

			// Get the worker and the corresponding thread
			auto& ThreadWorker = ThreadInfoPair.Key;
			auto& Thread = ThreadInfoPair.Value;

			if (ThreadWorker->HasResponseToConsume())
			{
				ReadySocketsToConsumeResponse++;
			}
		}

		if (ReadySocketsToConsumeResponse == SocketClientThreadsInfoList.Num())
		{
			break;
		}

		// Sleep for a short duration before the next check
		//FPlatformProcess::Sleep(0.1);
		//std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	
	// Get post physics update time
	std::chrono::steady_clock::time_point postStepPhysicsTime =
		std::chrono::steady_clock::now();

	// Calculate the microsseconds all step physics simulation
	// (considering communication )took
	std::stringstream ss;
	ss << std::chrono::duration_cast<std::chrono::microseconds>
		(postStepPhysicsTime - preStepPhysicsTime).count();
	const std::string elapsedTime = ss.str();

	// Get the step phyiscs time (time spent updating physics on 
	// services) in FString
	const FString ElapsedPhysicsTimeMicroseconds =
		UTF8_TO_TCHAR(elapsedTime.c_str());

	//RPES_LOG_WARNING(TEXT("STP Duration: %s"),
		//*ElapsedPhysicsTimeMicroseconds);

	// Append the step physics time to the current step measurement
	StepPhysicsTimeWithCommsOverheadTimeMeasure +=
		ElapsedPhysicsTimeMicroseconds + "\n";

	// For each socket client thread info, get the physics service response so 
	// we can delegate the update to the corresponding physics service
	for (auto& SocketClientThreadInfo : SocketClientThreadsInfoList)
	{
		// Get the thread info
		auto& ThreadInfoPair = SocketClientThreadInfo.Value;

		// Get the worker and the corresponding thread
		auto& ThreadWorker = ThreadInfoPair.Key;
		auto& Thread = ThreadInfoPair.Value;

		// Once completed, get the response on the worker
		FString Result = ThreadWorker->ConsumeResponse();

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

void APSDActorsCoordinator::StartPSDActorsSimulation
	(const TArray<FString>& SocketServerIpAddrList)
{
	RPES_LOG_INFO(TEXT("Starting PSD actors simulation."));

	if (SocketServerIpAddrList.Num() != PhysicsServiceRegionList.Num())
	{
		RPES_LOG_ERROR(TEXT("Could not start PSDActors simulation as the "
			"number of servers to connect to don't match the number of "
			"physics services regions."));
		return;
	}

	// Foreach physics service region, create a thread for its communication
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		// Get the physics region physics service id
		const int32 RegionPhysicsServiceId =
			PhysicsServiceRegion->RegionOwnerPhysicsServiceId;

		// Create a new worker for this given region 
		FSocketClientThreadWorker* NewSocketClientWorker = new
			FSocketClientThreadWorker(RegionPhysicsServiceId);

		// Create the new thread for the worker
		FRunnableThread* NewSocketClientThread = FRunnableThread::Create
		(NewSocketClientWorker,
			*FString::Printf(TEXT("SocketClientWorkerThread_%d"),
				RegionPhysicsServiceId));

		// Start the thread work
		NewSocketClientWorker->StartThread();

		// Create a new thread pair for the given work and thread
		auto NewSocketClientThreadInfoPair =
			TPair<FSocketClientThreadWorker*, FRunnableThread*>
			(NewSocketClientWorker, NewSocketClientThread);

		// Add the threading info the the list
		SocketClientThreadsInfoList.Add(RegionPhysicsServiceId,
			NewSocketClientThreadInfoPair);
	}
	
	// Reset delta measurements
	DeltaTimeMeasurement = FString();
	StepPhysicsTimeWithCommsOverheadTimeMeasure = FString();
	UsedRamMeasurement = FString();
	AllocatedRamMeasurement = FString();
	CPUUsageMeasurement = FString();

	// Aux to attribute physicsd service regions ip addr
	int32 CurrentPhysicsInitializedPhysicsRegion = 0;

	// For each physics service region on the world, initialize it
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		const FString& PhysicsServiceRegionIpAddr =
			SocketServerIpAddrList[CurrentPhysicsInitializedPhysicsRegion++];

		// Initialize the region with the given ip addr
		PhysicsServiceRegion->InitializePhysicsServiceRegion
			(PhysicsServiceRegionIpAddr);

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

void APSDActorsCoordinator::StartPSDActorsSimulationTest
	(const TArray<FString>& SocketServerIpAddrList, 
	float TestDurationInSeconds/*=30.f*/)
{
	bHasMeasuredCpuAndRamForSimulation = false;

	// Start the PSD actors simulation
	StartPSDActorsSimulation(SocketServerIpAddrList);

	// Start timer to stop the simulation after 30 seconds passed
	GetWorld()->GetTimerManager().SetTimer(PSDActorsTestTimerHandle, this,
		&APSDActorsCoordinator::StopPSDActorsSimulation, TestDurationInSeconds, 
		false);
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

	// For each physics service region on the world, save the measurements
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		PhysicsServiceRegion->SavePhysicsServiceMeasuresements();
	}

	// For each physics service region on the world, clear it
	for (const auto& PhysicsServiceRegion : PhysicsServiceRegionList)
	{
		PhysicsServiceRegion->ClearPhysicsServiceRegion();
	}

	if (HasAuthority())
	{
		// Save measurements
		SaveDeltaTimeMeasurementToFile();
		SaveStepPhysicsTimeMeasureToFile();
		SaveUsedRamMeasurements();
		SaveAllocatedRamMeasurements();
		SaveCpuMeasurements();
	}

	// For each socket client thread info, stop the thread
	for (auto& SocketClientThreadInfo : SocketClientThreadsInfoList)
	{
		// Get the thread info
		auto& ThreadInfoPair = SocketClientThreadInfo.Value;

		// Get the worker and the corresponding thread
		auto& ThreadWorker = ThreadInfoPair.Key;
		auto& Thread = ThreadInfoPair.Value;

		// Stop worker
		ThreadWorker->Stop();

		// Delete the thread
		delete Thread;
	}

	// Clear the socket threads
	SocketClientThreadsInfoList.Empty();

	RPES_LOG_INFO(TEXT("PSD actors simulation has been stopped."));
}

void APSDActorsCoordinator::SaveDeltaTimeMeasurementToFile_Implementation() 
	const
{
	FString TargetFolder = TEXT("FPSMeasure");
	FString FullFolderPath = 
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}
	
	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/%s_Remote_%d.txt"), *MapName, 
		FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/%s_Remote_%d.txt"), *MapName, 
			FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	RPES_LOG_WARNING(TEXT("Saving delta time measurement into \"%s\""), 
		*FileFullPath);

	FFileHelper::SaveStringToFile(DeltaTimeMeasurement, *FileFullPath);
}

void APSDActorsCoordinator::SaveStepPhysicsTimeMeasureToFile_Implementation()
	const
{
	FString TargetFolder = TEXT("StepPhysicsMeasureWithCommsOverhead");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/StepPhysicsTime_%s_%d.txt"),
		*MapName, FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/StepPhysicsTime_%s_%d.txt"), 
			*MapName, FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	RPES_LOG_WARNING(TEXT("Saving step physics time measurement into \"%s\""),
		*FileFullPath);

	FFileHelper::SaveStringToFile(StepPhysicsTimeWithCommsOverheadTimeMeasure,
		*FileFullPath);
}

void APSDActorsCoordinator::SaveUsedRamMeasurements_Implementation() const
{
	FString TargetFolder = TEXT("UsedRamMeasurements");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/UsedRam_%s_%d.txt"),
		*MapName, FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/UsedRam_%s_%d.txt"),
			*MapName, FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	RPES_LOG_WARNING(TEXT("Saving used ram measurement into \"%s\""),
		*FileFullPath);

	FFileHelper::SaveStringToFile(UsedRamMeasurement, *FileFullPath);
}

void APSDActorsCoordinator::SaveAllocatedRamMeasurements_Implementation() const
{
	FString TargetFolder = TEXT("AllocatedRamMeasurements");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/AllocatedRam_%s_%d.txt"),
		*MapName, FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/AllocatedRam_%s_%d.txt"),
			*MapName, FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	RPES_LOG_WARNING(TEXT("Saving allocated ram measurement into \"%s\""),
		*FileFullPath);

	FFileHelper::SaveStringToFile(AllocatedRamMeasurement, *FileFullPath);
}

void APSDActorsCoordinator::SaveCpuMeasurements_Implementation() const
{
	FString TargetFolder = TEXT("CpuPercentageMeasurements");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/CpuPercentage_%s_%d.txt"),
		*MapName, FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/CpuPercentage_%s_%d.txt"),
			*MapName, FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	RPES_LOG_WARNING(TEXT("Saving cpu usage measurement into \"%s\""),
		*FileFullPath);

	FFileHelper::SaveStringToFile(CPUUsageMeasurement, *FileFullPath);
}

void APSDActorsCoordinator::GetRamMeasurement_Implementation()
{
	// Get the memory stats
	FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();

	// Get and store the used memory
	uint32 UsedMemoryMB = MemoryStats.UsedPhysical / (1024 * 1024);
	UsedRamMeasurement += FString::Printf(TEXT("%d\n"),
		UsedMemoryMB);

	// Get and store the allocated memory
	uint32 AllocatedMemoryMB =
		MemoryStats.AvailablePhysical / (1024 * 1024);
	AllocatedRamMeasurement += FString::Printf(TEXT("%d\n"), 
		AllocatedMemoryMB);
}

void APSDActorsCoordinator::GetCPUMeasurement_Implementation()
{
    double cpuPercentage = 0.0;

	// Hide the console window
	AllocConsole();
	ShowWindow(GetConsoleWindow(), SW_HIDE);

    // Execute the system command and read the output
    FILE* pipe = _popen(GetCpuUsageCommand, "r");
    if (!pipe) {
        return;
    }

    char buffer[128];
    std::string result = "";

    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL) {
            result += buffer;
        }
    }

    _pclose(pipe);

    // Parse the output to get the CPU percentage
    cpuPercentage = std::stod(result);

	// Append the cpu percentage to string
	CPUUsageMeasurement += FString::Printf(TEXT("%f\n"), cpuPercentage);

    return;
}
