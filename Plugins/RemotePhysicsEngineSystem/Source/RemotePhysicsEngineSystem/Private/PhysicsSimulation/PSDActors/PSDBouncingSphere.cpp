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
		FString::Printf(TEXT("sphere;%d;primary;%s;%s;%s\n"), PSDActorBodyId, 
		*CurrentActorLocationAsString, *CurrentActorLinearVelocityAsString,
		*CurrentActorAngularVelocityString);

	return PSDActorPhysicsServiceInitializationString;
}
