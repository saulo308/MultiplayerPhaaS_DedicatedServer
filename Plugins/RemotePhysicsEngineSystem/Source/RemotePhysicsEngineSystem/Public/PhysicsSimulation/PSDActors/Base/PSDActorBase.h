// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PSDActorBase.generated.h"

/** 
* The enum that contains data on the current PSDActor status according to 
* physics regions. Thus, will store if the actor is inside a region, a shared
* region or no region at all.
*/
UENUM(BlueprintType)
enum class EPSDActorPhysicsRegionStatus : uint8
{
	InsideRegion,
	SharedRegion,
	NoRegion
};

/** 
* Called once the PSDActor has entered a physics service region. Will 
* broadcast this PSDActor reference and the physics service region id he has 
* entered.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorEnteredPhysicsRegion,
	APSDActorBase*, EnteredPSDActor, int32, EnteredPhysicsRegionId);

/**
* Called once the PSDActor has exited a physics service region. Will
* broadcast this PSDActor reference and the physics service region id he has
* exited.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorExitedPhysicsRegion,
	APSDActorBase*, ExitedPSDActor, int32, ExitedPhysicsRegionId);

/**
* The base class for all Physics-Service-Drive (PSD) actors. This actor should
* have its physics simulation driven by the physics service, and therefore, it
* only get its Transform update according to the last simulation coming from
* the PSDActorsCoordinator - which centralizes the physics simulation.
* 
* @see PSDActorsCoordinator.
*/
UCLASS()
class REMOTEPHYSICSENGINESYSTEM_API APSDActorBase : public AActor
{
	GENERATED_BODY()
	
public:
	/** Getter to the physics service region owner id */
	UFUNCTION(BlueprintCallable)
	int32 GetActorOwnerPhysicsServiceRegionId() const
		{ return ActorOwnerPhysicsServiceRegionId; }

	/**
	* Getter to the PSDActor's current status on his owning physics service
	* region
	*/
	UFUNCTION(BlueprintCallable)
	EPSDActorPhysicsRegionStatus GetPSDActorPhysicsRegionStatus() const
		{ return CurrentPSDActorPhysicsRegionStatus; }

	/** Setter to the physics service region owner id */
	UFUNCTION(BlueprintCallable)
	void SetActorOwnerPhysicsServiceRegionId
		(const int32 NewOwnerPhysicsServiceRegionId)
		{ ActorOwnerPhysicsServiceRegionId = NewOwnerPhysicsServiceRegionId; }

public:	
	/** Sets default values for this actor's properties */
	APSDActorBase();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

public:
	/** */
	FVector GetPSDActorLinearVelocity() const
		{ return PSDActorLinearVelocity; }

	/** */
	FString GetPSDActorLinearVelocityAsString() const;

	/** */
	FVector GetPSDActorAngularVelocity() const 
		{ return PSDActorAngularVelocity; }

	/** */
	FString GetPSDActorAngularVelocityAsString() const;

	/**
	* Returns this actor's current location as a string with ";" delimiters.
	* This should be used to initialize the physics world on the physics
	* service server. Each PSD actor will have a initial position equal to
	* the engine's starting position.
	*
	* @return This actor's current lcoation as a string delimited by ";"
	*/
	virtual FString GetCurrentActorLocationAsString() const;

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
	* Returns the PSDActor body id on the physics service. This should be
	* used to identify this PSDActor on the physics service.
	*
	* @note This a unique ID
	*
	* @return This PSDActor's body unique ID on the physics service
	*/
	int32 GetPSDActorBodyId() const { return PSDActorBodyId; }

	/**
	* Returns if this PSDActor is static (should move/updated or not)
	*
	* @return True if this PSDActor is static. False otherwise
	*/
	constexpr bool IsPSDActorStatic() const { return bIsPSDActorStatic; }

	/**
	* Called once the PSDActor has entered a physics service region. This is
	* used to broadcast the delegate that this PSDActor has entered a new
	* region.
	*
	* @param EnteredPhysicsRegionId The physics service region's id the
	* PSDActor has just entered
	*/
	void OnEnteredPhysicsRegion(int32 EnteredPhysicsRegionId);

	/**
	* Called once the PSDActor has exited a physics service region. This is
	* used to broadcast the delegate that this PSDActor has exited a new
	* region.
	*
	* @param ExitedPhysicsRegionId The physics service region's id the
	* PSDActor has just exited
	*/
	void OnExitedPhysicsRegion(int32 ExitedPhysicsRegionId);
	
