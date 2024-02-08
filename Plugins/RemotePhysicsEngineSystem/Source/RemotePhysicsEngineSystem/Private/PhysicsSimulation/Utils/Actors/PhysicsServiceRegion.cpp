// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsSimulation/Utils/Actors/PhysicsServiceRegion.h"
#include "PhysicsSimulation/PSDActors/Base/PSDActorBase.h"
#include "PhysicsSimulation/Utils/Components/PSDactorSpawnerComponent.h"
#include "ExternalCommunication/Sockets/SocketClientProxy.h"
#include "ExternalCommunication/Sockets/SocketClientInstance.h"
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

void APhysicsServiceRegion::AddPSDActorCloneOnPhysicsService
	(const APSDActorBase* PSDActorToClone)
{
	RPES_LOG_INFO(TEXT("Adding PSDActor \"%s\" clone on region (id: %d)"),
		*PSDActorToClone->GetName(), RegionOwnerPhysicsServiceId);

	// Get the PSDActor ID to use it as the body ID
	const int32 PSDActorBodyID = PSDActorToClone->GetPSDActorBodyId();

	// Get the PSDActor to clone location as string
	const auto PSDActorCloneLocation =
		PSDActorToClone->GetCurrentActorLocationAsString();

	// Get the current actor's linear velocity as a string
	const auto PSDActorCloneLinearVelocityAsString =
		PSDActorToClone->GetPSDActorLinearVelocityAsString();

	// Get the current actor's angular velocity as a string
	const auto PSDActorCloneAngularVelocityString =
		PSDActorToClone->GetPSDActorAngularVelocityAsString();

	// Create the message to send server
	// The template is:
	// "AddBody\n
	// actorType; Id; bodyType; posX; posY; posZ; LinearVelocityX; 
	// LinearVelocityY; LinearVelocityZ;AngularVelocityX; AngularVelocityY;
	// AngularVelocityZ\nMessageEnd\n"
	const FString SpawnNewPSDActorCloneMessage =
		FString::Printf(TEXT("AddBody\nsphere;%d;clone;%s;%s;%s\nMessageEnd\n"),
			PSDActorBodyID, *PSDActorCloneLocation,
			*PSDActorCloneLinearVelocityAsString, 
			*PSDActorCloneAngularVelocityString);

	// Get the socket connection instance to send the message
	auto* SocketConnectionToSend = 
		FSocketClientProxy::GetSocketConnectionByServerId
		(RegionOwnerPhysicsServiceId);

	// Check if valid 
	if (!SocketConnectionToSend)
	{
		RPES_LOG_ERROR(TEXT("Could not send message to socket with ID \"%d\" "
			"as such connection does not exist."), 
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Convert message to std string
	std::string MessageAsStdString
		(TCHAR_TO_UTF8(*SpawnNewPSDActorCloneMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to add the body to the physics world on service
	const FString Response = SocketConnectionToSend->SendMessageAndGetResponse
		(MessageAsChar);

	RPES_LOG_INFO(TEXT("Add new PSDActor clone action response: %s"),
		*Response);
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

void APhysicsServiceRegion::DestroyPSDActorOnPhysicsRegion
	(APSDActorBase* PSDActorToDestroy)
{
	// First, remove the PSDActor from the physics service
	RemovePSDActorFromPhysicsService(PSDActorToDestroy);

	// Get the actor's body id to find it on the dynamic PSDActors map
	const auto PSDActorBodyId = PSDActorToDestroy->GetPSDActorBodyId();

	// Remove this PSDActor from the dynamic PSDActors map
	DynamicPSDActorsOnRegion.Remove(PSDActorBodyId);

	// Destroy the PSDActor
	PSDActorToDestroy->Destroy();
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
		// The key is the PSDActor's body ID
		// The value is the reference to the actor
		DynamicPSDActorsOnRegion.Add(PSDActor->GetPSDActorBodyId(), PSDActor);
	}
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

		// Set the PSDActor owner physics service region id
		FoundActorAsPSDActorBase->SetActorOwnerPhysicsServiceRegionId
		(RegionOwnerPhysicsServiceId);

		// Set the new PSDActor status on this region, as it now resides
		// inside of it
		FoundActorAsPSDActorBase->UpdatePSDActorStatusOnRegion
		(EPSDActorPhysicsRegionStatus::InsideRegion);
	}

	return PSDActorsOnRegion;
}

void APhysicsServiceRegion::InitializePhysicsServiceRegion(const FString&
	RegionPhysicsServiceIpAddr)
{
	RPES_LOG_INFO(TEXT("Starting PSD actors simulation on region with "
		"ID: %d."), RegionOwnerPhysicsServiceId);

	// Set the region's physics service ip addr
	PhysicsServiceIpAddr = RegionPhysicsServiceIpAddr;

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

	// Get the socket connection instance to send the message
	auto* SocketConnectionToSend = 
		FSocketClientProxy::GetSocketConnectionByServerId
		(RegionOwnerPhysicsServiceId);

	// Check if valid 
	if (!SocketConnectionToSend)
	{
		RPES_LOG_ERROR(TEXT("Could not send message to socket with ID \"%d\" "
			"as such connection does not exist."),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*InitializationMessage));

	// Convert message to char*. This is needed as some UE converting has 
	// the limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	RPES_LOG_INFO(TEXT("Sending init message for service with id \"%d\". "
		"Message: %s"), RegionOwnerPhysicsServiceId, *InitializationMessage);

	// Send message to initialize physics world on service
	const FString Response = SocketConnectionToSend->SendMessageAndGetResponse
		(MessageAsChar);

	RPES_LOG_WARNING(TEXT("Physics service with ID (%d) response: %s"),
		RegionOwnerPhysicsServiceId, *Response);
}

void APhysicsServiceRegion::OnRegionEntry
	(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
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

	// Get the actor's body ID
	const auto BodyIdToRemove = PSDActorToRemove->GetPSDActorBodyId();

	// Create the message to send to the physics service
	// The template is:
	// "RemoveBody\n
	// BodyId\n
	// MessageEnd\n"
	const FString RemoveBodyMessage =
		FString::Printf(TEXT("RemoveBody\n%d\nMessageEnd\n"), BodyIdToRemove);

	// Get the socket connection instance to send the message
	auto* SocketConnectionToSend =
		FSocketClientProxy::GetSocketConnectionByServerId
		(RegionOwnerPhysicsServiceId);

	// Check if valid 
	if (!SocketConnectionToSend)
	{
		RPES_LOG_ERROR(TEXT("Could not send message to socket with ID \"%d\" "
			"as such connection does not exist."),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*RemoveBodyMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to remove body from the physics world on service
	const FString Response = SocketConnectionToSend->SendMessageAndGetResponse
		(MessageAsChar);

	RPES_LOG_INFO(TEXT("Remove body request response: %s"), *Response);
}

void APhysicsServiceRegion::RemovePSDActorOwnershipFromRegion
	(APSDActorBase* TargetPSDActor)
{
	RPES_LOG_INFO(TEXT("Removing PSDActor \"%s\" ownership from region "
		"(id:%d)"), *TargetPSDActor->GetName(), RegionOwnerPhysicsServiceId);

	// Get the PSDActor body id
	const auto PSDActorBodyId = TargetPSDActor->GetPSDActorBodyId();

	// Check if this region owns this PSDActor
	if (!DynamicPSDActorsOnRegion.Contains(PSDActorBodyId))
	{
		RPES_LOG_ERROR(TEXT("Requested to remove PSDActor \"%s\" ownership "
			"from region (id:%d), but region does not own the target "
			"PSDActor"), *TargetPSDActor->GetName(),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Remove the PSDActor from dynamic PSDActors map so this region wont
	// try to update it
	DynamicPSDActorsOnRegion.Remove(PSDActorBodyId);
}

void APhysicsServiceRegion::UpdatePSDActorsOnRegion
	(const FString& PhysicsSimulationResultStr)
{
	//RPES_LOG_INFO(TEXT("Updating PSD actors on region with ID: %d."),
		//RegionOwnerPhysicsServiceId);

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
		if (ParsedActorSimulationResult.Num() < 13)
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

		// Update PSD actor linear velocity
		const float NewLinearVelocityX =
			FCString::Atof(*ParsedActorSimulationResult[7]);
		const float NewLinearVelocityY =
			FCString::Atof(*ParsedActorSimulationResult[8]);
		const float NewLinearVelocityZ =
			FCString::Atof(*ParsedActorSimulationResult[9]);
		const FVector NewLinearVelocity(NewLinearVelocityX, NewLinearVelocityY,
			NewLinearVelocityZ);

		ActorToUpdate->SetPSDActorLinearVelocity(NewLinearVelocity);

		// Update PSD actor angular velocity
		const float NewAngularVelocityX =
			FCString::Atof(*ParsedActorSimulationResult[10]);
		const float NewAngularVelocityY =
			FCString::Atof(*ParsedActorSimulationResult[11]);
		const float NewAngularVelocityZ =
			FCString::Atof(*ParsedActorSimulationResult[12]);
		const FVector NewAngularVelocity(NewAngularVelocityX,
			NewAngularVelocityY, NewAngularVelocityZ);

		ActorToUpdate->SetPSDActorAngularVelocity(NewAngularVelocity);

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

void APhysicsServiceRegion::UpdatePSDActorBodyType
	(const APSDActorBase* TargetPSDActor, const FString& NewBodyType)
{
	// Create the message to send server to update the body type. This is 
	// needed as the body is now primary for this service
	// The template is:
	// "UpdateBodyType\n
	// BodyId; newBodyType\n
	// MessageEnd\n"
	const FString UpdateBodyTypeMessage =
		FString::Printf(TEXT("UpdateBodyType\n%d;%s\nMessageEnd\n"),
		TargetPSDActor->GetPSDActorBodyId(), *NewBodyType);

	// Get the socket connection instance to send the message
	auto* SocketConnectionToSend =
		FSocketClientProxy::GetSocketConnectionByServerId
		(RegionOwnerPhysicsServiceId);

	// Check if valid 
	if (!SocketConnectionToSend)
	{
		RPES_LOG_ERROR(TEXT("Could not send message to socket with ID \"%d\" "
			"as such connection does not exist."),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*UpdateBodyTypeMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to update the body type on service
	const FString Response = SocketConnectionToSend->SendMessageAndGetResponse
		(MessageAsChar);

	RPES_LOG_INFO(TEXT("Update PSDActor BodyType response: %s"),
		*Response);
}

void APhysicsServiceRegion::SetPSDActorOwnershipToRegion
	(APSDActorBase* TargetPSDActor)
{
	RPES_LOG_INFO(TEXT("Setting PSDActor \"%s\" ownership to region "
		"(id:%d)"), *TargetPSDActor->GetName(), RegionOwnerPhysicsServiceId);

	// Get the PSDActor body id
	const auto PSDActorBodyId = TargetPSDActor->GetPSDActorBodyId();

	// Update the PSDActor's owner physics service region id
	TargetPSDActor->SetActorOwnerPhysicsServiceRegionId
		(RegionOwnerPhysicsServiceId);

	// Add the PSDActor to this region's DynamicPSDActors so it will start to
	// update it
	DynamicPSDActorsOnRegion.Add(PSDActorBodyId,
		TargetPSDActor);
}

void APhysicsServiceRegion::SpawnNewPSDSphere(const FVector NewSphereLocation,
	const FVector NewSphereLinearVelocity, const FVector 
	NewSphereAngularVelocity)
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
	const int32 NewSphereBodyId = SpawnedSphere->GetPSDActorBodyId();

	// Add the sphere to the dynamic PSDActors map so it's Transform can be 
	// updated on next step
	DynamicPSDActorsOnRegion.Add(NewSphereBodyId, SpawnedSphere);

	// Create the message to send server
	// The template is:
	// "AddBody\n
	// actorType; Id; bodyType; posX; posY; posZ\n
	// MessageEnd\n"
	const FString SpawnNewPSDSphereMessage =
		FString::Printf(TEXT("AddBody\nspere;%d;primary;%f;%f;%f;%f;%f;%f;%f;%f;%f\nMessageEnd\n"), 
		NewSphereBodyId, NewSphereLocation.X, NewSphereLocation.Y,
		NewSphereLocation.Z, NewSphereLinearVelocity.X, 
		NewSphereLinearVelocity.Y, NewSphereLinearVelocity.Z,
		NewSphereAngularVelocity.X, NewSphereAngularVelocity.Y, 
		NewSphereAngularVelocity.Z);

	// Get the socket connection instance to send the message
	auto* SocketConnectionToSend =
		FSocketClientProxy::GetSocketConnectionByServerId
		(RegionOwnerPhysicsServiceId);

	// Check if valid 
	if (!SocketConnectionToSend)
	{
		RPES_LOG_ERROR(TEXT("Could not send message to socket with ID \"%d\" "
			"as such connection does not exist."),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*SpawnNewPSDSphereMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to initialize physics world on service
	const FString Response = SocketConnectionToSend->SendMessageAndGetResponse
		(MessageAsChar);

	RPES_LOG_INFO(TEXT("Add new sphere action response: %s"), *Response);
}

void APhysicsServiceRegion::SavePhysicsServiceMeasuresements()
{
	// Create the message to send to the physics service
	// The template is:
	// "GetSimulationMeasures\n
	// MessageEnd\n"
	const FString GetSimulationMeasuresMessage =
		FString::Printf(TEXT("GetSimulationMeasures\n%d\nMessageEnd\n"));

	// Get the socket connection instance to send the message
	auto* SocketConnectionToSend =
		FSocketClientProxy::GetSocketConnectionByServerId
		(RegionOwnerPhysicsServiceId);

	// Check if valid 
	if (!SocketConnectionToSend)
	{
		RPES_LOG_ERROR(TEXT("Could not send message to socket with ID \"%d\" "
			"as such connection does not exist."),
			RegionOwnerPhysicsServiceId);
		return;
	}

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8
		(*GetSimulationMeasuresMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to initialize physics world on service
	const FString Response = SocketConnectionToSend->SendMessageAndGetResponse
		(MessageAsChar);
	
	// Save the physics service measurements to file
	SavePhysicsServiceMeasuresToFile(Response);
}

void APhysicsServiceRegion::SavePhysicsServiceMeasuresToFile
	(const FString& Measurements) const
{
	FString TargetFolder = TEXT("StepPhysicsMeasureWithoutCommsOverhead");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diret�rio se j� n�o existe
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
	FString FileName = FString::Printf(TEXT("/StepPhysicsTime_%s_Region%d_%d.txt"),
		*MapName, RegionOwnerPhysicsServiceId, FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/StepPhysicsTime_%s_Region%d_%d.txt"),
			*MapName, RegionOwnerPhysicsServiceId, FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	RPES_LOG_WARNING(TEXT("Saving service step physics time measurement into "
		"\"%s\""), *FileFullPath);

	FFileHelper::SaveStringToFile(Measurements, *FileFullPath);
}
