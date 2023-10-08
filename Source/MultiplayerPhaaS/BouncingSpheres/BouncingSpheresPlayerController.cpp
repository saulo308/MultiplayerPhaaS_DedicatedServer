// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "BouncingSpheresPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Widgets/PauseMenuW.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSGameInstance.h"

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

	// Get the game's game instance
	const auto CurrentGameInstance = 
		Cast<UMultiplayerPhaaSGameInstance>(GetGameInstance());

	// Set the pause menu's interface
	PauseMenuWidget->SetMainMenuInterface(CurrentGameInstance);
}
