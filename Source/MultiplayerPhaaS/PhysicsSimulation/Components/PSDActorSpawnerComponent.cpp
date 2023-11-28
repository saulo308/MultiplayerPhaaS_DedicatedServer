// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PSDActorSpawnerComponent.h"

UPSDActorSpawnerComponent::UPSDActorSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPSDActorSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPSDActorSpawnerComponent::TickComponent(float DeltaTime, 
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
