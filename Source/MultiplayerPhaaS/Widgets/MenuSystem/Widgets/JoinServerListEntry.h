// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JoinServerListEntry.generated.h"

/** 
* Delegate to broadcast that this server list entry has been selected. As 
* param, will broadcast the selected entry index on the server list.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerListEntrySelected, uint32,
	SelectedEntryIndex);

/**
* The server list entry. This acts as a widget container for a scroll box entry
* that presents a available server session. Thus, it contains information on
* the session's name, number of players, number of slots, etc.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UJoinServerListEntry : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** 
	* Sets the "BIsEntrySelected" flag.
	* 
	* @param BInIsSelected The new value for the flag.
	*/
	UFUNCTION(BlueprintCallable)
	void SetIsSelected(bool BInIsSelected) 
		{ BIsEntrySelected = BInIsSelected; }

	/**
	* Sets the "BIsEntryHovered" flag.
	*
	* @param BInIsHovered The new value for the flag.
	*/
	UFUNCTION(BlueprintCallable)
	void SetIsHovered(bool BInIsHovered)
		{ BIsEntryHovered = BInIsHovered; }

	/**
	* Getter that informs if this server list entry is currently selected.
	*
	* @return True if this entry is the currently selected entry on the server
	* list.
	*/
	UFUNCTION(BlueprintPure)
	bool IsEntrySelected()
		{ return BIsEntrySelected; }

	/**
	* Getter that informs if this server list entry is currently hovered.
	*
	* @return True if this entry is the currently hovered by player's mouse
	*/
	UFUNCTION(BlueprintPure)
	bool IsEntryHovered() 
		{ return BIsEntryHovered; }

public:
	/** 
	* Sets the server entry data on this server list entry 
	* 
	* @param SessionData The session's data to set on this entry
	* 
	* @see FAvailableSessionData
	*/
	void SetServerEntryData(const struct FAvailableSessionData& SessionData);

	/** 
	* Sets this server list entry index. This will be used to find the 
	* selected entry index on the server list.
	* 
	* @param InServerListEntryIndex This server entry's index on the available
	* server list
	*/
	inline void SetServerListEntryIndex(const uint32 InServerListEntryIndex)
		{ ServerListEntryIndex = InServerListEntryIndex; }

protected:
	/** Called when the widget is being constructed. */
	virtual void NativeConstruct() override;

private:
	/** 
	* Called when this entry is clicked by the player, promoting it as the
	* selected server list entry.
	*/
	UFUNCTION()
	void OnSeverEntryClicked();

public:
	/** 
	* This server list entry's button. This will be used to know when player
	* has selected this server list entry by clicking on it.
	*/
	UPROPERTY(meta=(BindWidget))
	class UButton* ServerEntryListButton = nullptr;

	/** The server's session name represented by this list entry. */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* SessionNameTextBlock = nullptr;

	/** The server's session name represented by this list entry. */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* SessionHostUsernameTextBlock = nullptr;

	/** The server's session name represented by this list entry. */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* SessionConnectionFractionTextBlock = nullptr;

public:
	/** Called when this entry is selected on the servers list. */
	FOnServerListEntrySelected OnServerListEntrySelected;

private:
	/** This server list entry index on the server list */
	TOptional<uint32 >ServerListEntryIndex;

private:
	/** Flag to indicate if this sever list entry is currently selected. */
	bool BIsEntrySelected = false;

	/** 
	* Flag to indicate if this sever list entry is currently hovered by mouse. 
	*/
	bool BIsEntryHovered = false;
};
