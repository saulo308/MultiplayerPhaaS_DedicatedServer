// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MainMenuInterface.generated.h"

/**
* The struct that stores data on found available session. This will store the
* session's name, the host username (who created it), the current number of
* connected players and the max number of players.
*/
USTRUCT()
struct FAvailableSessionData
{
	GENERATED_BODY()

public:
	/** The session's name */
	FString SessionName = FString();

	/** The session's host username (who created it) */
	FString SessionHostUsername = FString();

	/** The number of connected players at the moment on the session. */
	uint32 CurrentNumberOfConnectedPlayers = 0;

	/** The max number of players the session can have. */
	uint32 MaxNumberOfPlayers = 0;
};

/** 
* Delegate that broadcasts when the FindSessions() on SessionInterface has
* completed. As a parameter, will broadcast the found sessions' names.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindAvailableSessionsComplete,
	TArray<FAvailableSessionData>, FoundSessionsData);

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMainMenuInterface : public UInterface
{
	GENERATED_BODY()
};

/**
* The main menu interface. The main menu system calls methods to host and 
* join servers. By inverting the dependency, we transform this call in 
* interfaces, so anyone can implement it transparently to the menu system.
*/
class MULTIPLAYERPHAAS_API IMainMenuInterface
{
	GENERATED_BODY()

public:
	/** 
	* Hosts a game session. Turns the client into a hosting server. 
	* 
	* @param SessionName The desired session name to host
	*/
	virtual void HostSession(const FName& SessionName) = 0;

	/** 
	* Joins a existing server. The param should be the index of the server
	* on the available server list entry.
	* 
	* @param ServerListEntryIndex The server entry index on the server list to 
	* join session
	*/
	virtual void JoinServer(const uint32& ServerListEntryIndex) = 0;

	/** 
	* Finds the available sessions to player. May be used to show player a 
	* available sessions list
	*/
	virtual void FindAvaialableSessions() = 0;

	/** Quits a server by loading the game's main menu */
	virtual void QuitServerAndLoadMainMenu() = 0;

public:
	/** 
	* Delgate that should be fired when the FindSessions() on SessionInterface
	* completes.
	*/
	FOnFindAvailableSessionsComplete OnFindAvailableSessionsComplete;
};
