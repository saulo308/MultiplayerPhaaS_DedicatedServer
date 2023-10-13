// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ClientGameInstanceBase.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"
#include "MultiplayerPhaaS/SessionManagement/ClientSessionManager.h"

class UClientSessionManager* UClientGameInstanceBase::GetClientSessionManager()
{
	// Check if the server session manager is valid
	if (!ClientSessionManager.IsValid())
	{
		// If not valid, log it
		MPHAAS_LOG_INFO
			(TEXT("UClientSessionManager was not valid on game instance. Creating a new one."));

		// Create the server session manager object
		ClientSessionManager = NewObject<UClientSessionManager>();
		if (!ClientSessionManager.IsValid())
		{
			MPHAAS_LOG_ERROR
				(TEXT("Error while creating client session manager."));
			return nullptr;
		}

		// Initialize the session manager
		ClientSessionManager.Get()->Initialize(GetWorld());
	}

	return ClientSessionManager.Get();
}