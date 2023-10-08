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

public:
	/** 
	* Sets the main menu interface on the base. Overwritten here, it will
	* bind the OnFindAvailableSessionsComplete delegate to refresh the current 
	* available sessions list.
	* 
	* @InMainMenuInterface The main menu interface reference
	*/
	virtual void SetMainMenuInterface(IMainMenuInterface* InMainMenuInterface) 
		override;

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
	/** Called when the player presses to open the join server menu. */
	UFUNCTION()
	void OnOpenJoinServerMenuBtnClicked();

	/** 
	* Called when the SessionInterface finds the current sessions for player.
	* Will refresh the server list widget to show the found sessions data in
	* a scrollbox.
	* 
	* @param FoundSessionsData The found session's data list
	* 
	* @see FAvailableSessionData
	*/
	UFUNCTION()
	void OnFindSessionsComplete
		(const TArray<struct FAvailableSessionData> FoundSessionsData);

	/**
	* Called when the "HostServerBtn" has been clicked. Will start hosting
	* a server.
	*/
	UFUNCTION()
	void OnHostServerBtnClicked();

	/**
	* Called when the "JoinServerBtn" has been clicked. Will get the server
	* ip address from the UTextBlock and join a server.
	*/
	UFUNCTION()
	void OnJoinServerBtnClicked();

	/**
	* Called when the player presses to go back to main menu from the join
	* server menu.
	*/
	UFUNCTION()
	void OnBackMainMenuBtnClicked();

	/** Called when the player presses to quit game. Will quit game. */
	UFUNCTION()
	void OnQuitGameClicked();

	/** 
	* Called when the player selectes a new server on the server list entry. 
	*/
	UFUNCTION()
	void OnServerListEntrySelected(const uint32 SelectedEntryIndex);

public:
	/** The "OpenJoinServerMenu" button reference. */
	UPROPERTY(meta = (BindWidget))
	class UButton* OpenJoinServerMenuBtn = nullptr;

	/** The "Joinserver" button reference */
	UPROPERTY(meta = (BindWidget))
	class UButton* JoinServerBtn = nullptr;

	/** The "BackToMainMenu" from join server menu button reference. */
	UPROPERTY(meta = (BindWidget))
	class UButton* BackToMainMenuBtn = nullptr;

	/** The quit game button */
	UPROPERTY(meta=(BindWidget))
	class UButton* QuitGameBtn = nullptr;

	/**
	* The avaialable server list scroll box. Shows all the available sessions
	* so player can choose one to play.
	*/
	UPROPERTY(meta = (BindWidget))
	class UScrollBox* ServerListScrollBox = nullptr;

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

	/**
	* The join server menu widget. Used to switch to this widget using a
	* widget switcher.
	*/
	UPROPERTY(meta = (BindWidget))
	class UWidget* JoinServerMenuWidget = nullptr;

private:
	/** 
	* The join server list entry widget class. Used to show the available
	* servers to join on join server menu.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, 
		meta=(AllowPrivateAccess="True"))
	TSubclassOf<class UJoinServerListEntry> JoinServerListEntryClass;

private:
	/** The current select server index on server entry list. */
	TOptional<uint32> SelectedServerListEntryIndex;
};
