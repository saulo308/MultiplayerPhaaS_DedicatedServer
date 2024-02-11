// Fill out your copyright notice in the Description page of Project Settings.


#include "LocalPhysicsEngineSystem/Public/JoltPhysicsSystem/MyContactListener.h"

ValidateResult FMyContactListener::OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult)
{
	// UE_LOG(LogTemp, Warning, TEXT("Contact validate callback"));

	// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
	return ValidateResult::AcceptAllContactsForThisBodyPair;
}

void FMyContactListener::OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings)
{
	// UE_LOG(LogTemp, Warning, TEXT("A contact was added"));
}

void FMyContactListener::OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings)
{
	// UE_LOG(LogTemp, Warning, TEXT("A contact was persisted"));
}

void FMyContactListener::OnContactRemoved(const SubShapeIDPair& inSubShapePair)
{
	// UE_LOG(LogTemp, Warning, TEXT("A contact was removed"));
}
