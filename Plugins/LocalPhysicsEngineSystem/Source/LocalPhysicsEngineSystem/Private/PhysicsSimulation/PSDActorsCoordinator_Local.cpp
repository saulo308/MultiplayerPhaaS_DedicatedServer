// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "PhysicsSimulation/PSDActorsCoordinator_Local.h"
#include "LocalPhysicsEngineSystem/LocalPhysicsEngineSystemLogging.h"
#include "LocalPhysicsEngineSystem/Public/JoltPhysicsSystem/PhysicsServiceImpl.h"
#include "RemotePhysicsEngineSystem/Public/PhysicsSimulation/PSDActors/Base/PSDActorBase.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "GenericPlatform/GenericPlatformMemory.h"

APSDActorsCoordinator_Local::APSDActorsCoordinator_Local()
{
	PrimaryActorTick.bCanEverTick = true;

	// Replication setup...
	bReplicates = true;
	bAlwaysRelevant = true;

}

void APSDActorsCoordinator_Local::BeginPlay()
{
	Super::BeginPlay();
	
}

void APSDActorsCoordinator_Local::EndPlay
	(const EEndPlayReason::Type EndPlayReason)
{
	StopPSDActorsSimulation();
}

void APSDActorsCoordinator_Local::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSimulatingPhysics)
	{
		if (HasAuthority())
		{
			// Check if the simulation timer remaining is less then 15s. If so,
			// measure the CPU and RAM
			if (!bHasMeasuredCpuAndRamForSimulation &&
				(GetWorld()->GetTimerManager().GetTimerRemaining
				(PSDActorsTestTimerHandle) <= 15.f))
			{
				// Set the flag as we will measure the CPU and RAM
				bHasMeasuredCpuAndRamForSimulation = true;

				// Get memory measurement (will muiltcast to clients)
				GetRamMeasurement();
			}
			else
			{
				// If not, just measure the DeltaTime. Here we do this as
				// the CPU and RAM usage will affect the delta time. So we 
				// don't want to measure the DeltaTime during it
				DeltaTimeMeasurement += FString::Printf(TEXT("%f\n"),
					DeltaTime);
			}
		}
		else
		{
			// Measure the delta time on client
			DeltaTimeMeasurement += FString::Printf(TEXT("%f\n"),
				DeltaTime);
		}
	}

	if (bIsSimulatingPhysics && HasAuthority())
	{
		// Update PSD actors by simulating physics on the service
		// and parsing it's results with the new actor position
		UpdatePSDActors();
	}
}

void APSDActorsCoordinator_Local::GetLifetimeReplicatedProps
	(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate simualting physics 
	DOREPLIFETIME(APSDActorsCoordinator_Local, bIsSimulatingPhysics);
}

