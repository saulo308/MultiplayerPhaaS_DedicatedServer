// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "UserWidgetBase.h"

void UUserWidgetBase::ShowWidget()
{
	// Get world reference
	const auto World = GetWorld();
	check(World != nullptr);

	// Get player controller reference
	const auto PlayerController = World->GetFirstPlayerController();
	check(PlayerController != nullptr);

	// Set widget to focusable
	bIsFocusable = true;

	// Change input mode to UI only
	FInputModeUIOnly NewInputMode;
	NewInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	NewInputMode.SetWidgetToFocus(TakeWidget());
	PlayerController->SetInputMode(NewInputMode);

	// Show mouse cursor
	PlayerController->bShowMouseCursor = true;

	// Add widget to viewport
	AddToViewport();
}

void UUserWidgetBase::DestroyWidget()
{
	// Get world reference
	const auto World = GetWorld();
	check(World != nullptr);

	// Get player controller reference
	const auto PlayerController = World->GetFirstPlayerController();
	check(PlayerController != nullptr);

	// Change input mode to game only and hide cursor as now player will start
	// playing the game
	FInputModeGameOnly NewInputMode;
	PlayerController->SetInputMode(NewInputMode);
	PlayerController->bShowMouseCursor = false;

	// Remove this widget from the viewport as the menu is not needed anymore
	RemoveFromViewport();
}
