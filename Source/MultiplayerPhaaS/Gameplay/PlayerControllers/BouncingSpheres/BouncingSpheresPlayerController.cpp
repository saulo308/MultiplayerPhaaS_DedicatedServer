// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "BouncingSpheresPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerPhaaS/Widgets/MenuSystem/Widgets/PauseMenuW.h"
#include "MultiplayerPhaaS/Gameplay/GameInstances/ClientGameInstanceBase.h"
#include "MultiplayerPhaaS/SessionManagement/ClientSessionManager.h"

#include "Net/UnrealNetwork.h"

#include "Kismet/GameplayStatics.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Base/PSDActorsCoordinator.h"
#include "MultiplayerPhaaS/PhysicsSimulation/Utils/PSDActorsSpawner.h"

void ABouncingSpheresPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("OpenGamePause", IE_Pressed, this,
		&ABouncingSpheresPlayerController::OnPauseKeyPressed);
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
	// Avoid executing this on the server
	if (HasAuthority())
	{
		return;
	}

	// Check if the pause menu widget class is valid
	check(PauseMenuWidgetClass != nullptr);

	// Create and show the pause menu on screen
	PauseMenuWidget = CreateWidget<UPauseMenuW>(this, PauseMenuWidgetClass,
		FName("PauseMenuWidget"));
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
	(const FString& ServerIpAddress)
{
	// Get the PSDActors controllers (Coordinator and Spawner)
	GetPSDActorsControllers();

	// Check if the coordinator is valid
	check(PSDActorCoordinator.Get());

	// Request start simulation
	PSDActorCoordinator->StartPSDActorsSimulation(ServerIpAddress);
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
	(const FString& ServerIpAddress, const int32 NumberOfActorsToSpawn)
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
	PSDActorCoordinator->StartPSDActorsSimulationTest(ServerIpAddress, 30.f);
}

void ABouncingSpheresPlayerController::GetLifetimeReplicatedProps
	(TArray<FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABouncingSpheresPlayerController,
		bIsBouncingSpheresSimulationActive);
}
