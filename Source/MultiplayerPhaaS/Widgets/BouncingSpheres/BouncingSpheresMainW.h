// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MultiplayerPhaaS/Widgets/Base/UserWidgetBase.h"
#include "BouncingSpheresMainW.generated.h"

/**
* The bouncing spheres main widget. This is responsible for implementing the 
* interface to show the player its options. This includes starting and stoping
* the simulation. Also, the user may inform the number of bouncing spheres to
* simulate, as well as the server IP address that contains the running physics
* service.
* 
* @see PSDActorsSpawner, PSDActorsCoordinator
*/
UCLASS()
class MULTIPLAYERPHAAS_API UBouncingSpheresMainW : public UUserWidgetBase
{
	GENERATED_BODY()
	
public:
	/** Called when the widget is being constructed. */
	virtual void NativeConstruct() override;

private:
	/** 
	* Start simulation by requesting the PSDACtorsCoordinator. The param should
	* be set as the server ip address that is running the physics service.
	* 
	* @param ServerIpAddress The server ip address that contains the running
	* physics service
	*/
	UFUNCTION(BlueprintCallable)
	void StartPSDActorsSimulation(const FString& ServerIpAddress);

	/** 
	* Stops the current bouncing spheres simulation. This is done by executing
	* a request to the PSDActorsCoordinator.
	*/
	UFUNCTION(BlueprintCallable)
	void StopPSDActorsSimulation();

	/** 
	* Spawns a number of PSDActors on the level. This uses the PSDActors 
	* spawner placed on the level to do so. 
	* 
	* @param NumberOfActorsToSpawn The number of PSD actors to spawn on the
	* level
	* 
	* @see PSDActorsSpawner
	*/
	UFUNCTION(BlueprintCallable)
	void SpawnPSDActors(const int32 NumberOfActorsToSpawn);

	/** Destroy all the PSD actors currently placed on the level */
	UFUNCTION(BlueprintCallable)
	void DestroyAllPSDActors();

	/** 
	* Starts a PSDActors test. This is a 30-seconds test that spawns the 
	* current set number of spheres and automatically starts/stops the 
	* simulation.
	* 
	* @param ServerIpAddress The server ip address that contains the running
	* physics service
	* @param NumberOfActorsToSpawn The number of PSD actors to spawn on the
	* level
	*/
	UFUNCTION(BlueprintCallable)
	void StartPSDActorsTest(const FString& ServerIpAddress, 
		const int32 NumberOfActorsToSpawn);

public:
	/** The StartSimulation button widget */
	UPROPERTY(meta=(BindWidget))
	class UButton* StartSimulationBtn;

	/** The StopSimulation button widget */
	UPROPERTY(meta = (BindWidget))
	class UButton* StopSimulationBtn;

public:
	/** The SpawnPSDActors button widget */
	UPROPERTY(meta = (BindWidget))
	class UButton* SpawnPSDActorsBtn;

	/** The DestroyAllPSDActors button widget */
	UPROPERTY(meta = (BindWidget))
	class UButton* DestroyPSDActorsBtn;

public:
	/** The StartPSDActorsTest button widget */
	UPROPERTY(meta = (BindWidget))
	class UButton* StartPSDActorsTestBtn;

private:
	/** The reference to the PSDActorsCoordinator placed on the map */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	TWeakObjectPtr<class APSDActorsCoordinator> PSDActorCoordinator;

	/** The reference to the PSDActorsSpawner placed on the map */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	TWeakObjectPtr<class APSDActorsSpawner> PSDActorSpawner;
};
