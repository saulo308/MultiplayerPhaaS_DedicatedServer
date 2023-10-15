// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PSDActorBase.generated.h"

/**
* The base class for all Physics-Service-Drive (PSD) actors. This actor should
* have its physics simulation driven by the physics service, and therefore, it
* only get its Transform update according to the last simulation coming from
* the PSDActorsCoordinator - which centralizes the physics simulation.
* 
* @see PSDActorsCoordinator.
*/
UCLASS()
class MULTIPLAYERPHAAS_API APSDActorBase : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Sets default values for this actor's properties */
	APSDActorBase();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

public:
	/** 
	* Returns this actor's current location as a string with ";" delimiters.
	* This should be used to initialize the physics world on the physics
	* service server. Each PSD actor will have a initial position equal to
	* the engine's starting position.
	* 
	* @return This actor's current lcoation as a string delimited by ";"
	*/
	FString GetCurrentActorLocationAsString();

public:
	/**
	* Updates this actor's position. This should be called by the 
	* PSDActorsCoordinator once the new physics update comes from the physics
	* service.
	* 
	* @param NewActorPosition The new actor position on the current physics
	* world state
	*/
	void UpdatePositionAfterPhysicsSimulation(const FVector NewActorPosition);

	/**
	* Updates this actor's rotation. This should be called by the
	* PSDActorsCoordinator once the new physics update comes from the physics
	* service.
	*
	* @param NewActorRotationEulerAngles The new actor rotation in euler angles
	* on the current physics world state.
	*/
	void UpdateRotationAfterPhysicsSimulation
		(const FVector NewActorRotationEulerAngles);

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

public:
	/** This actor's root component. Serve as the actor's pivot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* ActorRootComponent = nullptr;

	/** This actor's mesh component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStaticMeshComponent* ActorMeshComponent = nullptr;
};