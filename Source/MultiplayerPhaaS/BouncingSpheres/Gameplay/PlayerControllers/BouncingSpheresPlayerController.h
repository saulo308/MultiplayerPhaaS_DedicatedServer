// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BouncingSpheresPlayerController.generated.h"

/**
* The boucing spheres game player controller. This implements the input to 
* pause the menu during game's runtime.
*/
UCLASS()
class MULTIPLAYERPHAAS_API ABouncingSpheresPlayerController : 
	public APlayerController
{
	GENERATED_BODY()

protected:
	/** Setups the player input component. */
	virtual void SetupInputComponent() override;

private:
	/** 
	* Called when the pause key has been presed. Will show the pause menu
	* widget.
	*/
	UFUNCTION()
	void OnPauseKeyPressed();

public:
	/** The pause menu widget class to create */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class UMenuUserWidgetBase> PauseMenuWidgetClass;

private:
	/** The created pause menu widget reference */
	UMenuUserWidgetBase* PauseMenuWidget = nullptr;
};
