// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PhysicsSimulation/Base/PSDActorBase.h"
#include "PSDFloor.generated.h"

/**
* The PSDActor that represents a floor. This PSDActor should be static, and
* therefore, not updated on each physics service step.
*/
UCLASS()
class REMOTEPHYSICSENGINESYSTEM_API APSDFloor : public APSDActorBase
{
	GENERATED_BODY()

public:
	/** Default constructor */
	APSDFloor();

public:
	/** 
	* Returns the physics service initialization string. This will return
	* a string according to the initialization message template:
	* 
	* "ActorTypeString; BodyID; InitialPosX; InitialPosY; InitialPosY\n"
	* 
	* Overwritten here, will return the ActorTypeString as "floor"
	* 
	* @return The physics service initialization string for this PSDActor
	*/
	virtual FString GetPhysicsServiceInitializationString() override;
};
