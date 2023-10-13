// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#include "ServerGameInstanceBase.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"
#include "MultiplayerPhaaS/SessionManagement/ServerSessionManager.h"

class UServerSessionManager* UServerGameInstanceBase::GetServerSessionManager()
{
	// Check if the server session manager is valid
	if (!ServerSessionManager.IsValid())
	{
		// If not valid, log it
		MPHAAS_LOG_INFO
			(TEXT("UServerSessionManager was not valid on game instance. Creating a new one."));

		// Create the server session manager object
		ServerSessionManager = NewObject<UServerSessionManager>();
		if (!ServerSessionManager.IsValid())
		{
			MPHAAS_LOG_ERROR
				(TEXT("Error while creating server session manager."));
			return nullptr;
		}

		// Initialize the session manager
		ServerSessionManager.Get()->Initialize(GetWorld());
	}

	return ServerSessionManager.Get();
}
