// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "JoinServerListEntry.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "MultiplayerPhaas/MultiplayerPhaaSLogging.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Interfaces/MainMenuInterface.h"

void UJoinServerListEntry::NativeConstruct()
{
	Super::NativeConstruct();

	// Check if this entry's main button is valid
	check(ServerEntryListButton);

	// Bind the button clicked event to know when player has selected this
	// entry
	ServerEntryListButton->OnPressed.AddDynamic(this,
		&UJoinServerListEntry::OnSeverEntryClicked);
}

void UJoinServerListEntry::SetServerEntryData
	(const FAvailableSessionData& SessionData)
{
	// Check if the session name text block is valid
	check(SessionNameTextBlock);

	// Set the session name on the text block
	SessionNameTextBlock->SetText(FText::FromString(SessionData.SessionName));

	// Check if the SessionHostUsername text block is valid
	check(SessionHostUsernameTextBlock);

	// Set the host username on the text block
	SessionHostUsernameTextBlock->SetText
		(FText::FromString(SessionData.SessionHostUsername));

	// Check if the SessionConnectionFractionTextBlock is valid
	check(SessionConnectionFractionTextBlock);

	// Format the session connection fraction
	const FString FormattedFraction = FString::Printf(TEXT("%d/%d"), 
		SessionData.CurrentNumberOfConnectedPlayers, 
		SessionData.MaxNumberOfPlayers);

	// Set the fraction text block text
	SessionConnectionFractionTextBlock->SetText
		(FText::FromString(FormattedFraction));
}

void UJoinServerListEntry::OnSeverEntryClicked()
{
	// If this server list entry index is not set, something went wrong
	if (!ServerListEntryIndex.IsSet())
	{
		MPHAAS_LOG_WARNING
			(TEXT("Could not selected server list entry as the entry index has not been set."));
		return;
	}

	// Broadcast this button index informing that it has been selected
	OnServerListEntrySelected.Broadcast(ServerListEntryIndex.GetValue());
}