	/**
	* Updates this actor's position. This should be called by the 
	* PSDActorsCoordinator once the new physics update comes from the physics
	* service.
	* 
	* @param NewActorPosition The new actor position on the current physics
	* world state
	*/
	void UpdatePositionAfterPhysicsSimulation(const FVector& NewActorPosition);

	/**
	* Updates this actor's rotation. This should be called by the
	* PSDActorsCoordinator once the new physics update comes from the physics
	* service.
	*
	* @param NewActorRotationEulerAngles The new actor rotation in euler angles
	* on the current physics world state.
	*/
	void UpdateRotationAfterPhysicsSimulation
		(const FVector& NewActorRotationEulerAngles);

	/**
	* Updates this actor's physics region status. 
	* 
	* @param NewPhysicsRegionStatus The PSDActor's new physics region status
	*/
	void UpdatePSDActorStatusOnRegion
		(EPSDActorPhysicsRegionStatus NewPhysicsRegionStatus);

	/** */
	void SetPSDActorAngularVelocity(FVector InPSDActorAngularVelocity)
		{ PSDActorAngularVelocity = InPSDActorAngularVelocity; }

	/** 
	* Sets a new PSDActor body ID. This may be needed when spawning PSDActor
	* clones. The clone should have the same ID as his replica. Thus, after
	* being spawned, we set the PSDActor's body ID to the same as his replica.
	* 
	* @param NewPSDActorBodyId The new PSDActor body ID
	*/
	void SetPSDActorBodyId(int32 NewPSDActorBodyId)
		{ PSDActorBodyId = NewPSDActorBodyId; }

	/** */
	void SetPSDActorLinearVelocity(FVector InPSDActorLinearVelocity) 
		{ PSDActorLinearVelocity = InPSDActorLinearVelocity; }

protected:
	/** 
	* Called once CurrentPSDActorPhysicsRegionStatus is replicated. Thus,
	* once the current status has changed, this method will be called.
	*/
	UFUNCTION()
	void OnRep_PhysicsRegionStatusUpdated();

	/** Called once PSDActorBodyIdOn is replicated */
	UFUNCTION()
	void OnRep_PSDActorBodyId();

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

	/** 
	* The actor's body id text render component. Used to show the current
	* actor's bodyID on the physics service
	*/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//class UTextRenderComponent* ActorBodyIdTextRenderComponent = nullptr;

	/**
	* The actor's physics region text render component. Used to show the 
	* current actor's physics region status
	*/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//class UTextRenderComponent* ActorRegionStatusTextRender = nullptr;

public:
	/**
	* Delegate called once the PSDActor has entered a physics service region. 
	* Will broadcast this PSDActor reference and the physics service region id 
	* he has entered.
	*/
	FOnActorEnteredPhysicsRegion OnActorEnteredPhysicsRegion;

	/**
	* Delegate called once the PSDActor has exited a physics service region.
	* Will broadcast this PSDActor reference and the physics service region id
	* he has exited.
	*/
	FOnActorExitedPhysicsRegion OnActorExitedPhysicsRegion;

protected:
	/** 
	* The owning physics service region id. This represents the physics service 
	* region that updates this PSDActor.
	*
	* @note This id will be set once the PSDActor is inside a 
	* PhysicsServiceRegion upon BeginPlay()
	*/
	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 ActorOwnerPhysicsServiceRegionId = 0;

	/** 
	* The current physics region status of this PSDActor. This will inform if
	* the PSDActor is currently inside a region (a single region), if on a 
	* shared region (inside more that one physics region) or if he has no
	* region at all
	*/
	UPROPERTY(BlueprintReadOnly, 
		ReplicatedUsing = OnRep_PhysicsRegionStatusUpdated)
	EPSDActorPhysicsRegionStatus CurrentPSDActorPhysicsRegionStatus =
		EPSDActorPhysicsRegionStatus::NoRegion;

	/**
	* The PSDActor's body unique ID on the physics service. Useful to
	* uniquely identify this PSDActor either on the world or on the physics
	* service
	*/
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PSDActorBodyId)
	int32 PSDActorBodyId = 0;

protected:
	/**
	* Flag that indicates if this PSDActor is static. If false, the PSDActor
	* will not be considered on each physics service step and will not be
	* updated on each frame
	*/
	bool bIsPSDActorStatic = false;

	/** */
	FVector PSDActorLinearVelocity = FVector();

	/** */
	FVector PSDActorAngularVelocity = FVector();
};
