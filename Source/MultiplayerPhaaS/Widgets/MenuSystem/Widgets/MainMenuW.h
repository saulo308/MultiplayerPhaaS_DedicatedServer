// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MultiplayerPhaaS/Widgets/Base/MenuUserWidgetBase.h"
#include "MainMenuW.generated.h"

/**
* This class implements the main menu. It contains the logic to join servers. 
* Also implements the menu switching to join a existing game server.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UMainMenuW : public UMenuUserWidgetBase
{
	GENERATED_BODY()

protected:
	/** Called when the widget is being constructed. */
	virtual void NativeConstruct() override;

	/**
	* Called when the level that this widget is on is removed from world,
	* a.k.a. when the map changes.
	*/
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
		override;

private:
	/**
	* Called when the "JoinServerBtn" has been clicked. Will get the server
	* ip address from the UTextBlock and join a server.
	*/
	UFUNCTION()
	void OnJoinServerBtnClicked();

	/** Called when the player presses to quit game. Will quit game. */
	UFUNCTION()
	void OnQuitGameClicked();

public:
	/** The "Joinserver" button reference */
	UPROPERTY(meta = (BindWidget))
	class UButton* JoinServerBtn = nullptr;

	/** The quit game button */
	UPROPERTY(meta=(BindWidget))
	class UButton* QuitGameBtn = nullptr;

	/**
	* The editable text box player which player writes the server ip address
	* he wants to connect to.
	*/
	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* ServerIpAddressTextBox = nullptr;

public:
	/** The menu switcher. Used to switch menus. */
	UPROPERTY(meta = (BindWidget))
	class UWidgetSwitcher* MenuSwitcher = nullptr;

	/**
	* The main menu widget. Used to switch to this widget using a
	* widget switcher.
	*/
	UPROPERTY(meta = (BindWidget))
	class UWidget* MainMenuWidget = nullptr;
};
