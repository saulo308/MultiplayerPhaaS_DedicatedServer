// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerPhaaSGameInstance.h"
#include "MultiplayerPhaaSLogDeclaration.h"
#include "OnlineSessionSettings.h"
#include "Widgets/MenuSystem/Widgets/MainMenuW.h"

const static FName SESSION_NAME_KEY = TEXT("SessionName");

void UMultiplayerPhaaSGameInstance::Init()
{
	Super::Init();

	// Get the online subsystem reference
	OnlineSubsystem = IOnlineSubsystem::Get();
	check(OnlineSubsystem);

	// Log the found subsystem name
	UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Found \"%s\" OnlineSubsystem."),
		*OnlineSubsystem->GetSubsystemName().ToString());

	// Get the online session interface
	OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
	check(OnlineSessionInterface);

	// Bind to on create session complete to know when the session creation
	// has finished processing
	OnlineSessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this,
		&UMultiplayerPhaaSGameInstance::OnCreateSessionComplete);

	// Bind to on destroy session complete to know when the requested session
	// destroy has finished processing
	OnlineSessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this,
		&UMultiplayerPhaaSGameInstance::OnDestroySessionComplete);

	// Bind to on find sessions complete to know when the requested find 
	// sessions has finished processing
	OnlineSessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this,
		&UMultiplayerPhaaSGameInstance::OnFindSessionsComplete);

	// Bind to on JoinSession complete to know when the player has successfully
	// joined a session
	OnlineSessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this,
		&UMultiplayerPhaaSGameInstance::OnJoinSessionComplete);

	// Bind the network failure delegate to manage networking errors
	if (GEngine != nullptr)
	{
		GEngine->OnNetworkFailure().AddUObject(this, 
			&UMultiplayerPhaaSGameInstance::OnNetworkFailure);
	}
}

void UMultiplayerPhaaSGameInstance::SetMainMenuInterfaceImplementation
	(class UMainMenuW* MainMenuWidget)
{
	// Check if main menu widget is valid
	check(MainMenuWidget != nullptr);

	// Set the main menu interface to this game instance
	MainMenuWidget->SetMainMenuInterface(this);
}

void UMultiplayerPhaaSGameInstance::HostSession(const FName& SessionName)
{
	// Execute a GetNamedSession() to check if the session has already been
	// created
	const auto ExistingSession = 
		OnlineSessionInterface->GetNamedSession(SessionName);

	// If session already exists, destroy it before creating a new session
	// TODO: This should be replaced to a feedback requesting user to inform
	// another session name
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Destroying \"%s\" session"), 
			*SessionName.ToString());
		OnlineSessionInterface->DestroySession(SessionName);
		return;
	}

	// Creating session
	CreateNewSession(SessionName);
}

void UMultiplayerPhaaSGameInstance::JoinServer
	(const uint32& ServerListEntryIndex)
{
	UE_LOG(LogMultiplayerPhaaS, Log, 
		TEXT("Trying to join server with index \"%d\" on server list."), 
		ServerListEntryIndex);

	// Check if the online search shared ptr is valid
	if (!OnlineSessionSearchSharedPtr)
	{
		UE_LOG(LogMultiplayerPhaaS, Error,
			TEXT("Unable to join server with index \"%d\" as session search shared ptr is null"),
			ServerListEntryIndex);
		return;
	}

	// Get the search results
	const auto& OnlineSessionSearchResults = 
		OnlineSessionSearchSharedPtr->SearchResults;

	// Check if index is valid
	if (!OnlineSessionSearchResults.IsValidIndex(ServerListEntryIndex))
	{
		UE_LOG(LogMultiplayerPhaaS, Error, 
			TEXT("Unable to join server with index \"%d\" as index is not valid"), 
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

	// Store the chosen session name
	CurrentHostedSessionName = FName(*DesiredSessionName);

	// Check if online session intarface is valid
	check(OnlineSessionInterface);

	UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Joining session with name: %s"),
		*DesiredSessionName);

	// Join desired session
	OnlineSessionInterface->JoinSession(0, CurrentHostedSessionName,
		DesiredSessionToJoin);
}

void UMultiplayerPhaaSGameInstance::StartCurrentSession()
{
	// Check if the session interface is valid
	check(OnlineSessionInterface);

	// Start the online session created by this server
	OnlineSessionInterface->StartSession(CurrentHostedSessionName);
}

void UMultiplayerPhaaSGameInstance::FindAvaialableSessions()
{
	UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Finding online sessions..."));

	// Create a new online session search shared ptr
	OnlineSessionSearchSharedPtr = MakeShareable(new FOnlineSessionSearch());

	// Check if it is valid
	if (!OnlineSessionSearchSharedPtr)
	{
		UE_LOG(LogMultiplayerPhaaS, Error,
			TEXT("Could not find online sessions as online session search ptr is null."));
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

void UMultiplayerPhaaSGameInstance::QuitServerAndLoadMainMenu()
{
	// Get engine reference
	const auto Engine = GetEngine();
	if (!Engine)
	{
		return;
	}

	// Display messge on screen
	Engine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
		FString::Printf(TEXT("Quiting server and loading main menu")));

	// Get player controller
	const auto PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogMultiplayerPhaaS, Error,
			TEXT("No player controller valid when quiting server."));
		return;
	}

	// Execute cleint travel to the main menu
	PlayerController->ClientTravel("/Game/Maps/Map_MainMenu", 
		ETravelType::TRAVEL_Absolute);
}

