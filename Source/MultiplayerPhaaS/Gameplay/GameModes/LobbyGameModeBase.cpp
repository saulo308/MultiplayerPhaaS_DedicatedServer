// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "LobbyGameModeBase.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"
#include "MultiplayerPhaaS/Gameplay/GameInstances/ServerGameInstanceBase.h"
#include "MultiplayerPhaaS/SessionManagement/ServerSessionManager.h"

void ALobbyGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Increase the number of connected player once one has connected
	NumberOfConnectedPlayers++;

	// Check if the number of connected player is enough to start game
	if (NumberOfConnectedPlayers  == NumberOfPlayerToStartGame)
	{
		// Once it is enough, call on may players logged in
		OnMaxPlayersLoggedIn();
	}
}

void ALobbyGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Someone has logout, decrease the number of connected players
	NumberOfConnectedPlayers--;

	// Check if the number of connected player is not enough to start the game
	// and if the timer to start is valid.
	if ((NumberOfConnectedPlayers < NumberOfPlayerToStartGame) && 
		ServerTravelTimerHandle.IsValid())
	{
		// If it's valid, stop it as we can no longer start the game 
		// (not enough player)
		GetWorld()->GetTimerManager().ClearTimer(ServerTravelTimerHandle);
	}
}

void ALobbyGameModeBase::OnMaxPlayersLoggedIn()
{
	MPHAAS_LOG_INFO(TEXT("Max players connected."));

	// Enable seamless travel so player can see a seamless transition to the
	// game world
	bUseSeamlessTravel = true;

	// If the delay has already started, just ignore it.
	// Potentially, we could restart the time here as another player has logged
	// in
	if (ServerTravelTimerHandle.IsValid())
	{
		return;
	}

	MPHAAS_LOG_INFO(TEXT("Starting game."));

	// Start timer to execute the server travel
	GetWorld()->GetTimerManager().SetTimer(ServerTravelTimerHandle, this,
		&ALobbyGameModeBase::StartGame,
		DelayToStartGameInSeconds, false);
}

void ALobbyGameModeBase::StartGame()
{
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

	// Start the current session
	ServerSessionManger->StartCurrentSession();

	// Execute server travel to the actual game world map
	GetWorld()->ServerTravel(GameWorldMapPath);
}
