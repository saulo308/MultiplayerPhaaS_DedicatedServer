// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PSDActorBase.generated.h"

/** 
* Delegate called once this PSDActor has exited his owning physics service
* region.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorExitedPhysicsRegion, 
	APSDActorBase*, ExitedActor);

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
	/** Getter to the physics service owner id */
	UFUNCTION(BlueprintCallable)
	int32 GetActorOwnerPhysicsServiceId()
		{ return ActorOwnerPhysicsServiceId; }

	/** Setter to the physics service owner id */
	UFUNCTION(BlueprintCallable)
	void SetActorOwnerPhysicsServiceId(const int32 NewOwnerPhysicsServiceId)
		{ ActorOwnerPhysicsServiceId = NewOwnerPhysicsServiceId; }

public:	
	/** Sets default values for this actor's properties */
	APSDActorBase();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

public:
	/**
	* Returns the physics service initialization string. This should return
	* a string according to the initialization message template:
	*
	* "ActorTypeString; BodyID; InitialPosX; InitialPosY; InitialPosY\n"
	*
	* This method should be overwritten by each PSDActor
	*
	* @return The physics service initialization string for this PSDActor
	*/
	virtual FString GetPhysicsServiceInitializationString();
	
	/** 
	* Returns this actor's current location as a string with ";" delimiters.
	* This should be used to initialize the physics world on the physics
	* service server. Each PSD actor will have a initial position equal to
	* the engine's starting position.
	* 
	* @return This actor's current lcoation as a string delimited by ";"
	*/
	virtual FString GetCurrentActorLocationAsString();

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

public:
	/** Called once this PSDActor enters a new physics service region. */
	virtual void OnEnteredNewPhysicsRegion();

	/**
	* Called once this PSDActor has existed his owning physics service
	* region. Will broadcast this event with the delegate.
	*/
	virtual void OnExitedPhysicsRegion();

public:
	/** 
	* Returns the PSDActor body id on the physics service. This should be 
	* used to identify this PSDActor on the physics service. 
	*
	* @note This a unique ID
	* 
	* @return This PSDActor's body unique ID on the physics service
	*/
	uint32 GetPSDActorBodyIdOnPhysicsService() const
		{ return PSDActorBodyIdOnPhysicsService; }

	/** 
	* Returns if this PSDActor is static (should move/updated or not)
	*
	* @return True if this PSDActor is static. False otherwise
	*/
	bool IsPSDActorStatic() const { return bIsPSDActorStatic; }

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

public:
	/** 
	* Called once this actor has exited his current physics region. Useful
	* to know if we should migrate this actor to a new physics service region
	*/
	FOnActorExitedPhysicsRegion OnActorExitedCurrentPhysicsRegion;

protected:
	/** 
	* The owning physics service ID. This represents the physics service that
	* updates this PSDActor.
	*
	* @note This Id will be overwritten if the PSDActor is inside a 
	* PhysicsServiceRegion upon BeginPlay()
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, 
		meta=(AllowPrivateAccess="true"))
	int32 ActorOwnerPhysicsServiceId = 0;

protected:
	/** 
	* The PSDActor's body unique ID on the physics service. Useful to 
	* uniquely identify this PSDActor either on the world or on the physics 
	* service
	*/
	uint32 PSDActorBodyIdOnPhysicsService = 0;

	/** 
	* Flag that indicates if this PSDActor is static. If false, the PSDActor
	* will not be considered on each physics service step and will not be
	* updated on each frame
	*/
	bool bIsPSDActorStatic = false;
};
