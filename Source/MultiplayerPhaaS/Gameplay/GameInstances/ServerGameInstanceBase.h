// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ServerGameInstanceBase.generated.h"

/**
* The server game instance base. Creates a server session manager object that
* deals with session management, such as hosting and starting sessions.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UServerGameInstanceBase : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 
	* Getter to the server session manager pointer. 
	* 
	* @return The ServerSessionManager pointer. Or nullptr if not valid
	*/
	UFUNCTION(BlueprintCallable)
	class UServerSessionManager* GetServerSessionManager();

private:
	/** 
	* The server session manager object reference. Deals with the server
	* session methods.
	*/
	TWeakObjectPtr<class UServerSessionManager> ServerSessionManager;
};
