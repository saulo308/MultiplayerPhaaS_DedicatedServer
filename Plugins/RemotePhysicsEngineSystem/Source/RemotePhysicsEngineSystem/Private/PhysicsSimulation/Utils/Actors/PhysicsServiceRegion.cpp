// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsSimulation/Utils/Actors/PhysicsServiceRegion.h"
#include "PhysicsSimulation/PSDActors/Base/PSDActorBase.h"
#include "PhysicsSimulation/Utils/Components/PSDactorSpawnerComponent.h"
#include "ExternalCommunication/Sockets/SocketClientProxy.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"

#include "Components/BoxComponent.h"

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
	RPES_LOG_INFO(TEXT("Starting PSD actors simulation on region with "
		"ID: %d."), RegionOwnerPhysicsServiceId);

	// Connect this region to his physics service (given IP addr and ID)
	const bool bWasConnectionSuccessful = ConnectToPhysicsService();
	if (!bWasConnectionSuccessful)
	{
		RPES_LOG_ERROR(TEXT("Physics service region with ID %d could not "
			"connect to the physics service server."),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Get all the dynamic PSDActors on this region so they can be updated on
	// each physics step
	GetAllDynamicPSDActorOnRegion();

	// Initialize physics world on the physics service
	InitializeRegionPhysicsWorld();

	// Set the flag to indicate that this physics service region is now active
	bIsPhysicsServiceRegionActive = true;

	RPES_LOG_INFO(TEXT("Physics service region with ID %d is ready."),
		RegionOwnerPhysicsServiceId);
}

void APhysicsServiceRegion::GetAllDynamicPSDActorOnRegion()
{
	// Get all PSDActors on this region
	const auto PSDActorsOnRegion = GetAllPSDActorsOnRegion();

	// Foreach found actor, append to PSDActorsToSimulateMap
	for (int i = 0; i < PSDActorsOnRegion.Num(); i++)
	{
		// Cast to PSDActor to get a reference to it
		const auto PSDActor = Cast<APSDActorBase>(PSDActorsOnRegion[i]);
		if (!PSDActor)
		{
			continue;
		}

		// Check if the PSDActor is static
		if (PSDActor->IsPSDActorStatic())
		{
			// If it is, just ignore it
			continue;
		}

		// If not, it is a dynamic body.
		// Add to map that stores all the dynamic PSD actors so they can be
		// updated on each physics step
		// The key is the PSDActor's body ID on the physics service
		// The value is the reference to the actor
		DynamicPSDActorsOnRegion.Add
			(PSDActor->GetPSDActorBodyIdOnPhysicsService(), PSDActor);
	}
}

void APhysicsServiceRegion::InitializeRegionPhysicsWorld()
{
	RPES_LOG_INFO(TEXT("Initializing physics world on physics service with "
		"ID: %d."), RegionOwnerPhysicsServiceId);

	// Create the initialization message string and initialize it with "Init"
	// so physics service knows what this message is
	FString InitializationMessage = "Init\n";

	// Get all PSDActors on this region
	const auto PSDActorsOnRegion = GetAllPSDActorsOnRegion();

	// Foreach PSD actor on the region, get its initialization info and append 
	// to the initialization message
	for (auto& PSDActor : PSDActorsOnRegion)
	{
		// Get the current actor's owning server id
		const auto CurrentActorOwnerServerId =
			PSDActor->GetActorOwnerPhysicsServiceId();

		// Check if the PSDActor owning server id is the same as this region's
		// physics service id. If not, something went wrong
		if (CurrentActorOwnerServerId != RegionOwnerPhysicsServiceId)
		{
			RPES_LOG_ERROR(TEXT("PSDActor owning server id (%d) is not the "
				"same as the region he is in (region ID: %d)."),
				CurrentActorOwnerServerId, RegionOwnerPhysicsServiceId);
		}

		// Get the PSDActor physics service initialization string
		const FString PSDActorInitializationMessage =
			PSDActor->GetPhysicsServiceInitializationString();

		// If empty, just ignore it. There must be a log with error
		if (PSDActorInitializationMessage.IsEmpty())
		{
			continue;
		}

		// Append it to the final initialization message
		InitializationMessage += PSDActorInitializationMessage;
	}

	// Append the end message token. This is 
	// needed as this may reach the server in separate messages, since it 
	// can be too big to send everything at once. The server will 
	// acknowledge the initialization message has ended with this token
	InitializationMessage += "MessageEnd\n";

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*InitializationMessage));

	// Convert message to char*. This is needed as some UE converting has 
	// the limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	RPES_LOG_INFO(TEXT("Sending init message for service with id \"%d\". "
		"Message: %s"), RegionOwnerPhysicsServiceId, *InitializationMessage);

	// Send message to initialize physics world on service
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar, RegionOwnerPhysicsServiceId);

	//MPHAAS_LOG_INFO(TEXT("Physics service with ID (%d) response: %s"),
		//RegionOwnerPhysicsServiceId, *Response);
}

