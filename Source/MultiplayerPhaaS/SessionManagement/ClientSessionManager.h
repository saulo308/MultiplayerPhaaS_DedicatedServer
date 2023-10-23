// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Interfaces/MainMenuInterface.h"
#include "ClientSessionManager.generated.h"

/**
* The client session manager. Deals with joining servers. This should be a 
* object created for each client on its entry map (such as a main menu).
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
	void JoinServer(const FString& ServerIpAddress);

	/** Quits a server by loading the game's main menu */
	UFUNCTION(Exec)
	void QuitServerAndLoadMainMenu();

private:
	/** Called when a network failure has happened. */
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType, const FString& ErrorString);

private:
	/** World reference to server travel. */
	UWorld* WorldRef = nullptr;
};
