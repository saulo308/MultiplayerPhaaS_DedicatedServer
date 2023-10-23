// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MainMenuInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMainMenuInterface : public UInterface
{
	GENERATED_BODY()
};

/**
* The main menu interface. The main menu system calls methods to join servers. 
* By inverting the dependency, we transform this call in interfaces, so anyone 
* can implement it transparently to the menu system.
*/
class MULTIPLAYERPHAAS_API IMainMenuInterface
{
	GENERATED_BODY()

public:
	/**
	* Joins a existing server. The param should be the server ip address to
	* connect to.
	*
	* @param ServerListEntryIndex The server ip address to connect to.
	*/
	virtual void JoinServer(const FString& ServerIpAddress) = 0;

	/** Quits a server by loading the game's main menu */
	virtual void QuitServerAndLoadMainMenu() = 0;
};
