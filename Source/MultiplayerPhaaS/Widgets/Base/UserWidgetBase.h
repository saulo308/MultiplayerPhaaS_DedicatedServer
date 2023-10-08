// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UserWidgetBase.generated.h"

/**
* The user widget base class. This will extend the UUserWdiget class to 
* implement more functionalities. For instance, it implements the ShowWidget()
* and DestroyWidget() that changes the current player input mode and adds or
* remove this current widget from the viewport.
*/
UCLASS()
class MULTIPLAYERPHAAS_API UUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** 
	* Shows the widget. Adds it to viewport and change the input mode to 
	* UI only.
	*/
	UFUNCTION(BlueprintCallable)
	virtual void ShowWidget();

	/**
	* Destroys the widget. Removes it from the viewport and change the input
	* mode to game only.
	*/
	UFUNCTION(BlueprintCallable)
	virtual void DestroyWidget();
};
