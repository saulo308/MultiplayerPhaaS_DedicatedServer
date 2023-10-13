// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UserWidgetBase.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Interfaces/MainMenuInterface.h"
#include "MenuUserWidgetBase.generated.h"

/**
* The menus widget base. This extends the UUserWidgetBase to implement a
* main menu interface to execute methods.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UMenuUserWidgetBase : public UUserWidgetBase
{
	GENERATED_BODY()

public:
	/**
	* Sets the main menu interface reference.
	*
	* @InMainMenuInterface The main menu interface reference
	*/
	virtual void SetMainMenuInterface
		(IMainMenuInterface* InMainMenuInterface)
		{ MainMenuInterface = InMainMenuInterface; }

protected:
	/**
	* The main menu interface. Implements methods of hosting and joining
	* server transparently to this class.
	*/
	IMainMenuInterface* MainMenuInterface = nullptr;
};
