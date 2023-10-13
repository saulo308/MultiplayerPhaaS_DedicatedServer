// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ServerEntryGameModeBase.generated.h"

/**
* The server entry game mode base. Will deal with the funcitonality of hosting
* a new session. This should happen on game's start. For instance, on 
* bouncing spheres game, it will call "StartHostingServerSession()" on the
* level blueprint's BeginPlay event.
*/
UCLASS()
class MULTIPLAYERPHAAS_API AServerEntryGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	/**
	* Will start hosting a new session with the given
	* "ServerSessionDefaultName" name. Once the session is created, will
	* server travel to "LobbyWorldMapPath" to await players connections.
	*/
	UFUNCTION(BlueprintCallable)
	void StartHostingServerSession();

public:
	/**
	* The server's session default name. Change this to create a server
	* session with a different name. (Unreal does not allow to create sessions
	* with same name)
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (Category = "GameSessionConfig"))
	FName ServerSessionDefaultName = "BouncingSpheresServer";

	/**
	* The editor's path to the lobby world map. Will be used as the lobby map.
	* Override this if using another lobby map.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		meta = (Category = "GameSessionConfig"))
	FString LobbyWorldMapPath = 
		"/Game/Maps/BouncingSpheres/Map_BouncingSpheres_Lobby";
};
