// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Widgets/MenuSystem/Interfaces/MainMenuInterface.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerPhaaSGameInstance.generated.h"

/**
* The game's main game instance. Deals with the logic of hosting and joining
* servers.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UMultiplayerPhaaSGameInstance : 
	public UGameInstance, public IMainMenuInterface
{
	GENERATED_BODY()
	
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

public:
	/** The init method. Called to start up the game instance. */
	virtual void Init() override;

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

private:
	/** 
	* The OnlineSessionSearch shared pointer. Needed to find the online
	* sessions using a online session interface reference. Also used to get the
	* found sessions.
	*/
	TSharedPtr<class FOnlineSessionSearch> OnlineSessionSearchSharedPtr;
};
