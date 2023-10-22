// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "MainMenuW.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Overlay.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "JoinServerListEntry.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"

void UMainMenuW::NativeConstruct()
{
	Super::NativeConstruct();

	// Check if buttons are valid
	//check(OpenJoinServerMenuBtn != nullptr);
	check(JoinServerBtn != nullptr);
	check(BackToMainMenuBtn != nullptr);

	// Bind main menu buttons (open host server menu and open join server menu)
	//OpenJoinServerMenuBtn->OnClicked.AddDynamic(this,
		//&UMainMenuW::OnOpenJoinServerMenuBtnClicked);

	// Bind join server menu buttons (join server and back to main menu)
	JoinServerBtn->OnClicked.AddDynamic(this,
		&UMainMenuW::OnJoinServerBtnClicked);
	BackToMainMenuBtn->OnClicked.AddDynamic(this,
		&UMainMenuW::OnBackMainMenuBtnClicked);

	// Bind quit game btn
	QuitGameBtn->OnClicked.AddDynamic(this,
		&UMainMenuW::OnQuitGameClicked);
}

void UMainMenuW::SetMainMenuInterface(IMainMenuInterface* InMainMenuInterface)
{
	Super::SetMainMenuInterface(InMainMenuInterface);

	// Bind the OnFindAvailableSessionsComplete delegate to refresh the 
	// session list
	MainMenuInterface->OnFindAvailableSessionsComplete.AddDynamic(this,
		&UMainMenuW::OnFindSessionsComplete);
}

void UMainMenuW::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);

	// Remove the widget as the menu is not needed anymore
	DestroyWidget();
}

void UMainMenuW::OnOpenJoinServerMenuBtnClicked()
{
	// Check if widget switcher is valid 
	check(MenuSwitcher != nullptr);

	// Check if JoinServerMenuWidget widget is valid
	check(JoinServerMenuWidget != nullptr);

	// Set the JoinServerMenuWidget widget as the active widget on the 
	// widget switcher
	MenuSwitcher->SetActiveWidget(JoinServerMenuWidget);

	// Check if menu interface is valid
	check(MainMenuInterface);

	// Request the menu interface to find the current available sessions
	MainMenuInterface->FindAvaialableSessions();
}

void UMainMenuW::OnJoinServerBtnClicked()
{
	/*
	// Check if interface is null
	check(MainMenuInterface != nullptr);

	// Join server passing the selected server list entry index
	MainMenuInterface->JoinServer(SelectedServerListEntryIndex.GetValue());
	*/

	// Check if the editable text box is null
	check(ServerIpAddressTextBox != nullptr);

	// Get the server ip address from the editable text box
	const FString ServerIpAddr = ServerIpAddressTextBox->GetText().ToString();

	// Request join server. TODO: pass the server ip as parameter
	MainMenuInterface->JoinServer(ServerIpAddr);
}

void UMainMenuW::OnBackMainMenuBtnClicked()
{
	// Check if widget switcher is valid 
	check(MenuSwitcher != nullptr);

	// Check if MainMenuWidget widget is valid
	check(MainMenuWidget != nullptr);

	// Set the main menu widget as the active widget on the widget switcher
	MenuSwitcher->SetActiveWidget(MainMenuWidget);
}

void UMainMenuW::OnFindSessionsComplete
	(const TArray<FAvailableSessionData> FoundSessionsData)
{
	// Check if the join server list entry class is valid before creating it
	check(JoinServerListEntryClass);

	// Check if server list scroll box is valid
	check(ServerListScrollBox);

	// Clear the server list to avoid keeping state from previous refresh
	ServerListScrollBox->ClearChildren();

	// The server list entry counter
	uint32 ServerListEntryCounter = 0;

	// For each found session, create a widget and add it to the server list
	for (const auto& SessionData : FoundSessionsData)
	{
		// Create the new server list entry widget
		const auto NewServerListEntry =
			CreateWidget<UJoinServerListEntry>(this, JoinServerListEntryClass);

		// Set the new server list entry index
		NewServerListEntry->SetServerListEntryIndex(ServerListEntryCounter);

		// Increase the server list entry counter so next entry will receive
		// the next index
		ServerListEntryCounter++;

		// Set the new entry's session data
		NewServerListEntry->SetServerEntryData(SessionData);

		// Bind on server list entry selected so we know when the player 
		// selects a new server entry
		NewServerListEntry->OnServerListEntrySelected.AddDynamic(this,
			&UMainMenuW::OnServerListEntrySelected);

		// Add entry to server list scroll box
		ServerListScrollBox->AddChild(NewServerListEntry);
	}
}

void UMainMenuW::OnQuitGameClicked()
{
	// Get world reference
	const auto World = GetWorld();
	check(World);

	// Get player controller
	const auto PlayerController = World->GetFirstPlayerController();

	// Check if player controller is valid
	check(PlayerController);

	// Execute console command to quit game
	PlayerController->ConsoleCommand("quit");
}

void UMainMenuW::OnServerListEntrySelected(const uint32 SelectedEntryIndex)
{
	MPHAAS_LOG_INFO(TEXT("Selected server list entry with index: %d"), 
		SelectedEntryIndex);

	// Get all the entries on the scroll box (on the server list)
	const auto& ServerListEntries = ServerListScrollBox->GetAllChildren();

	// Get the current selected server list entry (if set)
	if (SelectedServerListEntryIndex.IsSet())
	{
		// Check if the entry exists in server list
		if (ServerListEntries.IsValidIndex
			(SelectedServerListEntryIndex.GetValue()))
		{
			// Get the selected entry
			const auto& SelectedEntry = 
				Cast<UJoinServerListEntry>
				(ServerListEntries[SelectedServerListEntryIndex.GetValue()]);

			// Set the entry as deselected
			SelectedEntry->SetIsSelected(false);
		}
	}

	// Set the selected server index
	SelectedServerListEntryIndex = SelectedEntryIndex;

	// Get the new selected entry
	const auto& SelectedEntry =
		Cast<UJoinServerListEntry>
		(ServerListEntries[SelectedEntryIndex]);

	// Set the entry as selected
	SelectedEntry->SetIsSelected(true);
}
