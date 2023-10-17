// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "BouncingSpheresMainW.h"
#include "MultiplayerPhaaS/Gameplay/PlayerControllers/BouncingSpheres/BouncingSpheresPlayerController.h"

void UBouncingSpheresMainW::NativeConstruct()
{
	// Get the player controller
	const auto PlayerController = GetWorld()->GetFirstPlayerController();

	// Cast to bouncing spheres player controller so we can use it on the
	// blueprint to call RPCs
	BouncingSpheresPlayerController =
		Cast<ABouncingSpheresPlayerController>(PlayerController);
}
