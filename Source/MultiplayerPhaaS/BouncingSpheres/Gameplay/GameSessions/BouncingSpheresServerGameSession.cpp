// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "BouncingSpheresServerGameSession.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogDeclaration.h"

/** The session's name key to create as session settings. */
const static FName SESSION_NAME_KEY = TEXT("SessionName");

void ABouncingSpheresServerGameSession::RegisterServer()
{
	Super::RegisterServer();

	UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Registering server!"));

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
		&ABouncingSpheresServerGameSession::OnCreateSessionComplete);

	// Bind to on destroy session complete to know when the requested session
	// destroy has finished processing
	OnlineSessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this,
		&ABouncingSpheresServerGameSession::OnDestroySessionComplete);

	// Starting hosting a new session
	HostSession("BouncingSpheresServer");
}

void ABouncingSpheresServerGameSession::HostSession
	(const FName& HostSessionName)
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
		UE_LOG(LogMultiplayerPhaaS, Log, TEXT("Destroying \"%s\" session"),
			*HostSessionName.ToString());
		OnlineSessionInterface->DestroySession(HostSessionName);
		return;
	}

	// Creating session
	CreateNewSession(HostSessionName);
}

void ABouncingSpheresServerGameSession::CreateNewSession
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

void ABouncingSpheresServerGameSession::OnCreateSessionComplete
	(FName CreatedSessionName,	bool BSuccess)
{
	UE_LOG(LogMultiplayerPhaaS, Log,
		TEXT("Session \"%s\" creation completed with \"%d\" success flag."),
		*CreatedSessionName.ToString(), BSuccess);

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
	World->ServerTravel("/Game/Maps/Map_MainLobby");
}

void ABouncingSpheresServerGameSession::OnDestroySessionComplete
	(FName DestroyedSessionName, bool BSuccess)
{
	UE_LOG(LogMultiplayerPhaaS, Log,
		TEXT("Session \"%s\" destroy completed with \"%d\" success flag."),
		*DestroyedSessionName.ToString(), BSuccess);

	// Check if destroy was successful
	if (!BSuccess)
	{
		// If not, just log an error
		UE_LOG(LogMultiplayerPhaaS, Error,
			TEXT("Session destroy has failed."));
		return;
	}

	// Create the session once the last has been destroyed
	CreateNewSession(DestroyedSessionName);
}