void APSDActorsCoordinator_Local::InitializePhysicsWorld()
{
	// Start initialization message
	FString InitializationMessage = FString();

	// Foreach PSD actor, get its StepPhysicsString
	for (auto& PSDActor : PSDActorMap)
	{
		InitializationMessage += 
			PSDActor.Value->GetPhysicsServiceInitializationString();
	}

	// Convert message to std string
	std::string InitializationMessageAsStdString
		(TCHAR_TO_UTF8(*InitializationMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* InitializationMessageAsChar = &InitializationMessageAsStdString[0];

	PhysicsServiceLocalImpl->InitPhysicsSystem(InitializationMessage);
}

void APSDActorsCoordinator_Local::UpdatePSDActors()
{
	// Check if we are simulating
	if (!bIsSimulatingPhysics)
	{
		return;
	}

	LPES_LOG_WARNING(TEXT("Stepping: %d"), StepPhysicsCounter++);

	// Step physics
	FString PhysicsSimulationResultStr =
		PhysicsServiceLocalImpl->StepPhysicsSimulation();

	// Parse physics simulation result
	// Each line will contain a result for a actor in terms of:
	// "Id; posX; posY; posZ; rotX; rotY; rotZ"
	TArray<FString> ParsedSimulationResult;
	PhysicsSimulationResultStr.ParseIntoArrayLines(ParsedSimulationResult);

	// Foreach line, parse its results (getting each actor pos)
	for (auto& SimulationResultLine : ParsedSimulationResult)
	{
		// Parse the line with ";" delimit
		TArray<FString> ParsedActorSimulationResult;
		SimulationResultLine.ParseIntoArray(ParsedActorSimulationResult,
			TEXT(";"), false);

		// Check for errors
		if (ParsedActorSimulationResult.Num() < 13)
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not parse line %s. Num is:%d"),
				*SimulationResultLine, ParsedSimulationResult.Num());
			return;
		}

		// Get the actor id to float
		const uint32 ActorID = FCString::Atoi(*ParsedActorSimulationResult[0]);

		// Check if the PSDActor exist with such id on the map
		if (!PSDActorMap.Contains(ActorID))
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not find actor with id %d"),
				ActorID);
			continue;
		}

		// Find the actor on the map
		APSDActorBase* ActorToUpdate = PSDActorMap[ActorID];
		if (!ActorToUpdate)
		{
			UE_LOG(LogTemp, Warning, TEXT("Actor with id %d is not valid."),
				ActorID);
			continue;
		}

		// Update PSD actor linear velocity
		const float NewLinearVelocityX =
			FCString::Atof(*ParsedActorSimulationResult[7]);
		const float NewLinearVelocityY =
			FCString::Atof(*ParsedActorSimulationResult[8]);
		const float NewLinearVelocityZ =
			FCString::Atof(*ParsedActorSimulationResult[9]);
		const FVector NewLinearVelocity(NewLinearVelocityX, NewLinearVelocityY,
			NewLinearVelocityZ);

		ActorToUpdate->SetPSDActorLinearVelocity(NewLinearVelocity);

		// Update PSD actor angular velocity
		const float NewAngularVelocityX =
			FCString::Atof(*ParsedActorSimulationResult[10]);
		const float NewAngularVelocityY =
			FCString::Atof(*ParsedActorSimulationResult[11]);
		const float NewAngularVelocityZ =
			FCString::Atof(*ParsedActorSimulationResult[12]);
		const FVector NewAngularVelocity(NewAngularVelocityX,
			NewAngularVelocityY, NewAngularVelocityZ);

		ActorToUpdate->SetPSDActorAngularVelocity(NewAngularVelocity);

		// Update PSD actor position with the result
		const float NewPosX = FCString::Atof(*ParsedActorSimulationResult[1]);
		const float NewPosY = FCString::Atof(*ParsedActorSimulationResult[2]);
		const float NewPosZ = FCString::Atof(*ParsedActorSimulationResult[3]);
		const FVector NewPos(NewPosX, NewPosY, NewPosZ);

		ActorToUpdate->UpdatePositionAfterPhysicsSimulation(NewPos);

		// Update PSD actor rotation with the result
		const float NewRotX = FCString::Atof(*ParsedActorSimulationResult[4]);
		const float NewRotY = FCString::Atof(*ParsedActorSimulationResult[5]);
		const float NewRotZ = FCString::Atof(*ParsedActorSimulationResult[6]);
		const FVector NewRotEuler(NewRotX, NewRotY, NewRotZ);

		ActorToUpdate->UpdateRotationAfterPhysicsSimulation(NewRotEuler);
	}

	LPES_LOG_INFO(TEXT("Physics updated for this frame."));
}

void APSDActorsCoordinator_Local::StartPSDActorsSimulation
	(const TArray<FString>& SocketServerIpAddrList)
{
	LPES_LOG_WARNING(TEXT("Starting PSD actors simulation."));

	// Get all PSDActors
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorBase::StaticClass(), FoundActors);

	// Foreach found actor, append to PSDActorsList
	for (int i = 0; i < FoundActors.Num(); i++)
	{
		// Cast to PSDActor to get a reference to it
		auto PSDActor = Cast<APSDActorBase>(FoundActors[i]);
		if (!PSDActor)
		{
			continue;
		}

		// Get the PSDActor body id
		const auto PSDActorBodyId = PSDActor->GetPSDActorBodyId();

		// Add to map 
		// The key is the body id on the physics system.
		// The value is the reference to the actor
		PSDActorMap.Add(PSDActorBodyId, PSDActor);
	}

	PhysicsServiceLocalImpl = new FPhysicsServiceImpl();
	if (!PhysicsServiceLocalImpl)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not create physics impl instance"));
		return;
	}

	// Initialize physics world
	InitializePhysicsWorld();

	DeltaTimeMeasurement = FString();

	bIsSimulatingPhysics = true;

	LPES_LOG_WARNING(TEXT("PSD actors started simulating..."));
}

