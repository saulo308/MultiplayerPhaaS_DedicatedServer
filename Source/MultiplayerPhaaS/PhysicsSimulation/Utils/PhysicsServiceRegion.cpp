// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsServiceRegion.h"
#include "Components/BoxComponent.h"

#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorBase.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Components/PSDactorSpawnerComponent.h"
#include "MultiplayerPhaaS/ExternalCommunication/Sockets/SocketClientProxy.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"

#include <string>

APhysicsServiceRegion::APhysicsServiceRegion()
{
	PrimaryActorTick.bCanEverTick = true;

	// Creating the root component
	RegionRootComponent = CreateDefaultSubobject<USceneComponent>
		(TEXT("RegionRootComponent"));

	// Set it as root component
	RootComponent = RegionRootComponent;

	// Creating the physics service's region box component
	PhysicsServiceRegionBoxComponent = CreateDefaultSubobject<UBoxComponent>
		(TEXT("PhysicsServiceRegionBoxComponent"));

	// Attach the box component to the root
	PhysicsServiceRegionBoxComponent->SetupAttachment(RootComponent);

	// Set a default box extent so it can be clear to user that this represents
	// the physics service "working" region
	PhysicsServiceRegionBoxComponent->SetBoxExtent
		(FVector(500.f, 500.f, 200.f));

	// Creating the PSDActorsSpawnerComponent
	PSDActorSpawner = CreateDefaultSubobject<UPSDActorSpawnerComponent>
		(TEXT("PSDActorSpawner"));

	// Bind box component events to the callbacks
	PhysicsServiceRegionBoxComponent->OnComponentBeginOverlap.AddDynamic
		(this, &APhysicsServiceRegion::OnRegionEntry);
	PhysicsServiceRegionBoxComponent->OnComponentEndOverlap.AddDynamic
		(this, &APhysicsServiceRegion::OnRegionExited);
}

void APhysicsServiceRegion::BeginPlay()
{
	Super::BeginPlay();
}

void APhysicsServiceRegion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APhysicsServiceRegion::InitializePhysicsServiceRegion()
{
	MPHAAS_LOG_INFO
		(TEXT("Starting PSD actors simulation on region with ID: %d."),
		RegionOwnerPhysicsServiceId);

	// Connect this region to his physics service (given IP addr and ID)
	const bool bWasConnectionSuccesful = ConnectToPhysicsService();
	if (!bWasConnectionSuccesful)
	{
		MPHAAS_LOG_ERROR
		(TEXT("Physics service region with ID %d could not connect to the physics service server."),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Prepare the physics service region for simulation. This will get all the
	// PSDActors on this region and fill the TMap with them, attributing a 
	// unique ID for each of them
	PreparePhysicsServiceRegionForSimulation();

	// Initialize physics world on the physics service
	InitializeRegionPhysicsWorld();

	MPHAAS_LOG_INFO(TEXT("Physics service region with ID %d is ready."),
		RegionOwnerPhysicsServiceId);
}

void APhysicsServiceRegion::PreparePhysicsServiceRegionForSimulation()
{
	// Get all PSDActors on this region
	const auto PSDActorsOnRegion = GetAllPSDActorsOnRegion();

	// Foreach found actor, append to PSDActorsToSimulateMap
	for (int i = 0; i < PSDActorsOnRegion.Num(); i++)
	{
		// Cast to PSDActor to get a reference to it
		auto PSDActor = Cast<APSDActorBase>(PSDActorsOnRegion[i]);
		if (!PSDActor)
		{
			continue;
		}

		// Add to map that stores all the PSD actors to simulate.
		// The key is the body id on the physics system. Starts at 1 as the
		// flor on physics system has body id of 0
		// The value is the reference to the actor
		const uint32 NewPSDActorId = i + 1;
		PSDActorsToSimulateMap.Add(NewPSDActorId, PSDActor);
	}
}

void APhysicsServiceRegion::InitializeRegionPhysicsWorld()
{
	MPHAAS_LOG_INFO
	(TEXT("Initializing physics world on physics service with ID: %d."),
		RegionOwnerPhysicsServiceId);

	// Create the initialization message string and initialize it with "Init"
	// so physics service knows what this message is
	FString InitializationMessage = "Init\n";

	// Foreach PSD actor, get its initialization info and append to the 
	// initialization message
	for (auto& PSDActor : PSDActorsToSimulateMap)
	{
		// Get the current actor's location as a string
		const auto CurrentActorLocation =
			PSDActor.Value->GetCurrentActorLocationAsString();

		// Get the current actor's owning server id
		const auto CurrentActorOwnerServerId =
			PSDActor.Value->GetActorOwnerPhysicsServiceId();

		// Check if the PSDActor owning server id is the same as this region's
		// physics service id. If not, something went wrong
		if (CurrentActorOwnerServerId != RegionOwnerPhysicsServiceId)
		{
			MPHAAS_LOG_ERROR
			(TEXT("PSDActor owning server id (%d) is not the same as the region he is in (region ID: %d)."),
				CurrentActorOwnerServerId, RegionOwnerPhysicsServiceId);
		}

		// On the initialization message, append this actor's key as its ID on 
		// the first param (delimited by ";") and its given location afterwards
		InitializationMessage += FString::Printf(TEXT("%d;%s\n"),
			PSDActor.Key, *CurrentActorLocation);
	}

	// Append the end message token. This is 
	// needed as this may reach the server in separate messages, since it 
	// can be too big to send everything at once. The server will 
	// acknowledge the initialization message has ended with this token
	InitializationMessage += "EndMessage\n";

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*InitializationMessage));

	// Convert message to char*. This is needed as some UE converting has 
	// the limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	MPHAAS_LOG_INFO
	(TEXT("Sending init message for service with id \"%d\". Message: %s"),
		RegionOwnerPhysicsServiceId, *InitializationMessage);

	// Send message to initialize physics world on service
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
	(MessageAsChar, RegionOwnerPhysicsServiceId);

	MPHAAS_LOG_INFO(TEXT("Physics service with ID (%d) response: %s"),
		RegionOwnerPhysicsServiceId, *Response);
}

