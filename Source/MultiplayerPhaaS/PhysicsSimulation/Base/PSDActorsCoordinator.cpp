// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#include "PSDActorsCoordinator.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Utils/PSDActorsSpawner.h"
#include "MultiplayerPhaaS/ExternalCommunication/Sockets/SocketClientProxy.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"
#include "Kismet/GameplayStatics.h"
#include "PSDActorBase.h"
#include "Net/UnrealNetwork.h"

#include <string>
#include <iostream>
#include <sstream>
#include <chrono>

APSDActorsCoordinator::APSDActorsCoordinator()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APSDActorsCoordinator::BeginPlay()
{
	Super::BeginPlay();
}

void APSDActorsCoordinator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPSDActorsSimulation();
}

void APSDActorsCoordinator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSimulating && HasAuthority())
	{
		// Update PSD actors by simulating physics on the service
		// and parsing it's results with the new actor position
		UpdatePSDActors();
	}
}

void APSDActorsCoordinator::StartPSDActorsSimulation
	(const FString& SocketServerIpAddr)
{
	MPHAAS_LOG_INFO(TEXT("Starting PSD actors simulation."));
	
	// Get all PSDActors
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorBase::StaticClass(), FoundActors);

	// Foreach found actor, append to PSDActorsList
	for (int i = 0; i < FoundActors.Num(); i++)
	{
		// Cast to PSDActor to get a reference to it
		auto PSDActor = Cast<APSDActorBase>(FoundActors[i]);
		if (!PSDActor)
		{
			continue;
		}

		// Add to map that stores all the PSD actors to simulate.
		// The key is the body id on the physics system. Starts at 1 as the
		// flor on physics system has body id of 0
		// The value is the reference to the actor
		PSDActorMap.Add(i + 1, PSDActor);
	}
	
	// Open Socket connection with the physics service server, given its IpAddr
	const bool bWasOpenSocketSuccess =
		FSocketClientProxy::OpenSocketConnectionToLocalhostServer
		(SocketServerIpAddr);

	// Check for errors on opening. 
	// Any error should be shown in log
	if (!bWasOpenSocketSuccess)
	{
		MPHAAS_LOG_ERROR(TEXT("Socket openning error. Check logs."));
		return;
	}

	MPHAAS_LOG_INFO(TEXT("Physics service server socket opened."));

	// Initialize physics world on the physics service
	InitializePhysicsWorld();
	
	// Set the flag to start simulating on each tick
	bIsSimulating = true;

	MPHAAS_LOG_INFO(TEXT("PSD actors started simulating."));
}

void APSDActorsCoordinator::StopPSDActorsSimulation()
{
	if (!bIsSimulating)
	{
		return;
	}

	MPHAAS_LOG_INFO(TEXT("Stopping PSD actors simulation."));

	bIsSimulating = false;
	
	// Close Socket connection with localhost server
	const bool bWaCloseSocketSuccess =
		FSocketClientProxy::CloseSocketConnection();

	// Check for errors on opening. 
	// Any error should be shown in log
	if (!bWaCloseSocketSuccess)
	{
		MPHAAS_LOG_ERROR(TEXT("Socket closing error. Check logs."));
		return;
	}

	MPHAAS_LOG_INFO(TEXT("Physics service service socket closed."));
	MPHAAS_LOG_INFO(TEXT("PSD actors stopped simulating."));
}

void APSDActorsCoordinator::StartPSDActorsSimulationTest
	(const FString& SocketServerIpAddr, float TestDurationInSeconds/*=30.f*/)
{
	// Start the PSD actors simulation
	StartPSDActorsSimulation(SocketServerIpAddr);

	// Start timer to stop the simulation after 30 seconds passed
	GetWorld()->GetTimerManager().SetTimer(PSDActorsTestTimerHandle, this,
		&APSDActorsCoordinator::StopPSDActorsSimulation, TestDurationInSeconds, 
		false);
}

void APSDActorsCoordinator::InitializePhysicsWorld()
{
	// Start initialization message
	FString InitializationMessage = "Init\n";

	// Foreach PSD actor, get its StepPhysicsString
	for (auto& PSDActor : PSDActorMap)
	{
		// Get the current actor's location as a string
		const auto CurrentActorLocation = 
			PSDActor.Value->GetCurrentActorLocationAsString();

		// Append the current actor location to the initialization message
		InitializationMessage += FString::Printf(TEXT("%d;%s\n"),
			PSDActor.Key, *CurrentActorLocation);
	}

	// Append the end message token. This is needed as this may reach the
	// server in separate messages, since it can be too big to send everything
	// at once. The server will acknowledge the initialization message has 
	// ended with this token
	InitializationMessage += "EndMessage\n";

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*InitializationMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to initialize physics world on service
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar);
}