void APSDActorsCoordinator_Local::StartPSDActorsSimulationTest
	(const TArray<FString>& SocketServerIpAddrList,
	float TestDurationInSeconds/*=30.f*/)
{
	bHasMeasuredCpuAndRamForSimulation = false;

	// Start the PSD actors simulation
	StartPSDActorsSimulation(SocketServerIpAddrList);

	// Start timer to stop the simulation after 30 seconds passed
	GetWorld()->GetTimerManager().SetTimer(PSDActorsTestTimerHandle, this,
		&APSDActorsCoordinator_Local::StopPSDActorsSimulation, 
		TestDurationInSeconds, false);
}

void APSDActorsCoordinator_Local::StopPSDActorsSimulation()
{
	if (!bIsSimulatingPhysics)
	{
		return;
	}

	LPES_LOG_INFO(TEXT("Stopping PSD actors simulation."));

	// Set the flag to false to stop ticking PSDActors' update
	bIsSimulatingPhysics = false;

	// Get the step physics time measurements on physics implementation
	StepPhysicsTimeMeasure = PhysicsServiceLocalImpl->GetSimulationMeasures();

	if (HasAuthority())
	{
		// Save measurements
		SaveDeltaTimeMeasurementToFile();
		SaveStepPhysicsTimeMeasureToFile();
		SaveUsedRamMeasurements();
		SaveAllocatedRamMeasurements();
	}

	LPES_LOG_INFO(TEXT("PSD actors simulation has been stopped."));
}

void APSDActorsCoordinator_Local::SaveDeltaTimeMeasurementToFile_Implementation()
	const
{
	FString TargetFolder = TEXT("FPSMeasure");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/%s_Local_%d.txt"), *MapName,
		FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/%s_Local_%d.txt"), *MapName,
			FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	LPES_LOG_WARNING(TEXT("Saving delta time measurement into \"%s\""),
		*FileFullPath);

	FFileHelper::SaveStringToFile(DeltaTimeMeasurement, *FileFullPath);
}

void APSDActorsCoordinator_Local::SaveStepPhysicsTimeMeasureToFile_Implementation()
const
{
	FString TargetFolder = TEXT("StepPhysicsMeasureWithoutCommsOverhead");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/StepPhysicsTime_%s_%d.txt"),
		*MapName, FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/StepPhysicsTime_%s_%d.txt"),
			*MapName, FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	LPES_LOG_WARNING(TEXT("Saving step physics time measurement into \"%s\""),
		*FileFullPath);

	FFileHelper::SaveStringToFile(StepPhysicsTimeMeasure, *FileFullPath);
}

void APSDActorsCoordinator_Local::SaveUsedRamMeasurements_Implementation() const
{
	FString TargetFolder = TEXT("UsedRamMeasurements");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/UsedRam_%s_%d.txt"),
		*MapName, FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/UsedRam_%s_%d.txt"),
			*MapName, FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	LPES_LOG_WARNING(TEXT("Saving used ram measurement into \"%s\""),
		*FileFullPath);

	FFileHelper::SaveStringToFile(UsedRamMeasurement, *FileFullPath);
}

void APSDActorsCoordinator_Local::SaveAllocatedRamMeasurements_Implementation() const
{
	FString TargetFolder = TEXT("AllocatedRamMeasurements");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	// Get the current world
	UWorld* CurrentWorld = GetWorld();

	// Get the current level
	ULevel* CurrentLevel = CurrentWorld->GetCurrentLevel();

	// Get the map name
	FString MapName = CurrentLevel->GetOuter()->GetName();

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/AllocatedRam_%s_%d.txt"),
		*MapName, FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/AllocatedRam_%s_%d.txt"),
			*MapName, FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	LPES_LOG_WARNING(TEXT("Saving allocated ram measurement into \"%s\""),
		*FileFullPath);

	FFileHelper::SaveStringToFile(AllocatedRamMeasurement, *FileFullPath);
}

void APSDActorsCoordinator_Local::GetRamMeasurement_Implementation()
{
	// Get the memory stats
	FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();

	// Get and store the used memory
	uint32 UsedMemoryMB = MemoryStats.UsedPhysical / (1024 * 1024);
	UsedRamMeasurement += FString::Printf(TEXT("%d\n"),
		UsedMemoryMB);

	// Get and store the allocated memory
	uint32 AllocatedMemoryMB =
		MemoryStats.AvailablePhysical / (1024 * 1024);
	AllocatedRamMeasurement += FString::Printf(TEXT("%d\n"),
		AllocatedMemoryMB);
}