void APhysicsServiceRegion::UpdatePSDActorsOnRegion
	(const FString& PhysicsSimulationResultStr)
{
	RPES_LOG_INFO(TEXT("Updating PSD actors on region with ID: %d."),
		RegionOwnerPhysicsServiceId);

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
			RPES_LOG_ERROR(TEXT("Could not parse line \"%s\". Number of "
				"arguments is: %d"), *SimulationResultLine,
				ParsedActorSimulationResult.Num());
			return;
		}

		// Get the actor id to float
		const int32 ActorID =
			FCString::Atoi(*ParsedActorSimulationResult[0]);

		// Check if the actor exists on the dynamic PSDActors map
		const bool bDoesActorExistOnMap = 
			DynamicPSDActorsOnRegion.Contains(ActorID);
		if (!bDoesActorExistOnMap)
		{
			//MPHAAS_LOG_ERROR
				//(TEXT("Could not find dynamic actor with ID (%d) on physics service region (id: %d)."),
				//ActorID, RegionOwnerPhysicsServiceId);
			continue;
		}

		// Find the actor on the map
		auto ActorToUpdate = DynamicPSDActorsOnRegion[ActorID];

		// To be sure, check if the actor is valid
		if (!ActorToUpdate)
		{
			RPES_LOG_ERROR(TEXT("Could not update dynamic actor with ID (%d) "
				"on physics service region (id: %d) as he is invalid."),
				ActorID, RegionOwnerPhysicsServiceId);
			continue;
		}

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
	RPES_LOG_INFO(TEXT("Parsing server IP address: \"%s\""), 
		*PhysicsServiceIpAddr);

	// Parse the server ip addr
	TArray<FString> ParsedServerIpAddr;
	PhysicsServiceIpAddr.ParseIntoArray(ParsedServerIpAddr, TEXT(":"));

	if (ParsedServerIpAddr.Num() < 2)
	{
		RPES_LOG_ERROR(TEXT("Could not parse server ip addr: \"%s\". Check "
			"parsing."), *PhysicsServiceIpAddr);
		return false;
	}

	// Get the server ip addr and port
	const FString ServerIpAddr = ParsedServerIpAddr[0];
	const FString ServerPort = ParsedServerIpAddr[1];

	RPES_LOG_INFO(TEXT("Connecting to physics service: \"%s:%s\""),
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
		RPES_LOG_ERROR(TEXT("Socket openning error. Check logs."));
		return false;
	}

	return true;
}

