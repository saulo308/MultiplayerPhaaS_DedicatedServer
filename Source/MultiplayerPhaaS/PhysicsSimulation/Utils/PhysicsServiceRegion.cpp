// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsServiceRegion.h"
#include "Components/BoxComponent.h"

#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorBase.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"

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

TArray<APSDActorBase*> APhysicsServiceRegion::GetAllPSDActorsOnRegion()
{
	// Clear the current PSDActor list
	PSDActorsOnRegion.Empty();

	// Get all overlapping PSDActors on this region
	TArray<AActor*> FoundActors;
	PhysicsServiceRegionBoxComponent->GetOverlappingActors(FoundActors,
		APSDActorBase::StaticClass());

	// Cast all to "APSDActorBase"
	MPHAAS_LOG_INFO(TEXT("Num: %d"), FoundActors.Num());
	for (auto& FoundPSDActor : FoundActors)
	{
		// Get the found actor as PSDActorBase
		auto FoundActorAsPSDActorBase = Cast<APSDActorBase>(FoundPSDActor);

		// Add it to the list of actors on this region
		PSDActorsOnRegion.Add(FoundActorAsPSDActorBase);

		// Set the PSDActor owner physics service id
		FoundActorAsPSDActorBase->SetActorPhysicsServiceOwnerId
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

	MPHAAS_LOG_INFO
		(TEXT("Actor \"%s\" entried region with physics service owning id: %d."),
		*OtherActorAsPSDActor->GetName(), RegionOwnerPhysicsServiceId);
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
}
