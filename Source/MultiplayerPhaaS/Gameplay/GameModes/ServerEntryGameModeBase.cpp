// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ServerEntryGameModeBase.h"
#include "MultiplayerPhaaS/Gameplay/GameInstances/ServerGameInstanceBase.h"
#include "MultiplayerPhaaS/SessionManagement/ServerSessionManager.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"

void AServerEntryGameModeBase::StartHostingServerSession()
{
	MPHAAS_LOG_INFO(TEXT("Initializing server session."));

	// Get the server game instance
	const auto ServerGameInstance = 
		CastChecked<UServerGameInstanceBase>(GetGameInstance());

	// Check if the game instance is valid
	check(ServerGameInstance);

	// Get the server session manager reference
	const auto ServerSessionManger = 
		ServerGameInstance->GetServerSessionManager();

	// Check if the pointer is valid
	check(ServerSessionManger);

	// Start hosting a session (as this will be executed on the server's entry)
	ServerSessionManger->HostSession(ServerSessionDefaultName, 
		LobbyWorldMapPath);
}
