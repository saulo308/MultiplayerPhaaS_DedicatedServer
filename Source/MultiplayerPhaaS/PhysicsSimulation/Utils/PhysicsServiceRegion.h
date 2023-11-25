// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicsServiceRegion.generated.h"

/** 
* This is the physics service region. This represents the simulating area of a
* given physics service. If any PSDActor is whithin this area, it will be 
* simulated by the owning physics service of this region. If the PSDActor
* leaves this region, it will no longer be on the responsibility of this
* physics service's region owner (thus, will no longer be simulated by it).
*/
UCLASS()
class MULTIPLAYERPHAAS_API APhysicsServiceRegion : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Sets default values for this actor's properties */ 
	APhysicsServiceRegion();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;
	
private:
	/** 
	* The physics service region actor root component. Only used to easily
	* place the region on the world
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, 
		meta=(AllowPrivateAccess="true"))
	USceneComponent* RegionRootComponent = nullptr;

	/** 
	* The box component that collides with PSDActors. This represents the
	* physics service region itself. If any PSDActor is whithin this area, it 
	* will be simulated by the owning physics service of this region. If the 
	* PSDActor leaves this region, it will no longer be on the responsibility 
	* of this physics service's region owner (thus, will no longer be simulated
	* by it).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* PhysicsServiceRegionBoxComponent = nullptr;
};
