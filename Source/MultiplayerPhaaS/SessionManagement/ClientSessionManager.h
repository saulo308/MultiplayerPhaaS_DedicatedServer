// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Interfaces/MainMenuInterface.h"
#include "ClientSessionManager.generated.h"

/**
* The client session manager. Deals with the finding and joining online 
* sessions. This should be a object created for each client on its entry map 
* (such as a main menu).
*/
UCLASS()
class MULTIPLAYERPHAAS_API UClientSessionManager : public UObject, 
	public IMainMenuInterface
{
	GENERATED_BODY()

public:
	/**
	* Initializes this session manager. Will get the online session subsystem 
	* and interface.
	*/
	void Initialize(UWorld* World);

public:
	/**
	* Sets the main menu interface implementation to this instance of game
	* instance (as it implements the interface)
	*
	* @param MainMenuWidget The main menu widget reference to set the reference
	*/
	UFUNCTION(BlueprintCallable)
	void SetMainMenuInterfaceImplementation(class UMainMenuW* MainMenuWidget);

public:
	/**
	* Joins a server using ClientTravel. Passes the server's ip address as
	* parameter
	*
	* @param ServerIp The server's ip to connect to
	*/
	UFUNCTION(Exec)
	void JoinServer(const uint32& ServerListEntryIndex);

	/** Quits a server by loading the game's main menu */
	UFUNCTION(Exec)
	void QuitServerAndLoadMainMenu();

private:
	/**
	* Executes a FindSessions() on SessionInterface. The OnFindSessionsComplete
	* callback will broadcast a delegate with the found sessions' data. Can be
	* used to refresh the available online sessions list shown to player.
	*/
	void FindAvaialableSessions();

private:
	/**
	* Called when the online session interface find session has finished
	* processing. Moreover, broadcasts the delegate to inform the found online
	* sessions data.
	*
	* @param BSuccess Flag that indicates if the find session was sucessful.
	*/
	void OnFindSessionsComplete(bool BSuccess);

	/**
	* Called when the player JoinSession called on session interface complets
	* its processing. Will execute a client travel if succesfully joined a
	* session.
	*
	* @param JoinedSessionName The joined session name
	* @param JoinSessionCompleteResultType The join session complete result
	* type
	*
	* @see EOnJoinSessionCompleteResult
	*/
	void OnJoinSessionComplete(FName JoinedSessionName,
		EOnJoinSessionCompleteResult::Type JoinSessionCompleteResultType);

	/** Called when a network failure has happened. */
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType, const FString& ErrorString);

private:
	/** The IOnlineSubsystem reference */
	class IOnlineSubsystem* OnlineSubsystem = nullptr;

	/** The OnlineSubsystem's session interface */
	IOnlineSessionPtr OnlineSessionInterface;

	/** World reference to server travel. */
	UWorld* WorldRef = nullptr;

private:
	/**
	* The OnlineSessionSearch shared pointer. Needed to find the online
	* sessions using a online session interface reference. Also used to get the
	* found sessions.
	*/
	TSharedPtr<class FOnlineSessionSearch> OnlineSessionSearchSharedPtr;
};
