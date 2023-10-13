// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "BouncingSpheresPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Widgets/PauseMenuW.h"
#include "MultiplayerPhaaS/Gameplay/GameInstances/ClientGameInstanceBase.h"
#include "MultiplayerPhaaS/SessionManagement/ClientSessionManager.h"

void ABouncingSpheresPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("OpenGamePause", IE_Pressed, this,
		&ABouncingSpheresPlayerController::OnPauseKeyPressed);
}

void ABouncingSpheresPlayerController::OnPauseKeyPressed()
{
	// Check if the pause menu widget class is valid
	check(PauseMenuWidgetClass != nullptr);

	// Create and show the pause menu on screen
	PauseMenuWidget = CreateWidget<UPauseMenuW>(this, PauseMenuWidgetClass,
		FName("PauseMenuWidget"));
	PauseMenuWidget->ShowWidget();

	// Get the client game instance
	const auto ClientGameInstance =
		CastChecked<UClientGameInstanceBase>(GetGameInstance());

	// Check if the game instance is valid
	check(ClientGameInstance);

	// Get the client session manager reference
	const auto ClientSessionManger =
		ClientGameInstance->GetClientSessionManager();

	// Check if the pointer is valid
	check(ClientSessionManger);

	// Set the pause menu's interface
	PauseMenuWidget->SetMainMenuInterface(ClientSessionManger);
}
