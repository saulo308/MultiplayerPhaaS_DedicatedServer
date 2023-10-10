// // 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "BouncingSpheresServerGameSession.generated.h"

/**
* The server's game session. Will create a session upon dedicated server 
* registration. When session is created, will server travel to the main lobby
* to await players connections.
*/
UCLASS()
class MULTIPLAYERPHAAS_API ABouncingSpheresServerGameSession : public AGameSession
{
	GENERATED_BODY()
	
public:
	/** 
	* Allow a dedicated server a chance to register itself with an online 
	* service. Will starting hosting a new session, creating it using session
	* interface.
	*/
	virtual void RegisterServer() override;

private:
	/**
	* Hosts a new session using ServerTravel which players' may connect to.
	* Thus, turns the player into a hosting server.
	*
	* @param SessionName The session name to host
	*/
	void HostSession(const FName& HostSessionName);

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
};
