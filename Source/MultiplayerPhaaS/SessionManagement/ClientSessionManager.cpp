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

	// Get the online subsystem reference
	OnlineSubsystem = IOnlineSubsystem::Get();
	check(OnlineSubsystem);

	// Log the found subsystem name
	MPHAAS_LOG_INFO(TEXT("Found \"%s\" OnlineSubsystem."),
		*OnlineSubsystem->GetSubsystemName().ToString());

	// Get the online session interface
	OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
	check(OnlineSessionInterface);

	// Bind to on find sessions complete to know when the requested find 
	// sessions has finished processing
	OnlineSessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this,
		&UClientSessionManager::OnFindSessionsComplete);

	// Bind to on JoinSession complete to know when the player has successfully
	// joined a session
	OnlineSessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this,
		&UClientSessionManager::OnJoinSessionComplete);

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

void UClientSessionManager::JoinServer(const uint32& ServerListEntryIndex)
{
	MPHAAS_LOG_INFO
		(TEXT("Trying to join server with index \"%d\" on server list."),
		ServerListEntryIndex);

	// Check if the online search shared ptr is valid
	if (!OnlineSessionSearchSharedPtr)
	{
		MPHAAS_LOG_ERROR
			(TEXT("Unable to join server with index \"%d\" as session search shared ptr is null"),
			ServerListEntryIndex);
		return;
	}

	// Get the search results
	const auto& OnlineSessionSearchResults =
		OnlineSessionSearchSharedPtr->SearchResults;

	// Check if index is valid
	if (!OnlineSessionSearchResults.IsValidIndex(ServerListEntryIndex))
	{
		MPHAAS_LOG_ERROR
			(TEXT("Unable to join server with index \"%d\" as index is not valid"),
			ServerListEntryIndex);
		return;
	}

	// Get the desired session to join
	const auto& DesiredSessionToJoin =
		OnlineSessionSearchResults[ServerListEntryIndex];

	// Get the session name
	FString DesiredSessionName = FString();
	DesiredSessionToJoin.Session.SessionSettings.Get(SESSION_NAME_KEY,
		DesiredSessionName);

	// Check if online session intarface is valid
	check(OnlineSessionInterface);

	MPHAAS_LOG_INFO(TEXT("Joining session with name: %s"),
		*DesiredSessionName);

	// Join desired session
	OnlineSessionInterface->JoinSession(0, FName(*DesiredSessionName),
		DesiredSessionToJoin);
}

void UClientSessionManager::FindAvaialableSessions()
{
	MPHAAS_LOG_INFO(TEXT("Finding online sessions..."));

	// Create a new online session search shared ptr
	OnlineSessionSearchSharedPtr = MakeShareable(new FOnlineSessionSearch());

	// Check if it is valid
	if (!OnlineSessionSearchSharedPtr)
	{
		MPHAAS_LOG_ERROR
			(TEXT("Could not find online sessions as online session search ptr is null."));
		return;
	}

	// Set the number of servers to find
	OnlineSessionSearchSharedPtr->MaxSearchResults = 100;

	// Set search presence to true to find steam servers
	OnlineSessionSearchSharedPtr->QuerySettings.Set(SEARCH_PRESENCE, true,
		EOnlineComparisonOp::Equals);

	// Request online session interface to find sessions
	OnlineSessionInterface->FindSessions(0,
		OnlineSessionSearchSharedPtr.ToSharedRef());
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

void UClientSessionManager::OnFindSessionsComplete(bool BSuccess)
{
	MPHAAS_LOG_INFO
		(TEXT("Find online sessions has returned with \"%d\" flag."), 
		BSuccess);

	// Check if find session was successful
	if (!BSuccess)
	{
		// If not, just log an error
		MPHAAS_LOG_ERROR(TEXT("Find session has failed."));
		return;
	}

	// Get the online sessions found
	const auto& FoundSessionsResult =
		OnlineSessionSearchSharedPtr->SearchResults;

	// For each found session, create the FAvaialableSessionData from it and
	// return the list o data
	TArray<FAvailableSessionData> FoundSessionsData;
	for (const auto& FoundSession : FoundSessionsResult)
	{
		// Create the available session data
		FAvailableSessionData NewFoundSessionData;

		// Get the session name using session name key
		FString FoundSessionName = FString();
		FoundSession.Session.SessionSettings.Get(SESSION_NAME_KEY,
			FoundSessionName);

		// Set the session name
		NewFoundSessionData.SessionName = FoundSessionName;

		// Set the host username
		NewFoundSessionData.SessionHostUsername =
			FoundSession.Session.OwningUserName;

		// Set the number of max players
		NewFoundSessionData.MaxNumberOfPlayers =
			FoundSession.Session.SessionSettings.NumPublicConnections;

		// Set the number of players connected to the session. This is 
		// calculated by the "MaxNumberOfPlayer" - "NumberOfOpenConnections"
		NewFoundSessionData.CurrentNumberOfConnectedPlayers =
			NewFoundSessionData.MaxNumberOfPlayers -
			FoundSession.Session.NumOpenPublicConnections;

		// Add session data do list
		FoundSessionsData.Add(NewFoundSessionData);
	}

	// Broadcast the found session name list
	OnFindAvailableSessionsComplete.Broadcast(FoundSessionsData);
}

void UClientSessionManager::OnJoinSessionComplete
(FName JoinedSessionName,
	EOnJoinSessionCompleteResult::Type JoinSessionCompleteResultType)
{
	// Check if join was sucessful
	if (JoinSessionCompleteResultType != EOnJoinSessionCompleteResult::Success)
	{
		MPHAAS_LOG_WARNING
			(TEXT("Could not join session as result type is not success."));
		return;
	}

	// The connect info to connect to the joined session
	FString ConnectInfo = FString();

	// Get the session's connect string to execute a client travel
	OnlineSessionInterface->GetResolvedConnectString(JoinedSessionName,
		ConnectInfo);

	MPHAAS_LOG_WARNING(TEXT("Joining session: %s"), *ConnectInfo);

	// Check if world reference is valid
	check(WorldRef);

	// Get player controller
	const auto PlayerController = WorldRef->GetFirstPlayerController();
	if (!PlayerController)
	{
		MPHAAS_LOG_ERROR
			(TEXT("No player controller valid when joining server."));
		return;
	}

	// Execute client travel with the connect info given by online session
	// interface
	PlayerController->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
}

void UClientSessionManager::OnNetworkFailure(UWorld* World,
	UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	MPHAAS_LOG_ERROR(TEXT("Network failure: %s"), *ErrorString);

	// Go back to the main menu
	QuitServerAndLoadMainMenu();
}