void APhysicsServiceRegion::UpdatePSDActorsOnRegion()
{
	MPHAAS_LOG_INFO(TEXT("Updating PSD actors on region with ID: %d."),
		RegionOwnerPhysicsServiceId);

	// Check if we have a valid connection with this region's physics service
	// given its ID
	if (!FSocketClientProxy::IsConnectionValid(RegionOwnerPhysicsServiceId))
	{
		MPHAAS_LOG_ERROR
			(TEXT("Could not simulate as there's no valid connection for physics service region with ID: %d"),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Construct message. This will be verified so service knows that we are
	// making a "step physics" call
	const char* StepPhysicsMessage = "Step";

	MPHAAS_LOG_INFO
		(TEXT("Sending \"Step\" request to physics service with id: %d."),
		RegionOwnerPhysicsServiceId);

	// Request physics simulation on physics service
	FString PhysicsSimulationResultStr =
		FSocketClientProxy::SendMessageAndGetResponse(StepPhysicsMessage,
		RegionOwnerPhysicsServiceId);

	MPHAAS_LOG_INFO(TEXT("Physics service (id: %d) response: %s"),
		RegionOwnerPhysicsServiceId, *PhysicsSimulationResultStr);

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
		const int32 ActorID =
			FCString::Atoi(*ParsedActorSimulationResult[0]);

		// Check if the actor exists on the map
		const bool bDoesActorExistOnSimulationMap = 
			PSDActorsToSimulateMap.Contains(ActorID);
		if (!bDoesActorExistOnSimulationMap)
		{
			MPHAAS_LOG_ERROR
				(TEXT("Could not find actor with ID (%d) on physics service region (%d)"),
				ActorID, RegionOwnerPhysicsServiceId);
			continue;
		}

		// Find the actor on the map
		auto ActorToUpdate = PSDActorsToSimulateMap[ActorID];

		// Update PSD actor with the result
		const float NewPosX =
			FCString::Atof(*ParsedActorSimulationResult[1]);
		const float NewPosY =
			FCString::Atof(*ParsedActorSimulationResult[2]);
		const float NewPosZ =
			FCString::Atof(*ParsedActorSimulationResult[3]);
		const FVector NewPos(NewPosX, NewPosY, NewPosZ);

		ActorToUpdate->UpdatePositionAfterPhysicsSimulation(NewPos);

		// Update PSD actor rotation with the result
		const float NewRotX =
			FCString::Atof(*ParsedActorSimulationResult[4]);
		const float NewRotY =
			FCString::Atof(*ParsedActorSimulationResult[5]);
		const float NewRotZ =
			FCString::Atof(*ParsedActorSimulationResult[6]);
		const FVector NewRotEuler(NewRotX, NewRotY, NewRotZ);

		ActorToUpdate->UpdateRotationAfterPhysicsSimulation(NewRotEuler);
	}
}

bool APhysicsServiceRegion::ConnectToPhysicsService()
{
	MPHAAS_LOG_INFO(TEXT("Parsing server IP address: \"%s\""), 
		*PhysicsServiceIpAddr);

	// Parse the server ip addr
	TArray<FString> ParsedServerIpAddr;
	PhysicsServiceIpAddr.ParseIntoArray(ParsedServerIpAddr, TEXT(":"));

	if (ParsedServerIpAddr.Num() < 2)
	{
		MPHAAS_LOG_ERROR
			(TEXT("Could not parse server ip addr: \"%s\". Check parsing."),
			*PhysicsServiceIpAddr);
		return false;
	}

	// Get the server ip addr and port
	const FString ServerIpAddr = ParsedServerIpAddr[0];
	const FString ServerPort = ParsedServerIpAddr[1];

	MPHAAS_LOG_INFO(TEXT("Connecting to physics service: \"%s:%s\""),
		*ServerIpAddr, *ServerPort);

	// Open Socket connection with the physics service server, given its
	// IpAddr and port. The server id will also be set and incremented 
	// here so each server has its own id
	const bool bWasOpenSocketSuccess =
		FSocketClientProxy::OpenSocketConnectionToServer
		(ServerIpAddr, ServerPort, RegionOwnerPhysicsServiceId);

	// Check for errors on opening. If could not open socket connection, return
	if (!bWasOpenSocketSuccess)
	{
		MPHAAS_LOG_ERROR(TEXT("Socket openning error. Check logs."));
		return false;
	}

	return true;
}

void APhysicsServiceRegion::SpawnNewPSDSphere(const FVector NewSphereLocation)
{
	MPHAAS_LOG_INFO(TEXT("Spawning new PSD sphere at location %s"),
		*NewSphereLocation.ToString());

	// Check if we have a valid PSDActors spawner. If not, find it
	check(PSDActorSpawner);

	// Spawn the new sphere
	const auto SpawnedSphere =
		PSDActorSpawner->SpawnPSDActor(NewSphereLocation);

	// Get the number of already spawned sphere
	const auto NumberOfSpawnedSpheres = PSDActorsToSimulateMap.Num();

	// The new sphere ID will be the NumberOfSpawnedSpheres + 1
	// TODO +2 here
	const int32 NewSphereID = NumberOfSpawnedSpheres + 2;

	// Add the sphere to the PSDActor map so it's Transform can be updated
	PSDActorsToSimulateMap.Add(NewSphereID, SpawnedSphere);

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
	// TODO this should receive a given physics service id
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar, RegionOwnerPhysicsServiceId);

	MPHAAS_LOG_INFO(TEXT("Add new sphere action response: %s"), *Response);
}

