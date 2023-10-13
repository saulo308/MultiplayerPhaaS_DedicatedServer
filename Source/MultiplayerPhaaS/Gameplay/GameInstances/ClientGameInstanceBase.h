// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ClientGameInstanceBase.generated.h"

/**
* The client's game instance base. Creates a client session manager object that
* deals with session management, such as finding and joining sessions.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UClientGameInstanceBase : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 
	* Getter to the client session manager pointer.
	* 
	* @return The ClienSessionManager ptr. Or nullptr if not valid.
	*/
	UFUNCTION(BlueprintCallable)
	class UClientSessionManager* GetClientSessionManager();

private:
	/**
	* The client session manager object reference. Deals with the client
	* session methods.
	*/
	TWeakObjectPtr<class UClientSessionManager> ClientSessionManager;
};