void APhysicsServiceRegion::SpawnNewPSDSphere(const FVector NewSphereLocation)
{
	RPES_LOG_INFO(TEXT("Spawning new PSD sphere at location (%s) on region "
		"with id: %d"), *NewSphereLocation.ToString(), 
		RegionOwnerPhysicsServiceId);

	// Check if we have a valid PSDActors spawner. If not, find it
	check(PSDActorSpawner);

	// Spawn the new sphere on the new sphere location and this region's owner
	// physics service ID (The ID is needed to avoid the "OnRegionEntry" being
	// called on the newly spawned actor).
	const auto SpawnedSphere = PSDActorSpawner->SpawnPSDActor
		(NewSphereLocation, RegionOwnerPhysicsServiceId);

	// Get the new sphere ID as the PSDActor BodyID
	const int32 NewSphereID =
		SpawnedSphere->GetPSDActorBodyIdOnPhysicsService();

	// Add the sphere to the dynamic PSDActors map so it's Transform can be 
	// updated on next step
	DynamicPSDActorsOnRegion.Add(NewSphereID, SpawnedSphere);

	// Create the message to send server
	// The template is:
	// "AddBody\n
	// actorType; Id; bodyType; posX; posY; posZ\n
	// MessageEnd\n"
	const FString SpawnNewPSDSphereMessage =
		FString::Printf(TEXT("AddBody\nspere;%d;primary;%f;%f;%f\nMessageEnd\n"), 
		NewSphereID, NewSphereLocation.X, NewSphereLocation.Y,
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

	RPES_LOG_INFO(TEXT("Add new sphere action response: %s"), *Response);
}

void APhysicsServiceRegion::AddPSDActorCloneOnPhysicsService
	(const APSDActorBase* PSDActorToClone)
{
	RPES_LOG_WARNING(TEXT("Adding PSDActor \"%s\" clone on region (id: %d)"),
		*PSDActorToClone->GetName(), RegionOwnerPhysicsServiceId);

	// Get the PSDActor ID to use it as the body ID on the physics service
	const int32 PSDActorBodyID = 
		PSDActorToClone->GetPSDActorBodyIdOnPhysicsService();

	// Get the PSDActor to clone location as string
	const auto PSDActorCloneLocation = 
		PSDActorToClone->GetCurrentActorLocationAsString();

	// Create the message to send server
	// The template is:
	// "AddBody\n
	// actorType; Id; bodyType; posX; posY; posZ\n
	// MessageEnd\n"
	const FString SpawnNewPSDActorCloneMessage =
		FString::Printf(TEXT("AddBody\nsphere;%d;clone;%s\nMessageEnd\n"), 
		PSDActorBodyID, *PSDActorCloneLocation);

	// Convert message to std string
	std::string MessageAsStdString
		(TCHAR_TO_UTF8(*SpawnNewPSDActorCloneMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to add the body to the physics world on service
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar, RegionOwnerPhysicsServiceId);

	RPES_LOG_INFO(TEXT("Add new PSDActor clone action response: %s"),
		*Response);
}

APSDActorBase* APhysicsServiceRegion::SpawnPSDActorFromPhysicsServiceClone
	(const APSDActorBase* TargetClonedPSDActor)
{
	// Check if we have a valid PSDActors spawner. If not, find it
	check(PSDActorSpawner);

	// Get the PSDActor location to spawn
	const FVector NewPSDActorLocation = 
		TargetClonedPSDActor->GetActorLocation();

	// Spawn the new sphere on the new sphere location and this region's owner
	// physics service ID (The ID is needed to avoid the "OnRegionEntry" being
	// called on the newly spawned actor).
	const auto SpawnedPSDActor = PSDActorSpawner->SpawnPSDActor
		(NewPSDActorLocation, RegionOwnerPhysicsServiceId);

	RPES_LOG_WARNING(TEXT("Spawning new PSDActor (%s) from clone \"%s\" on "
		"region(id: %d) at pos: %s"), *SpawnedPSDActor->GetName(), 
		*TargetClonedPSDActor->GetName(), RegionOwnerPhysicsServiceId, 
		*NewPSDActorLocation.ToString());

	// Get the cloned PSDActor body ID so the new PSDActor sphere has the
	// same body ID as his clone
	const auto ClonedPSDActorBodyID = 
		TargetClonedPSDActor->GetPSDActorBodyIdOnPhysicsService();

	// Override the spawned actor's PSDActor body ID on physics service to be
	// the same as his clone
	SpawnedPSDActor->SetPSDActorBodyIdOnPhysicsService(ClonedPSDActorBodyID);

	// Set the new PSDActor status on this region, as it now is inside a
	// region
	SpawnedPSDActor->UpdatePSDActorStatusOnRegion
		(EPSDActorPhysicsRegionStatus::InsideRegion);

	// Add the sphere to the dynamic PSDActors map so it's Transform can be 
	// updated on next step
	DynamicPSDActorsOnRegion.Add(ClonedPSDActorBodyID, SpawnedPSDActor);

	// Create the message to send server to update the body type. This is 
	// needed as the body is now primary for this service
	// The template is:
	// "UpdateBodyType\n
	// Id; newBodyType\n
	// MessageEnd\n"
	const FString UpdateBodyTypeMessage =
		FString::Printf(TEXT("UpdateBodyType\n%d;primary\nMessageEnd\n"),
		ClonedPSDActorBodyID);

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*UpdateBodyTypeMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to update the body type on service
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar, RegionOwnerPhysicsServiceId);

	RPES_LOG_INFO(TEXT("Update PSDActor BodyType response: %s"),
		*Response);

	return SpawnedPSDActor;
}

void APhysicsServiceRegion::DestroyPSDActorOnPhysicsRegion
	(APSDActorBase* PSDActorToDestroy)
{
	// First, remove the PSDActor from the physics service
	RemovePSDActorFromPhysicsService(PSDActorToDestroy);

	// Get the actor's body id on physics service to find it on the dynamic 
	// PSDActors map
	const auto PSDActorBodyId =
		PSDActorToDestroy->GetPSDActorBodyIdOnPhysicsService();

	// Remove this PSDActor from the dynamic PSDActors map
	DynamicPSDActorsOnRegion.Remove(PSDActorBodyId);

	// Destroy the PSDActor
	PSDActorToDestroy->Destroy();
}

void APhysicsServiceRegion::RemovePSDActorFromPhysicsService
	(APSDActorBase* PSDActorToRemove)
{
	RPES_LOG_INFO(TEXT("Removing PSDActor \"%s\" from physics region(id: %d)"),
		*PSDActorToRemove->GetName(), RegionOwnerPhysicsServiceId);

	// Check if the PSDActor to remove is valid
	if (!PSDActorToRemove)
	{
		RPES_LOG_ERROR(TEXT("Could not remove PSDActor as reference is "
			"invalid."));
		return;
	}

	// Get the actor's body ID on the physics service 
	const auto BodyIdToRemove = 
		PSDActorToRemove->GetPSDActorBodyIdOnPhysicsService();

	// Create the message to send to the physics service
	// The template is:
	// "RemoveBody\n
	// Id\n
	// MessageEnd\n"
	const FString RemoveBodyMessage = 
		FString::Printf(TEXT("RemoveBody\n%d\nMessageEnd\n"), BodyIdToRemove);

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*RemoveBodyMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to initialize physics world on service
	// TODO this should receive a given physics service id
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar, RegionOwnerPhysicsServiceId);

	RPES_LOG_INFO(TEXT("Remove body request response: %s"), *Response);
}

