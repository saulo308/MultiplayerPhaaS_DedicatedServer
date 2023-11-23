// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "BouncingSpheresPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Widgets/PauseMenuW.h"
#include "MultiplayerPhaaS/Widgets/BouncingSpheres/BouncingSpheresMainW.h"

#include "MultiplayerPhaaS/Gameplay/GameInstances/ClientGameInstanceBase.h"
#include "MultiplayerPhaaS/SessionManagement/ClientSessionManager.h"
#include "Net/UnrealNetwork.h"

#include "Kismet/GameplayStatics.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorsCoordinator.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Utils/PSDActorsSpawner.h"

#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"

void ABouncingSpheresPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Bind open game pause menu input action
	InputComponent->BindAction("OpenGamePause", IE_Pressed, this,
		&ABouncingSpheresPlayerController::OnPauseKeyPressed);

	// Bind open bouncing spheres menu input action
	InputComponent->BindAction("OpenBouncingSpheresMenu", IE_Pressed, this,
		&ABouncingSpheresPlayerController::OnOpenBouncingSpheresMenu);
}

void ABouncingSpheresPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If on the server and the coordinator is valid:
	if (HasAuthority() && PSDActorCoordinator.Get())
	{
		// Set the flag to know if the bouncing spheres are currently 
		// simulating. This flag will be replicated so clients can use it
		// on their widgets
		bIsBouncingSpheresSimulationActive =
			PSDActorCoordinator.Get()->IsSimulating();
	}
}

void ABouncingSpheresPlayerController::OnPauseKeyPressed()
{
	MPHAAS_LOG_INFO(TEXT("Requested pause menu."));

	// Avoid executing this on the server
	if (HasAuthority())
	{
		return;
	}

	// Check if the pause menu widget class is valid
	check(PauseMenuWidgetClass);

	// Create and show the pause menu on screen
	PauseMenuWidget = CreateWidget<UPauseMenuW>(this, PauseMenuWidgetClass,
		FName("PauseMenuWidget"));

	// Check if widget is valid
	check(PauseMenuWidget);
	PauseMenuWidget->ShowWidget();

	// Get the client game instance
	const auto ClientGameInstance =
		CastChecked<UClientGameInstanceBase>(GetGameInstance());

	// Check if the game instance is valid
	check(ClientGameInstance);

	// Get the client session manager reference
	const auto ClientSessionManger =
		ClientGameInstance->GetClientSessionManager();

	// Check if the pointer is valid
	check(ClientSessionManger);

	// Set the pause menu's interface
	PauseMenuWidget->SetMainMenuInterface(ClientSessionManger);
}

void ABouncingSpheresPlayerController::OnOpenBouncingSpheresMenu()
{
	MPHAAS_LOG_INFO(TEXT("Requested bouncing spheres menu."));

	// Avoid executing this on the server
	if (HasAuthority())
	{
		return;
	}

	// Check if the menu is already valid. If it is, just destroy it (thus,
	// this method works as a flip flop)
	if (BouncingSpheresMainWidget)
	{
		// Destroy the widget
		BouncingSpheresMainWidget->DestroyWidget();
		BouncingSpheresMainWidget = nullptr;

		// Set the input mode to game Only
		FInputModeGameOnly InputModeGameOnly;
		SetInputMode(InputModeGameOnly);

		// Hide mouse cursor
		bShowMouseCursor = false;

		return;
	}

	// Check if the bouncing spheres menu widget class is valid
	check(BouncingSpheresMainWidgetClass);

	// Create and show the pause menu on screen
	BouncingSpheresMainWidget = CreateWidget<UBouncingSpheresMainW>(this,
		BouncingSpheresMainWidgetClass,
		FName("BouncingSpheresMenu"));

	// Check if widget is valid
	check(BouncingSpheresMainWidget);
	BouncingSpheresMainWidget->ShowWidget();

	// Set the input mode to Game and UI
	FInputModeGameAndUI InputModeGameAndUI;
	InputModeGameAndUI.SetWidgetToFocus
		(BouncingSpheresMainWidget->TakeWidget());
	SetInputMode(InputModeGameAndUI);

	// Show mouse cursor
	bShowMouseCursor = true;
}

