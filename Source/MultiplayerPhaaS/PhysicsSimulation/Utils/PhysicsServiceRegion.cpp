// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsServiceRegion.h"
#include "Components/BoxComponent.h"

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
}

void APhysicsServiceRegion::BeginPlay()
{
	Super::BeginPlay();
}

void APhysicsServiceRegion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