void APhysicsServiceRegion::ClearPhysicsServiceRegion()
{
	RPES_LOG_INFO(TEXT("Clearing physics service region (id: %d)."),
		RegionOwnerPhysicsServiceId);

	// Set the flag to false to indicate this region is not active anymore
	bIsPhysicsServiceRegionActive = false;

	// Get all PSDActors on this region
	auto PSDActorsOnRegion = GetAllPSDActorsOnRegion();

	// Foreach PSDActor on this region, destroy it
	for (auto& PSDActor : PSDActorsOnRegion)
	{
		PSDActor->Destroy();
	}

	// Clear the map and list
	DynamicPSDActorsOnRegion.Empty();
	PendingMigrationPSDActors.Empty();

	// Close socket connection on this physics service (given its ID)
	const bool bWasCloseSocketSuccess =
		FSocketClientProxy::CloseSocketConnectionsToServerById
		(RegionOwnerPhysicsServiceId);

	// Check for errors on close. 
	// Any error should be shown in log
	if (!bWasCloseSocketSuccess)
	{
		RPES_LOG_ERROR(TEXT("Socket closing error. Check logs."));
		return;
	}

	RPES_LOG_INFO(TEXT("Physics service service (id: %d) socket closed."),
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

		// Set the new PSDActor status on this region, as it now resides
		// inside of it
		FoundActorAsPSDActorBase->UpdatePSDActorStatusOnRegion
			(EPSDActorPhysicsRegionStatus::InsideRegion);
	}

	return PSDActorsOnRegion;
}

void APhysicsServiceRegion::OnRegionEntry
	(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp,	int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	// If not on server, just ignore this call
	if (!HasAuthority())
	{
		return;
	}

	// If physics service region is not active, just ignore (we may be clearing
	// the physics region and removing all PSDActors from it)
	if (!bIsPhysicsServiceRegionActive)
	{
		return;
	}

	// Get the other actor as PSDActor
	auto OtherActorAsPSDActor = Cast<APSDActorBase>(OtherActor);

	// If not valid, just ignore. We only want PSDActors
	if (!OtherActorAsPSDActor)
	{
		return;
	}

	// Call the method to when he enteres a new physics service region
	OtherActorAsPSDActor->OnEnteredPhysicsRegion(RegionOwnerPhysicsServiceId);

	/**
	// Get the OtherActor physics service ID
	const auto OtherActorPhysicsServiceId = 
		OtherActorAsPSDActor->GetActorOwnerPhysicsServiceId();
	
	// If this actor already belongs to this region, ignore it. We want to 
	// process only PSDActors coming from other regions
	if (OtherActorPhysicsServiceId == RegionOwnerPhysicsServiceId)
	{
		return;
	}

	// If the actor is already inside a region and enters this region, then
	// now he is inside a shared region
	if (OtherActorAsPSDActor->GetPSDActorPhysicsRegionStatus() ==
		EPSDActorPhysicsRegionStatus::InsideRegion)
	{
		RPES_LOG_WARNING(TEXT("Actor \"%s\" entried region (id: %d) from \
			region with physics service owning id: %d (pos: %s)"),
			*OtherActorAsPSDActor->GetName(), RegionOwnerPhysicsServiceId,
			OtherActorPhysicsServiceId,
			*OtherActorAsPSDActor->GetActorLocation().ToString());

		// Add the PSDActor clone on this physics service
		AddPSDActorCloneOnPhysicsService(OtherActorAsPSDActor);

		// Add the PSDActor to the pending migration list
		PendingMigrationPSDActors.Add(OtherActorAsPSDActor);

		// Bind the actor's exited event on the callback
		OtherActorAsPSDActor->OnActorExitedCurrentPhysicsRegion.AddDynamic(this,
			&APhysicsServiceRegion::OnActorFullyExitedOwnPhysicsRegion);

		// Update the PSDActor's region status
		OtherActorAsPSDActor->UpdatePSDActorStatusOnRegion
			(EPSDActorPhysicsRegionStatus::SharedRegion);

		// Call the method to when he eneters a new physics service region
		OtherActorAsPSDActor->OnEnteredNewPhysicsRegion();

		return;
	}

	// If actor is not coming from a previous PSDActor region, we must treat
	// it somehow. They engine may be simulating its 
	RPES_LOG_WARNING(TEXT("A PSDActor has entered a region without being \
		previously inside a region. This is not yet treated."));
	*/
}