void UMultiplayerPhaaSGameInstance::CreateNewSession
	(const FName& NewSessionName)
{
	UE_LOG(LogMultiplayerPhaaS, Log, 
		TEXT("Session with name: \"%s\" creation requested."),
		*NewSessionName.ToString());

	// Create the new sesion settings
	FOnlineSessionSettings SessionSettings;

	// If using NULL subsystem, set the LAN match to true and false otherwise
	SessionSettings.bIsLANMatch = 
		(IOnlineSubsystem::Get()->GetSubsystemName() == "NULL") ? true : false;

	// Enable presence to use steam servers
	SessionSettings.bUsesPresence = true;

	// Enable to use steam lobbies
	SessionSettings.bUseLobbiesIfAvailable = true;

	// Set the number of players the session can have
	SessionSettings.NumPublicConnections = 5;

	// Set that this sessions should be "public". I.e., anyone can find it 
	// through a "FindSessions()" call
	SessionSettings.bShouldAdvertise = true;

	// Set the session name using Set() with the session name key
	SessionSettings.Set(SESSION_NAME_KEY, NewSessionName.ToString(),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// Creating session
	OnlineSessionInterface->CreateSession(0, NewSessionName, SessionSettings);
}

void UMultiplayerPhaaSGameInstance::OnCreateSessionComplete(FName SessionName,
	bool BSuccess)
{
	UE_LOG(LogMultiplayerPhaaS, Log,
		TEXT("Session \"%s\" creation completed with \"%d\" success flag."),
		*SessionName.ToString(), BSuccess);

	// Check if creation was successful
	if (!BSuccess)
	{
		// If not, just log an error
		UE_LOG(LogMultiplayerPhaaS, Error, 
			TEXT("Session creation has failed."));

		return;
	}

	// Get world reference
	const auto World = GetWorld();
	if (!World)
	{
		UE_LOG(LogMultiplayerPhaaS, Error,
			TEXT("No world reference valid when hosting server."));
		return;
	}

	// Execute server travel to lobby map
	World->ServerTravel("/Game/Maps/Map_MainLobby?listen");
}

void UMultiplayerPhaaSGameInstance::OnDestroySessionComplete(FName SessionName,
	bool BSuccess)
{
	UE_LOG(LogMultiplayerPhaaS, Log,
		TEXT("Session \"%s\" destroy completed with \"%d\" success flag."),
		*SessionName.ToString(), BSuccess);

	// Check if destroy was successful
	if (!BSuccess)
	{
		// If not, just log an error
		UE_LOG(LogMultiplayerPhaaS, Error, 
			TEXT("Session destroy has failed."));
		return;
	}

	// Create the session once the last has been destroyed
	CreateNewSession(SessionName);
}

void UMultiplayerPhaaSGameInstance::OnFindSessionsComplete(bool BSuccess)
{
	UE_LOG(LogMultiplayerPhaaS, Log,
		TEXT("Find online sessions has returned with \"%d\" flag."), BSuccess);

	// Check if find session was successful
	if (!BSuccess)
	{
		// If not, just log an error
		UE_LOG(LogMultiplayerPhaaS, Error, TEXT("Find session has failed."));
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

void UMultiplayerPhaaSGameInstance::OnJoinSessionComplete
	(FName JoinedSessionName, 
	EOnJoinSessionCompleteResult::Type JoinSessionCompleteResultType)
{
	// Check if join was sucessful
	if (JoinSessionCompleteResultType != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogMultiplayerPhaaS, Warning, 
			TEXT("Could not join session as result type is not success."));
		return;
	}

	// The connect info to connect to the joined session
	FString ConnectInfo = FString();

	// Get the session's connect string to execute a client travel
	OnlineSessionInterface->GetResolvedConnectString(JoinedSessionName, 
		ConnectInfo);

	UE_LOG(LogMultiplayerPhaaS, Warning,
		TEXT("Joining session: %s"), *ConnectInfo);

	// Get player controller
	const auto PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogMultiplayerPhaaS, Error,
			TEXT("No player controller valid when joining server."));
		return;
	}

	// Execute client travel with the connect info given by online session
	// interface
	PlayerController->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
}

void UMultiplayerPhaaSGameInstance::OnNetworkFailure(UWorld* World, 
	UNetDriver* NetDriver, ENetworkFailure::Type FailureType, 
	const FString& ErrorString)
{
	UE_LOG(LogMultiplayerPhaaS, Error, TEXT("Network failure: %s"), 
		*ErrorString);

	// Go back to the main menu
	QuitServerAndLoadMainMenu();
}