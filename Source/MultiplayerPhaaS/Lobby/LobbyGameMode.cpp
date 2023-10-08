// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "LobbyGameMode.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogDeclaration.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSGameInstance.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
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

void ALobbyGameMode::Logout(AController* Exiting)
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

void ALobbyGameMode::OnMaxPlayersLoggedIn()
{
	UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Max players connected..."));

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

	UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Starting game..."));

	// Start timer to execute the server travel
	GetWorld()->GetTimerManager().SetTimer(ServerTravelTimerHandle, this,
		&ALobbyGameMode::StartGame, DelayToStartGameInSeconds, false);
}

void ALobbyGameMode::StartGame()
{
	// Get the server's game instance
	const auto& CurrentGameInstance = 
		Cast<UMultiplayerPhaaSGameInstance>(GetGameInstance());

	// Check if game instance is valid
	check(CurrentGameInstance);

	UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Request start session..."));

	// Start the current session on online session interface
	CurrentGameInstance->StartCurrentSession();

	// Execute server travel to the actual game world map
	GetWorld()->ServerTravel("/Game/Maps/Map_GameWorldTest?listen");
}
