// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#include "PhysicsSimulation/PSDActors/Base/PSDActorBase.h"
#include "Components/StaticMeshComponent.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"

APSDActorBase::APSDActorBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Creating the actor's root
	ActorRootComponent = CreateDefaultSubobject<USceneComponent>
		(TEXT("ActorRoot"));
	RootComponent = ActorRootComponent;

	// Creating the actor's mesh
	ActorMeshComponent =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActorMesh"));
	ActorMeshComponent->SetupAttachment(ActorRootComponent);

	// Creating the actor's body id text render component. This will show
	// the actor's body id as 3D text
	ActorBodyIdTextRenderComponent =
		CreateDefaultSubobject<UTextRenderComponent>
		(TEXT("ActorBodyIdTextRenderComponent"));
	ActorBodyIdTextRenderComponent->SetupAttachment(ActorRootComponent);

	// Creating the actor's region status text render component. This will show
	// the actor's region statusas 3D text
	ActorRegionStatusTextRender =
		CreateDefaultSubobject<UTextRenderComponent>
		(TEXT("ActorRegionStatusTextRender"));
	ActorRegionStatusTextRender->SetupAttachment(ActorRootComponent);

	// Set this actor to replicate as it will spawn on the server
	bReplicates = true;
	SetReplicateMovement(true);

	// Set the actor's BodyID as the UniqueID
	PSDActorBodyId = GetUniqueID();
}

void APSDActorBase::BeginPlay()
{
	Super::BeginPlay();

	// Set the actor's body id on the server
	if (HasAuthority())
	{
		// Set the actor's BodyID as the UniqueID
		PSDActorBodyId = GetUniqueID();
	}
}

void APSDActorBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FString APSDActorBase::GetCurrentActorLocationAsString() const
{
	// Get the actor location
	const FVector ActorPos = GetActorLocation();

	// Parse it into a string with ";" delimiters
	const FString StepPhysicsString =
		FString::Printf(TEXT("%f;%f;%f"), ActorPos.X, ActorPos.Y,
			ActorPos.Z);

	return StepPhysicsString;
}

void APSDActorBase::GetLifetimeReplicatedProps
	(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APSDActorBase, ActorOwnerPhysicsServiceRegionId);
	DOREPLIFETIME(APSDActorBase, PSDActorBodyId);
	DOREPLIFETIME(APSDActorBase, CurrentPSDActorPhysicsRegionStatus);
}

FString APSDActorBase::GetPhysicsServiceInitializationString()
{
	RPES_LOG_ERROR(TEXT("Do not instantiate PSDActorBase directly. "
		"GetPhysicsServiceInitializationString() should be overwritten."));
	return FString();
}

void APSDActorBase::OnEnteredPhysicsRegion(int32 EnteredPhysicsRegionId)
{
	OnActorEnteredPhysicsRegion.Broadcast(this, EnteredPhysicsRegionId);
}

void APSDActorBase::OnExitedPhysicsRegion(int32 ExitedPhysicsRegionId)
{
	OnActorExitedPhysicsRegion.Broadcast(this, ExitedPhysicsRegionId);
}

void APSDActorBase::OnRep_PhysicsRegionStatusUpdated()
{
	// Switch the enum and set the new physics region status string
	FString NewPhysicsRegionStatusAsString = FString();
	switch (CurrentPSDActorPhysicsRegionStatus)
	{
	case EPSDActorPhysicsRegionStatus::InsideRegion:
		NewPhysicsRegionStatusAsString = "InsideRegion";
		break;
	case EPSDActorPhysicsRegionStatus::SharedRegion:
		NewPhysicsRegionStatusAsString = "SharedRegion";
		break;
	case EPSDActorPhysicsRegionStatus::NoRegion:
		NewPhysicsRegionStatusAsString = "NoRegion";
		break;
	default:
		break;
	}

	// Update the text render component text
	ActorRegionStatusTextRender->SetText
	(FText::FromString(*NewPhysicsRegionStatusAsString));
}

void APSDActorBase::OnRep_PSDActorBodyId()
{
	// Update the text render component
	ActorBodyIdTextRenderComponent->SetText(FText::AsNumber(PSDActorBodyId));
}

void APSDActorBase::UpdatePositionAfterPhysicsSimulation
	(const FVector& NewActorPosition)
{
	// If on server, update it
	if (HasAuthority())
	{
		SetActorLocation(NewActorPosition);
	}
}

void APSDActorBase::UpdateRotationAfterPhysicsSimulation
	(const FVector& NewActorRotationEulerAngles)
{
	// If on server, update it
	if (HasAuthority())
	{
		FQuat NewRotation = FQuat::MakeFromEuler(NewActorRotationEulerAngles);
		SetActorRotation(NewRotation);
	}
}

void APSDActorBase::UpdatePSDActorStatusOnRegion
	(EPSDActorPhysicsRegionStatus NewPhysicsRegionStatus)
{
	// If on server, update it
	if (HasAuthority())
	{
		// Update the current PSDActor physics region status
		CurrentPSDActorPhysicsRegionStatus = NewPhysicsRegionStatus;
	}
}
