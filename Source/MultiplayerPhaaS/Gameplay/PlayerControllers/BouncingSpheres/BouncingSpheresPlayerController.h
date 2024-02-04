// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BouncingSpheresPlayerController.generated.h"

/**
* The boucing spheres game player controller. This implements the input to 
* pause the menu during game's runtime.
*/
UCLASS()
class MULTIPLAYERPHAAS_API ABouncingSpheresPlayerController : 
	public APlayerController
{
	GENERATED_BODY()

public:
	/** 
	* Returns true if the bouncing spheres is currently simulating. This may
	* be used to update the widget on the client disable the simulation buttons
	* 
	* @return True if the bouncing spheres simulation is active. False 
	* otherwise
	*/
	UFUNCTION(BlueprintCallable)
	bool IsBouncingSpheresSimulationActive() const
		{ return bIsBouncingSpheresSimulationActive; }

public:
	/**
	* Start simulation by requesting the PSDActorsCoordinator. The param should
	* be set as the server ip addresses that are running physics services. The
	* workload should be distributed on this set of physics services.
	*
	* @param ServerIpAddressesList The server ip addresses list that are 
	* running physics services
	*/
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartPSDActorsSimulation
		(const TArray<FString>& ServerIpAddressesList);

	/**
	* Stops the current bouncing spheres simulation. This is done by executing
	* a request to the PSDActorsCoordinator.
	*/
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StopPSDActorsSimulation();

	/**
	* Spawns a number of PSDActors on the level. This uses the PSDActors
	* spawner placed on the level to do so.
	*
	* @param NumberOfActorsToSpawn The number of PSD actors to spawn on the
	* level
	*
	* @see PSDActorsSpawner
	*/
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SpawnPSDActors(const int32 NumberOfActorsToSpawn);

	/** Destroy all the PSD actors currently placed on the level */
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_DestroyAllPSDActors();

	/**
	* Starts a PSDActors test. This is a 30-seconds test that spawns the
	* current set number of spheres and automatically starts/stops the
	* simulation.
	*
	* @param ServerIpAddressesList The server ip addresses list that are 
	* running physics services
	* @param NumberOfActorsToSpawn The number of PSD actors to spawn on the
	* level
	*/
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartPSDActorsTest
		(const TArray<FString>& ServerIpAddressesList,
		const int32 NumberOfActorsToSpawn);

	/** 
	* Spawns a new PSDSphere both on the game and on the physics service.
	* 
	* @param SphereLocation The new PSD Sphere initial location to spawn
	*/
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SpawnNewPSDSphere(const FVector SphereLocation);

	/** */
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_ResetMap();

	/** */
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_LoadMap(const FString& NewMap);

public:
	/** Called once every fame. */
	virtual void Tick(float DeltaTime) override;

protected:
	/** Setups the player input component. */
	virtual void SetupInputComponent() override;

private:
	/** 
	* Called when the pause key has been presed. Will show the pause menu
	* widget.
	*/
	UFUNCTION()
	void OnPauseKeyPressed();

	/**
	* Called when the open bouncing spheres menu widget key has been presed.
	* Will show the main bouncing spheres menu widget.
	*/
	UFUNCTION()
	void OnOpenBouncingSpheresMenu();

private:
	/** 
	* Get the PSDActors controllers on the map. Will get both the 
	* APSDActorsCoordinator and APSDActorsSpanwer.
	*/
	void GetPSDActorsControllers();

public:
	/** The pause menu widget class to create */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class UPauseMenuW> PauseMenuWidgetClass;

	/** The bouncing spheres menu widget class to create */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class UBouncingSpheresMainW> BouncingSpheresMainWidgetClass;

private:
	/** The created pause menu widget reference */
	class UPauseMenuW* PauseMenuWidget = nullptr;

	/** The created pause bouncing spheres widget widget reference */
	class UBouncingSpheresMainW* BouncingSpheresMainWidget = nullptr;

private:
	/**
	* Flag that indicates if the bouncing spheres simulation is currently 
	* active
	*/
	UPROPERTY(Replicated)
	bool bIsBouncingSpheresSimulationActive = false;

private:
	/** The reference to the PSDActorsCoordinator placed on the map */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	TWeakObjectPtr<class APSDActorsCoordinator> PSDActorCoordinator;

	/** The reference to the PSDActorsSpawner placed on the map */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	TWeakObjectPtr<class APSDActorsSpawner> PSDActorSpawner;
};
