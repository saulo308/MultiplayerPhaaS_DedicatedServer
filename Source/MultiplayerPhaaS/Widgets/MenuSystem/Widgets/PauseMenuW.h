// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MultiplayerPhaaS/Widgets/Base/MenuUserWidgetBase.h"
#include "PauseMenuW.generated.h"

/**
* The pause menu widget. This will implement the buttons to stop the game, 
* continue it or to go back to the game's main menu (leaving the current 
* connected server).
*/
UCLASS()
class MULTIPLAYERPHAAS_API UPauseMenuW : public UMenuUserWidgetBase
{
	GENERATED_BODY()

protected:
	/** Called when the widget is being constructed. */
	virtual void NativeConstruct() override;

private:
	/** 
	* Called when the "ContinueGameBtn" button is pressed. Will just remove
	* this widget from the viewport and resume game.
	*/
	UFUNCTION()
	void OnContinueGameBtnClicked();

	/** 
	* Called when the "MainMenuBtn" button is pressed. Will leave the current 
	* connected server and return to the game's main menu.
	*/
	UFUNCTION()
	void OnMainMenuBtnClicked();

public:
	/** The "Continue" game button. */
	UPROPERTY(meta=(BindWidget))
	class UButton* ContinueGameBtn = nullptr;

	/** The "MainMenu" game button. */
	UPROPERTY(meta=(BindWidget))
	class UButton* MainMenuBtn = nullptr;
};
