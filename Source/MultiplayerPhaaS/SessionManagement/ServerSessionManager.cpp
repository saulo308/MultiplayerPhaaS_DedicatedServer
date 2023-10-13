// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ServerSessionManager.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"

/** The session's name key to create as session settings. */
const static FName SESSION_NAME_KEY = TEXT("SessionName");

void UServerSessionManager::Initialize(UWorld* World)
{
	MPHAAS_LOG_INFO(TEXT("Initializing a server session manager"));

	// Get the online subsystem reference
	OnlineSubsystem = IOnlineSubsystem::Get();
	check(OnlineSubsystem);

	// Log the found subsystem name
	MPHAAS_LOG_INFO(TEXT("Found \"%s\" OnlineSubsystem."),
		*OnlineSubsystem->GetSubsystemName().ToString());

	// Get the online session interface
	OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
	check(OnlineSessionInterface);

	// Bind to on create session complete to know when the session creation
	// has finished processing
	OnlineSessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this,
		&UServerSessionManager::OnCreateSessionComplete);

	// Bind to on destroy session complete to know when the requested session
	// destroy has finished processing
	OnlineSessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this,
		&UServerSessionManager::OnDestroySessionComplete);

	// Store the world reference
	WorldRef = World;
}

void UServerSessionManager::HostSession(const FName& HostSessionName,
	const FString& LobbyMapEditorPathToLoadOnSessionCreated)
{
	// Execute a GetNamedSession() to check if the session has already been
	// created
	const auto ExistingSession =
		OnlineSessionInterface->GetNamedSession(HostSessionName);

	// If session already exists, destroy it before creating a new session
	// TODO: This should be replaced to a feedback requesting user to inform
	// another session name
	if (ExistingSession != nullptr)
	{
		MPHAAS_LOG_INFO(TEXT("Destroying \"%s\" session"),
			*HostSessionName.ToString());
		OnlineSessionInterface->DestroySession(HostSessionName);
		return;
	}

	// Store the Lobby map editor path to load it when session is created
	LobbyMapEditorPath = LobbyMapEditorPathToLoadOnSessionCreated;

	// Request the session creation
	CreateNewSession(HostSessionName);
}

void UServerSessionManager::CreateNewSession(const FName& NewSessionName)
{
	MPHAAS_LOG_INFO
	(TEXT("Session with name: \"%s\" creation requested."),
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

	// Store the chosen session name
	CurrentHostedSessionName = NewSessionName;
}

void UServerSessionManager::StartCurrentSession()
{
	// Check if the session interface is valid
	check(OnlineSessionInterface);

	// Start the online session created by this server
	OnlineSessionInterface->StartSession(CurrentHostedSessionName);
}

void UServerSessionManager::OnCreateSessionComplete
	(FName CreatedSessionName, bool BSuccess)
{
	MPHAAS_LOG_INFO
		(TEXT("Session \"%s\" creation completed with \"%d\" success flag."),
		*CreatedSessionName.ToString(), BSuccess);

	// Check if creation was successful
	if (!BSuccess)
	{
		// If not, just log an error
		MPHAAS_LOG_ERROR(TEXT("Session creation has failed."));

		return;
	}

	// Check if world reference is valid
	check(WorldRef);

	// Execute server travel to lobby map
	WorldRef->ServerTravel(LobbyMapEditorPath);
}

void UServerSessionManager::OnDestroySessionComplete
	(FName DestroyedSessionName, bool BSuccess)
{
	MPHAAS_LOG_INFO
	(TEXT("Session \"%s\" destroy completed with \"%d\" success flag."),
		*DestroyedSessionName.ToString(), BSuccess);

	// Check if destroy was successful
	if (!BSuccess)
	{
		// If not, just log an error
		MPHAAS_LOG_ERROR(TEXT("Session destroy has failed."));
		return;
	}

	// Create the session once the last has been destroyed
	CreateNewSession(DestroyedSessionName);
}
