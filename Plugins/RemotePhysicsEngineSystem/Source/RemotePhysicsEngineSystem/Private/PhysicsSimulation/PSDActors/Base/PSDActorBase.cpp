// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#include "PhysicsSimulation/PSDActors/Base/PSDActorBase.h"
#include "Components/StaticMeshComponent.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"

APSDActorBase::APSDActorBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Creating the actor's root
	ActorRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ActorRoot"));
	RootComponent = ActorRootComponent;

	// Creating the actor's mesh
	ActorMeshComponent =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActorMesh"));
	ActorMeshComponent->SetupAttachment(ActorRootComponent);

	// Set this actor to replicate as it will spawn on the server
	bReplicates = true;
	SetReplicateMovement(true);

	// Set the actor's BodyID on the physics service as the UniqueID 
	PSDActorBodyIdOnPhysicsService = GetUniqueID();
}

void APSDActorBase::BeginPlay()
{
	Super::BeginPlay();
}

void APSDActorBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FString APSDActorBase::GetPhysicsServiceInitializationString()
{
	RPES_LOG_ERROR(TEXT("Do not instantiate PSDActorBase directly. \
		GetPhysicsServiceInitializationString() should be overwritten."));
	return FString();
}

FString APSDActorBase::GetCurrentActorLocationAsString()
{
	// Get the actor location
	const FVector ActorPos = GetActorLocation();

	// Parse it into a string with ";" delimiters
	const FString StepPhysicsString =
		FString::Printf(TEXT("%f;%f;%f"), ActorPos.X, ActorPos.Y,
		ActorPos.Z);

	return StepPhysicsString;
}

void APSDActorBase::UpdatePositionAfterPhysicsSimulation
	(const FVector NewActorPosition)
{
	SetActorLocation(NewActorPosition);
}

void APSDActorBase::UpdateRotationAfterPhysicsSimulation
	(const FVector NewActorRotationEulerAngles)
{
	FQuat NewRotation = FQuat::MakeFromEuler(NewActorRotationEulerAngles);
	SetActorRotation(NewRotation);
}

void APSDActorBase::OnEnteredNewPhysicsRegion()
{

}

void APSDActorBase::OnExitedPhysicsRegion()
{
	OnActorExitedCurrentPhysicsRegion.Broadcast(this);
}
