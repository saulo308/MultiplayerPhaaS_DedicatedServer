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

	// Get the current actor's linear velocity as a string
	const auto CurrentActorLinearVelocityAsString =
		GetPSDActorLinearVelocityAsString();

	// Get the current actor's angular velocity as a string
	const auto CurrentActorAngularVelocityString =
		GetPSDActorAngularVelocityAsString();

	// For the initialization message, format to the template message:
	// "ActorTypeString; BodyID; bodyType; InitialPosX; InitialPosY; 
	//	InitialPosY; InitialLinearVelocityX; InitialLinearVelocityY;
	// InitialLinearVelocityZ; InitialAngularVelocityX; 
	// InitialAngularVelocityY; InitialAngularVelocityZ\n"
	PSDActorPhysicsServiceInitializationString +=
		FString::Printf(TEXT("floor;%d;primary;%s;%s;%s\n"), PSDActorBodyId,
		*CurrentActorLocationAsString, *CurrentActorLinearVelocityAsString,
		*CurrentActorAngularVelocityString);

	return PSDActorPhysicsServiceInitializationString;
}
