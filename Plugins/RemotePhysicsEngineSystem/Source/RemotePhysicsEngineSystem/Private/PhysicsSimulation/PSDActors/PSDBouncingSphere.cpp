// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsSimulation/PSDActors/PSDBouncingSphere.h"

APSDBouncingSphere::APSDBouncingSphere()
{
	// Set if this actor is static to false as it should be a dynamic body
	bIsPSDActorStatic = false;
}

FString APSDBouncingSphere::GetPhysicsServiceInitializationString()
{
	// Create the initialization string
	FString PSDActorPhysicsServiceInitializationString = FString();

	// Get the current actor's location as a string
	const auto CurrentActorLocationAsString = 
		GetCurrentActorLocationAsString();

	// For the initialization message, format to the template message:
	// "ActorTypeString; BodyID; InitialPosX; InitialPosY; InitialPosY\n"
	PSDActorPhysicsServiceInitializationString +=
		FString::Printf(TEXT("sphere;%d;%s\n"), PSDActorBodyIdOnPhysicsService,
		*CurrentActorLocationAsString);

	return PSDActorPhysicsServiceInitializationString;
}
