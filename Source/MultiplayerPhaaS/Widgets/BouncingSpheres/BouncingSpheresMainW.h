// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MultiplayerPhaaS/Widgets/Base/UserWidgetBase.h"
#include "BouncingSpheresMainW.generated.h"

/**
* The bouncing spheres main widget. This is responsible for implementing the 
* UI to show the player the game's options, such as starting and stoping
* the simulation. Also, the user may inform the number of bouncing spheres to
* simulate, as well as the server IP address that contains the running physics
* service. This all is done through the ABouncingSpheresPlayerController as 
* we need to call a RPC.
* 
* @see PSDActorsSpawner, PSDActorsCoordinator, ABouncingSpheresPlayerController
*/
UCLASS()
class MULTIPLAYERPHAAS_API UBouncingSpheresMainW : public UUserWidgetBase
{
	GENERATED_BODY()
	
public:
	/** Called when the widget is being constructed. */
	virtual void NativeConstruct() override;

private:
	/** 
	* The current player controller for the bouncing spheres game. Used
	* to call the RPC methods related to this widget.
	*/
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class ABouncingSpheresPlayerController* 
		BouncingSpheresPlayerController	= nullptr;
};