void APhysicsServiceRegion::RemovePSDActorFromPhysicsService
	(APSDActorBase* PSDActorToRemove)
{
	MPHAAS_LOG_INFO(TEXT("Removing PSDActor \"%s\""),
		*PSDActorToRemove->GetName());

	// Check if the PSDActor to remove is valid
	if (!PSDActorToRemove)
	{
		MPHAAS_LOG_ERROR
			(TEXT("Could not remove PSDActor as reference is invalid."));
		return;
	}

	// Get the actor's key on the simulation map as it works as the BodyID
	// on the physics service
	const auto BodyIdToRemove = 
		PSDActorsToSimulateMap.FindKey(PSDActorToRemove);
	if (!BodyIdToRemove)
	{
		MPHAAS_LOG_ERROR
			(TEXT("Could not find PSDActor BodyId to remove as it is not on simulation map."));
		return;
	}

	// Create the message to send server
	// The template is:
	// "RemoveBody\n
	// Id"
	const FString RemoveBodyMessage =
		FString::Printf(TEXT("RemoveBody\n%d"), *BodyIdToRemove);

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*RemoveBodyMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to initialize physics world on service
	// TODO this should receive a given physics service id
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar, RegionOwnerPhysicsServiceId);

	MPHAAS_LOG_INFO(TEXT("Remove body request response: %s"), *Response);
}

void APhysicsServiceRegion::ClearPhysicsServiceRegion()
{
	MPHAAS_LOG_INFO(TEXT("Clearing physics service region with ID: %d."),
		RegionOwnerPhysicsServiceId);

	// Foreach PSDActor on this region, destroy it
	for (auto& PSDActor : PSDActorsToSimulateMap)
	{
		PSDActor.Value->Destroy();
	}

	// Clear the map
	PSDActorsToSimulateMap.Empty();

	// Close socket connection on this physics service (given its ID)
	const bool bWasCloseSocketSuccess =
		FSocketClientProxy::CloseSocketConnectionsToServerById
		(RegionOwnerPhysicsServiceId);

	// Check for errors on close. 
	// Any error should be shown in log
	if (!bWasCloseSocketSuccess)
	{
		MPHAAS_LOG_ERROR(TEXT("Socket closing error. Check logs."));
		return;
	}

	MPHAAS_LOG_INFO(TEXT("Physics service service (id:%d) socket closed."),
		RegionOwnerPhysicsServiceId);
}

