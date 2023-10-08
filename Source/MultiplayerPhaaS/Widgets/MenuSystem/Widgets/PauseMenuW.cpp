// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#include "PauseMenuW.h"
#include "Components/Button.h"

void UPauseMenuW::NativeConstruct()
{
	Super::NativeConstruct();

	// Check if buttons are valid
	check(ContinueGameBtn != nullptr);
	check(MainMenuBtn != nullptr);

	// Bind buttons' clicked event
	ContinueGameBtn->OnClicked.AddDynamic(this,
		&UPauseMenuW::OnContinueGameBtnClicked);
	MainMenuBtn->OnClicked.AddDynamic(this,
		&UPauseMenuW::OnMainMenuBtnClicked);
}

void UPauseMenuW::OnContinueGameBtnClicked()
{
	// When continue game btn is clicked, just destroy this widget
	DestroyWidget();
}

void UPauseMenuW::OnMainMenuBtnClicked()
{
	// Check if interface is null
	check(MainMenuInterface != nullptr);

	// Quit server and load main menu
	MainMenuInterface->QuitServerAndLoadMainMenu();
}
