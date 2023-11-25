// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "MainMenuW.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Overlay.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"

void UMainMenuW::NativeConstruct()
{
	Super::NativeConstruct();

	// Check if buttons are valid
	//check(OpenJoinServerMenuBtn != nullptr);
	check(JoinServerBtn != nullptr);

	// Bind join server menu buttons (join server and back to main menu)
	JoinServerBtn->OnClicked.AddDynamic(this,
		&UMainMenuW::OnJoinServerBtnClicked);

	// Bind quit game btn
	QuitGameBtn->OnClicked.AddDynamic(this,
		&UMainMenuW::OnQuitGameClicked);
}

void UMainMenuW::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);

	// Remove the widget as the menu is not needed anymore
	DestroyWidget();
}

void UMainMenuW::OnJoinServerBtnClicked()
{
	// Check if the editable text box is null
	check(ServerIpAddressTextBox != nullptr);

	// Get the server ip address from the editable text box
	const FString ServerIpAddr = ServerIpAddressTextBox->GetText().ToString();

	// Request join server. TODO: pass the server ip as parameter
	MainMenuInterface->JoinServer(ServerIpAddr);
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