void APhysicsServiceRegion::OnRegionExited
	(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// If not on server, just ignore this call
	if (!HasAuthority())
	{
		return;
	}

	// If physics service region is not active, just ignore (we may be clearing
	// the physics region and removing all PSDActors from it)
	if (!bIsPhysicsServiceRegionActive)
	{
		return;
	}

	// Get the other actor as PSDActor
	auto OtherActorAsPSDActor = Cast<APSDActorBase>(OtherActor);

	// If not valid, just ignore. We only want PSDActors
	if (!OtherActorAsPSDActor)
	{
		return;
	}

	// Call the method on the PSDActorBase so he knows he has exited this
	// physics service region
	OtherActorAsPSDActor->OnExitedPhysicsRegion(RegionOwnerPhysicsServiceId);

	/*
	// Get the OtherActor physics service ID
	const auto OtherActorPhysicsServiceId =
		OtherActorAsPSDActor->GetActorOwnerPhysicsServiceId();

	// If this actor does not belong to this region, ignore it. We want only
	// to process PSDActors inside this region
	if (OtherActorPhysicsServiceId != RegionOwnerPhysicsServiceId)
	{
		return;
	}

	RPES_LOG_WARNING(TEXT("Actor \"%s\" exited region with physics service \
		owning id: %d."), *OtherActorAsPSDActor->GetName(), 
		RegionOwnerPhysicsServiceId);
	
	// Remove this PSDActor from the phyiscs service as it is no longer
	// responsible for simulating it
	RemovePSDActorFromPhysicsService(OtherActorAsPSDActor);

	// Get the actor's body id on physics service to find it on the dynamic 
	// PSDActors map
	const auto PSDActorBodyId = 
		OtherActorAsPSDActor->GetPSDActorBodyIdOnPhysicsService();

	// Remove this PSDActor from the dynamic PSDActors map
	DynamicPSDActorsOnRegion.Remove(PSDActorBodyId);

	// Call the method on the PSDActorBase so he knows he has exited this
	// physics service region
	OtherActorAsPSDActor->OnExitedPhysicsRegion();

	// Set the new PSDActor status on this region, as it now exited the region
	OtherActorAsPSDActor->UpdatePSDActorStatusOnRegion
		(EPSDActorPhysicsRegionStatus::NoRegion);
	*/
}

void APhysicsServiceRegion::OnActorFullyExitedOwnPhysicsRegion(APSDActorBase*
	ExitedActor)
{
	/*
	RPES_LOG_INFO(TEXT("Physics service region (id:%d) processed actor \
		\"%s\" fully exiting previous region."), RegionOwnerPhysicsServiceId,
		*ExitedActor->GetName());

	// Remove the callback
	ExitedActor->OnActorExitedCurrentPhysicsRegion.RemoveDynamic(this,
		&APhysicsServiceRegion::OnActorFullyExitedOwnPhysicsRegion);

	// Check if this exited actor is on the pending migration list
	if (!PendingMigrationPSDActors.Contains(ExitedActor))
	{
		RPES_LOG_WARNING(TEXT("Actor was pending migration but is not on \
			migration list: %s"), *ExitedActor->GetName());
		return;
	}

	// Spawn a new PSD sphere on this region where the other actor is
	SpawnPSDActorFromPhysicsServiceClone(ExitedActor);

	// Remove the actor from the pending migration list
	PendingMigrationPSDActors.Remove(ExitedActor);

	// Destroy the OtherActor, as now he will be simulated inside this region
	ExitedActor->Destroy();
	*/
}