void APSDActorsCoordinator::UpdatePSDActors()
{
	// Check if we are simulating
	if (!bIsSimulating)
	{
		return;
	}

	MPHAAS_LOG_INFO(TEXT("Updating PSD actors for this frame."));
	
	// Check if we have a valid connection
	if (!FSocketClientProxy::HasValidConnection())
	{
		MPHAAS_LOG_ERROR
			(TEXT("Could not simulate as there's no valid connection"));
		return;
	}

	// Get pre step physics time
	std::chrono::steady_clock::time_point preStepPhysicsTime =
		std::chrono::steady_clock::now();

	// Construct message. This will be verified so service knows that we are
	// making a "step physics" call
	const char* StepPhysicsMessage = "Step";

	MPHAAS_LOG_INFO(TEXT("Seinds request to physics service."));

	// Request physics simulation on physics service bt passing the 
	// world delta time
	FString PhysicsSimulationResultStr =
		FSocketClientProxy::SendMessageAndGetResponse(StepPhysicsMessage);

	// Get post physics communication time
	std::chrono::steady_clock::time_point postStepPhysicsTime =
		std::chrono::steady_clock::now();

	// Calculate the microsseconds all step physics simulation
	// (considering communication )took
	std::stringstream ss;
	ss << std::chrono::duration_cast<std::chrono::microseconds>
		(postStepPhysicsTime - preStepPhysicsTime).count();
	const std::string elapsedTime = ss.str();

	// Get the delta time in FString
	const FString ElapsedPhysicsTimeMicroseconds =
		UTF8_TO_TCHAR(elapsedTime.c_str());

	MPHAAS_LOG_INFO(TEXT("Physics service response: %s"), 
		*PhysicsSimulationResultStr);
	MPHAAS_LOG_INFO(TEXT("=============="));

	// Parse physics simulation result
	// Each line will contain a result for a actor in terms of:
	// "Id; posX; posY; posZ; rotX; rotY; rotZ"
	TArray<FString> ParsedSimulationResult;
	PhysicsSimulationResultStr.ParseIntoArrayLines(ParsedSimulationResult);

	// Foreach line, parse its results (getting each actor pos)
	for (auto& SimulationResultLine : ParsedSimulationResult)
	{
		// Check if the line is just "MessageEnd" message
		if (SimulationResultLine.Contains("MessageEnd"))
		{
			continue;
		}

		// Parse the line with ";" delimit
		TArray<FString> ParsedActorSimulationResult;
		SimulationResultLine.ParseIntoArray(ParsedActorSimulationResult,
			TEXT(";"), false);

		// Check for errors
		if (ParsedActorSimulationResult.Num() < 7)
		{
			MPHAAS_LOG_ERROR
				(TEXT("Could not parse line \"%s\". Number of arguments is: %d"),
				*SimulationResultLine, ParsedActorSimulationResult.Num());
			return;
		}

		// Get the actor id to float
		const uint32 ActorID = FCString::Atoi(*ParsedActorSimulationResult[0]);

		// Find the actor on the map
		APSDActorBase* ActorToUpdate = PSDActorMap[ActorID];
		if (!ActorToUpdate)
		{
			MPHAAS_LOG_ERROR(TEXT("Could not find actor with ID:%f"), ActorID);
			continue;
		}

		// Update PSD actor with the result
		const float NewPosX = FCString::Atof(*ParsedActorSimulationResult[1]);
		const float NewPosY = FCString::Atof(*ParsedActorSimulationResult[2]);
		const float NewPosZ = FCString::Atof(*ParsedActorSimulationResult[3]);
		const FVector NewPos(NewPosX, NewPosY, NewPosZ);

		ActorToUpdate->UpdatePositionAfterPhysicsSimulation(NewPos);

		// Update PSD actor rotation with the result
		const float NewRotX = FCString::Atof(*ParsedActorSimulationResult[4]);
		const float NewRotY = FCString::Atof(*ParsedActorSimulationResult[5]);
		const float NewRotZ = FCString::Atof(*ParsedActorSimulationResult[6]);
		const FVector NewRotEuler(NewRotX, NewRotY, NewRotZ);

		ActorToUpdate->UpdateRotationAfterPhysicsSimulation(NewRotEuler);
	}
}

void APSDActorsCoordinator::SpawnNewPSDSphere(const FVector NewSphereLocation)
{
	// Check if we have a valid PSDActors spawner. If not, find it
	if (!PSDActorsSpanwer)
	{
		// Get the PSDActorsSpawner
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(),
			APSDActorsSpawner::StaticClass(), FoundActors);

		// Check if has found the PSDActorsSpawner
		if (FoundActors.Num() < 1)
		{
			MPHAAS_LOG_ERROR
				(TEXT("No PSDActorsSpawner found on the level to spawn new PSDSphere"));
			return;
		}

		// Set the reference
		PSDActorsSpanwer = Cast<APSDActorsSpawner>(FoundActors[0]);
	}

	// Spawn the new sphere
	const auto SpawnedSphere = 
		PSDActorsSpanwer->SpawnPSDActor(NewSphereLocation);

	// Get the number of already spawned sphere
	const auto NumberOfSpawnedSpheres = PSDActorMap.Num();

	// The new sphere ID will be the NumberOfSpawnedSpheres + 1
	const int32 NewSphereID = NumberOfSpawnedSpheres + 1;

	// Add the sphere to the PSDActor map so it's Transform can be updated
	PSDActorMap.Add(NewSphereID, SpawnedSphere);

	// Create the message to send server
	// The template is:
	// "AddSphereBody\n
	// Id; posX; posY; posZ"
	const FString SpawnNewPSDSphereMessage = 
		FString::Printf(TEXT("AddSphereBody\n%d;%f;%f;%f"), NewSphereID, 
		NewSphereLocation.X, NewSphereLocation.Y, 
		NewSphereLocation.Z);

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*SpawnNewPSDSphereMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to initialize physics world on service
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar);

	MPHAAS_LOG_INFO(TEXT("Add new sphere action response: %s"), *Response);
}