TArray<APSDActorBase*> APhysicsServiceRegion::GetAllPSDActorsOnRegion()
{
	// Create the list of PSDActors to return
	TArray<APSDActorBase*> PSDActorsOnRegion = TArray<APSDActorBase*>();

	// Get all overlapping PSDActors on this region
	TArray<AActor*> FoundActors;
	PhysicsServiceRegionBoxComponent->GetOverlappingActors(FoundActors,
		APSDActorBase::StaticClass());

	// Cast all to "APSDActorBase"
	for (auto& FoundPSDActor : FoundActors)
	{
		// Get the found actor as PSDActorBase
		auto FoundActorAsPSDActorBase = Cast<APSDActorBase>(FoundPSDActor);

		// Add it to the list of actors on this region
		PSDActorsOnRegion.Add(FoundActorAsPSDActorBase);

		// Set the PSDActor owner physics service id
		FoundActorAsPSDActorBase->SetActorOwnerPhysicsServiceId
			(RegionOwnerPhysicsServiceId);
	}

	return PSDActorsOnRegion;
}

void APhysicsServiceRegion::OnRegionEntry
	(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp,	int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	// Get the other actor as PSDActor
	auto OtherActorAsPSDActor = Cast<APSDActorBase>(OtherActor);

	// If not valid, just ignore. We only want PSDActors
	if (!OtherActorAsPSDActor)
	{
		return;
	}

	// Get the OtherActor physics service ID
	const auto OtherActorPhysicsServiceId = 
		OtherActorAsPSDActor->GetActorOwnerPhysicsServiceId();
	
	// If this actor already belongs to this region, ignore it. We want only
	// to process PSDActors from other regions
	if (OtherActorPhysicsServiceId == RegionOwnerPhysicsServiceId)
	{
		return;
	}

	MPHAAS_LOG_INFO
		(TEXT("Actor \"%s\" entried region with physics service owning id: %d."),
		*OtherActorAsPSDActor->GetName(), OtherActorPhysicsServiceId);

	// Add the PSDActor to the pending migration list
	PendingMigrationPSDActors.Add(OtherActorAsPSDActor);

	// Bind the actor's exited event on the callback
	OtherActorAsPSDActor->OnActorExitedCurrentPhysicsRegion.AddDynamic(this,
		&APhysicsServiceRegion::OnActorFullyExitedOwnPhysicsRegion);

	// Call the method to when he eneters a new physics service region
	OtherActorAsPSDActor->OnEnteredNewPhysicsRegion();
}

void APhysicsServiceRegion::OnRegionExited
	(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Get the other actor as PSDActor
	auto OtherActorAsPSDActor = Cast<APSDActorBase>(OtherActor);

	// If not valid, just ignore. We only want PSDActors
	if (!OtherActorAsPSDActor)
	{
		return;
	}

	MPHAAS_LOG_INFO
		(TEXT("Actor \"%s\" exited region with physics service owning id: %d."),
		*OtherActorAsPSDActor->GetName(), RegionOwnerPhysicsServiceId);

	// Remove this PSDActor from the phyiscs service as it is no longer
	// responsible for simulating it
	RemovePSDActorFromPhysicsService(OtherActorAsPSDActor);

	// Get the actor's key on the simualtion map
	const auto ActorKey = PSDActorsToSimulateMap.FindKey(OtherActorAsPSDActor);
	if (ActorKey)
	{
		// Remove this PSDActor from the simulation map
		PSDActorsToSimulateMap.Remove(*ActorKey);
	}

	// Call the method on the PSDActorBase so he knows he has exited this
	// physics service region
	OtherActorAsPSDActor->OnExitedPhysicsRegion();
}

void APhysicsServiceRegion::OnActorFullyExitedOwnPhysicsRegion(APSDActorBase*
	ExitedActor)
{
	// Remove the callback
	ExitedActor->OnActorExitedCurrentPhysicsRegion.RemoveDynamic(this,
		&APhysicsServiceRegion::OnActorFullyExitedOwnPhysicsRegion);

	// Check if this exited actor is on the pending migration list
	if (!PendingMigrationPSDActors.Contains(ExitedActor))
	{
		MPHAAS_LOG_WARNING
		(TEXT("Actor was pending migration but is not on migration list: %s"),
			*ExitedActor->GetName());
		return;
	}

	// Spawn a new PSD sphere on this region where the other actor is
	SpawnNewPSDSphere(ExitedActor->GetActorLocation());

	// Remove the actor from the pending migration list
	PendingMigrationPSDActors.Remove(ExitedActor);

	// Destroy the OtherActor, as now he will be simulated inside this region
	ExitedActor->Destroy();
}
