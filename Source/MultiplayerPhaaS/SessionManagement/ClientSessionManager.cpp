// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ClientSessionManager.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Widgets/MainMenuW.h"

/** The session's name key to create as session settings. */
const static FName SESSION_NAME_KEY = TEXT("SessionName");

void UClientSessionManager::Initialize(UWorld* World)
{
	MPHAAS_LOG_INFO(TEXT("Initializing a client session manager"));

	// Bind the network failure delegate to manage networking errors
	if (GEngine != nullptr)
	{
		GEngine->OnNetworkFailure().AddUObject(this,
			&UClientSessionManager::OnNetworkFailure);
	}

	// Store the world reference
	WorldRef = World;
}

void UClientSessionManager::SetMainMenuInterfaceImplementation
	(class UMainMenuW* MainMenuWidget)
{
	// Check if main menu widget is valid
	check(MainMenuWidget != nullptr);

	// Set the main menu interface to this game instance
	MainMenuWidget->SetMainMenuInterface(this);
}

void UClientSessionManager::JoinServer(const FString& ServerIpAddress)
{
	MPHAAS_LOG_INFO(TEXT("Join server requested for ip: %s"),
		*ServerIpAddress);

	// Get player controller
	const auto PlayerController = WorldRef->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogMultiplayerPhaaS, Error,
			TEXT("No player controller valid when joining server."));
		return;
	}

	// Execute client travel to join the server
	PlayerController->ClientTravel(ServerIpAddress,
		ETravelType::TRAVEL_Absolute);
}

void UClientSessionManager::QuitServerAndLoadMainMenu()
{
	// Check if world reference is valid
	check(WorldRef);

	// Get player controller
	const auto PlayerController = WorldRef->GetFirstPlayerController();
	if (!PlayerController)
	{
		MPHAAS_LOG_ERROR
			(TEXT("No player controller valid when quiting server."));
		return;
	}

	// Execute cleint travel to the main menu
	PlayerController->ClientTravel("/Game/Maps/Map_MainMenu",
		ETravelType::TRAVEL_Absolute);
}

void UClientSessionManager::OnNetworkFailure(UWorld* World,
	UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	MPHAAS_LOG_ERROR(TEXT("Network failure: %s"), *ErrorString);

	// Go back to the main menu
	QuitServerAndLoadMainMenu();
}
