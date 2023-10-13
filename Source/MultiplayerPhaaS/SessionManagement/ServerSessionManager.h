// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ServerSessionManager.generated.h"

/**
* The server session manager. Deals with the hosting (creating) and starting
* a new session. This should be a object created for each dedicated server
* on its entry map, as it may initialize the session it will handle.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UServerSessionManager : public UObject
{
	GENERATED_BODY()

public:
	/**  
	* Initializes this session manager. Will get the sesion interface and 
	* bind the delegates. 
	*/
	void Initialize(UWorld* World);

public:
	/**
	* Hosts a new session using ServerTravel which players' may connect to.
	* Thus, turns the player into a hosting server.  As param, will store the 
	* lobby map editor path to load when a new session is created and the 
	* session name to host.
	*
	* @param SessionName The session name to host
	*
	* @param LobbyMapEditorPathToLoadOnSessionCreated The lobby map path on
	* the editor. Will load this map when creating the session
	*/
	void HostSession(const FName& HostSessionName,
		const FString& LobbyMapEditorPathToLoadOnSessionCreated);

	/**
	* Starts the current session. As this is only called on the server,
	* the "CurrentHostedSessionName" will be set once the server creates the
	* current session.
	*/
	void StartCurrentSession();

private:
	/**
	* Request the creation of a new session using the online session
	* interface.
	*
	* @param NewSessionName The new session name
	*/
	void CreateNewSession(const FName& NewSessionName);

private:
	/**
	* Called when the online session interface create session has finished
	* processing. This will execute a server travel to the lobby map if the
	* session creation was sucessful.
	*
	* @param SessionName The created session name
	* @param BSuccess Flag that indicates if the session creation was
	* sucessful.
	*/
	void OnCreateSessionComplete(FName CreatedSessionName, bool BSuccess);

	/**
	* Called when the online session interface destroy session has finished
	* processing. This will request a session creation with the given session
	* name.
	*
	* @note TODO: This method should not create a new session with the given
	* name.
	*
	* @param SessionName The destroyed session name
	* @param BSuccess Flag that indicates if the session destroy was sucessful.
	*/
	void OnDestroySessionComplete(FName DestroyedSessionName, bool BSuccess);

private:
	/** The IOnlineSubsystem reference */
	class IOnlineSubsystem* OnlineSubsystem = nullptr;

	/** The OnlineSubsystem's session interface */
	IOnlineSessionPtr OnlineSessionInterface;

	/** World reference to server travel. */
	UWorld* WorldRef = nullptr;

private:
	/** The current hosted session name. */
	FName CurrentHostedSessionName = FName();

	/**
	* The editor's path to the lobby world map. Will be used as the lobby map.
	* Override this if using another lobby map.
	*/
	FString LobbyMapEditorPath = FString();
};
