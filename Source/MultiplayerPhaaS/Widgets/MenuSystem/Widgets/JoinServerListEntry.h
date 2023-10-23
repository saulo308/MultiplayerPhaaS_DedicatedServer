// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JoinServerListEntry.generated.h"

/**
* The server list entry. This acts as a widget container for a scroll box entry
* that presents a available server session. Thus, it contains information on
* the session's name, number of players, number of slots, etc.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UJoinServerListEntry : public UUserWidget
{
	GENERATED_BODY()
};
