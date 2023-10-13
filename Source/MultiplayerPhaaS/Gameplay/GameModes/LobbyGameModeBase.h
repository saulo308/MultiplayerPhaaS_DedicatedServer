// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameModeBase.generated.h"

/**
* The lobby map game mode. Will count the number of connected players on the
* lobby and will only start the game if the correct amount of players are
* connected to the lobby.
*/
UCLASS()
class MULTIPLAYERPHAAS_API ALobbyGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	/** 
	* Called when a new player has sucessfully logged in on the lobby. 
	* 
	* @param NewPlayer The player controller from the player that has logged in
	*/
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/**
	* Called when a new player has logged out from the lobby.
	*
	* @param Exiting The controller from the player that has logged out
	*/
	virtual void Logout(AController* Exiting) override;

private:
	/** 
	* Called when the max number of player have logged in on the lobby. This
	* will start a timer to start the game. Thus, new player may log in on this
	* time window.
	*/
	void OnMaxPlayersLoggedIn();

	/** Starts the actual game by executing a server travel. */
	void StartGame();

public:
	/**
	* The editor's path to the game world map. Will be used as the game map.
	* Override this if using another game world.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta=(Category="LobbyConfig"))
	FString GameWorldMapPath = 
		"Game/Maps/BouncingSpheres/Map_BouncingSpheres_GameWorld";

public:
	/** The amount of connected player that has to login to start game */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (Category = "LobbyConfig"))
	int32 NumberOfPlayerToStartGame = 2;

	/** 
	* The delay in seconds to start the game after the necessary amount of 
	* player has logged in.	
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (Category = "LobbyConfig"))
	float DelayToStartGameInSeconds = 5.f;

private:
	/** The current amount of connected player on the lobby. */
	int32 NumberOfConnectedPlayers = 0;

private:
	/** 
	* The current server travel timer handle that will start the game after
	* a delay. Potentially, other players may come in to join the game as well.
	*/
	FTimerHandle ServerTravelTimerHandle;
};
