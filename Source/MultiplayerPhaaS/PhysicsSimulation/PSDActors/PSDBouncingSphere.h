// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorBase.h"
#include "PSDBouncingSphere.generated.h"

/**
* The PSDActor that represents a bouncing sphere. This PSDActor should be 
* dynamic and movable; Thus, will be simulated and updated on each physics
* step.
*/
UCLASS()
class MULTIPLAYERPHAAS_API APSDBouncingSphere : public APSDActorBase
{
	GENERATED_BODY()

public:
	/** Default constructor */
	APSDBouncingSphere();

public:
	/**
	* Returns the physics service initialization string. This will return
	* a string according to the initialization message template:
	*
	* "ActorTypeString; BodyID; InitialPosX; InitialPosY; InitialPosY\n"
	*
	* Overwritten here, will return the ActorTypeString as "sphere"
	*
	* @return The physics service initialization string for this PSDActor
	*/
	virtual FString GetPhysicsServiceInitializationString() override;
};