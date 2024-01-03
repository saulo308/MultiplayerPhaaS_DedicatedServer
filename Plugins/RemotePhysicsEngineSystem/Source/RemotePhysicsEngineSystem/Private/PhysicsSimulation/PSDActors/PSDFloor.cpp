// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsSimulation/PSDActors/PSDFloor.h"

APSDFloor::APSDFloor()
{
	// Set if this actor is static to true
	bIsPSDActorStatic = true;
}

FString APSDFloor::GetPhysicsServiceInitializationString()
{
	// Create the initialization string
	FString PSDActorPhysicsServiceInitializationString = FString();

	// Get the current actor's location as a string
	const auto CurrentActorLocationAsString = 
		GetCurrentActorLocationAsString();

	// For the initialization message, format to the template message:
	// "ActorTypeString; BodyID; InitialPosX; InitialPosY; InitialPosY\n"
	PSDActorPhysicsServiceInitializationString +=
		FString::Printf(TEXT("floor;%d;%s\n"), PSDActorBodyIdOnPhysicsService, 
		*CurrentActorLocationAsString);

	return PSDActorPhysicsServiceInitializationString;
}