void ABouncingSpheresPlayerController::GetPSDActorsControllers()
{
	// If PSDActors controllers are already valid, just return
	if (PSDActorCoordinator.Get() && PSDActorSpawner.Get())
	{
		return;
	}

	// Get all actors of APSDActorsCoordinator
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorsCoordinator::StaticClass(), OutActors);

	// Check if there is only one avaialble on the map
	check(OutActors.Num() == 1);

	// Get the PSDActorsCoordinator on the map
	PSDActorCoordinator = Cast<APSDActorsCoordinator>(OutActors[0]);

	// Check if valid
	check(PSDActorCoordinator.Get());

	// Get all actors of APSDActorsSpawner
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorsSpawner::StaticClass(), OutActors);

	// Check if there is only one avaialble on the map
	check(OutActors.Num() == 1);

	// Get the APSDActorsSpawner on the map
	PSDActorSpawner = Cast<APSDActorsSpawner>(OutActors[0]);

	// Check if valid
	check(PSDActorSpawner.Get());
}

void ABouncingSpheresPlayerController::
Server_StartPSDActorsSimulation_Implementation
	(const TArray<FString>& ServerIpAddressesList)
{
	// Get the PSDActors controllers (Coordinator and Spawner)
	GetPSDActorsControllers();

	// Check if the coordinator is valid
	check(PSDActorCoordinator.Get());

	// Request start simulation
	PSDActorCoordinator->StartPSDActorsSimulation(ServerIpAddressesList);
}

void ABouncingSpheresPlayerController::
Server_StopPSDActorsSimulation_Implementation()
{
	// Get the PSDActors controllers (Coordinator and Spawner)
	GetPSDActorsControllers();

	// Check if the coordinator is valid
	check(PSDActorCoordinator.Get());

	// Request stop simulation
	PSDActorCoordinator->StopPSDActorsSimulation();
}

void ABouncingSpheresPlayerController::
Server_SpawnPSDActors_Implementation
	(const int32 NumberOfActorsToSpawn)
{
	// Get the PSDActors controllers (Coordinator and Spawner)
	GetPSDActorsControllers();

	// Check if the spawner is valid
	check(PSDActorSpawner.Get());

	// Request spawner to spawn the number of actors. This is a RPC call to
	// the server
	PSDActorSpawner->SpawnPSDActors(NumberOfActorsToSpawn);
}

void ABouncingSpheresPlayerController::
Server_DestroyAllPSDActors_Implementation()
{
	// Get the PSDActors controllers (Coordinator and Spawner)
	GetPSDActorsControllers();

	// Check if the spawner is valid
	check(PSDActorSpawner.Get());

	// Request spawner to destroy the number of actors. This is a RPC call to
	// the server
	PSDActorSpawner->DestroyPSDActors();
}

void ABouncingSpheresPlayerController::
Server_StartPSDActorsTest_Implementation
	(const TArray<FString>& ServerIpAddressesList, 
	const int32 NumberOfActorsToSpawn)
{
	// Get the PSDActors controllers (Coordinator and Spawner)
	GetPSDActorsControllers();

	// First, destroy all PSD actors on level
	Server_DestroyAllPSDActors();

	// Spawn the number of actors requested
	Server_SpawnPSDActors(NumberOfActorsToSpawn);

	// Check if the coordinator is valid
	check(PSDActorCoordinator.Get());

	// Start the PSD actors test (for 30 seconds)
	PSDActorCoordinator->StartPSDActorsSimulationTest(ServerIpAddressesList, 
		30.f);
}

void ABouncingSpheresPlayerController::Server_SpawnNewPSDSphere_Implementation
	(const FVector SphereLocation)
{
	// Get the PSDActors controllers (Coordinator and Spawner)
	GetPSDActorsControllers();

	// Check if the coordinator is valid
	check(PSDActorCoordinator.Get());

	// Request the PSDActor coordinator to spawn new PSD Sphere at location
	PSDActorCoordinator->SpawnNewPSDSphere(SphereLocation);
}

void ABouncingSpheresPlayerController::GetLifetimeReplicatedProps
	(TArray<FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABouncingSpheresPlayerController,
		bIsBouncingSpheresSimulationActive);
}
