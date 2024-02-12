// Fill out your copyright notice in the Description page of Project Settings.

#include "LocalPhysicsEngineSystem/Public/JoltPhysicsSystem/PhysicsServiceImpl.h"
#include "LocalPhysicsEngineSystem/LocalPhysicsEngineSystemLogging.h"

FPhysicsServiceImpl::FPhysicsServiceImpl()
{

}

void FPhysicsServiceImpl::InitPhysicsSystem
	(const FString& initializationActorsInfo)
{
	LPES_LOG_INFO(TEXT("Initializing physics system..."));
	LPES_LOG_INFO(TEXT("Init message: %s"), *initializationActorsInfo);

	// If physics system is already initialized, clear the last initialization
	if (bIsInitialized)
	{
		ClearPhysicsSystem();
	}

	// Register allocation hook
	RegisterDefaultAllocator();

	// Install callbacks
	//Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

	// Create a factory
	Factory::sInstance = new Factory();

	// Register all Jolt physics types
	RegisterTypes();

	// We need a temp allocator for temporary allocations during the physics 
	// update. We're pre-allocating 10 * 1024 * 1024 MB to avoid having to do 
	// allocations during the physics update. 
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc 
	// to fall back to malloc/free.
	temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);

	// We need a job system that will execute physics jobs on multiple threads. 
	// Typically you would implement the JobSystem interface yourself and let 
	// Jolt Physics run on top of your own job scheduler. JobSystemThreadPool 
	// is an example implementation.
	job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers,
		thread::hardware_concurrency() - 1);

	// This is the max amount of rigid bodies that you can add to the physics 
	// system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real 
	// project use something in the order of 65536.
	const uint cMaxBodies = 128000;

	// This determines how many mutexes to allocate to protect rigid bodies 
	// from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time 
	// (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a 
	// queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to
	// do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real 
	// project use something in the order of 65536.
	const uint cMaxBodyPairs = 65536;

	// This is the maximum size of the contact constraint buffer. If more 
	// contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start 
	// interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real 
	// project use something in the order of 10240.
	const uint cMaxContactConstraints = 10240;

	// Now we can create the actual physics system.
	physics_system = new PhysicsSystem();
	physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs,
		cMaxContactConstraints, broad_phase_layer_interface,
		object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

	// Define the physics world settings
	PhysicsSettings physicsSettingsData;
	physicsSettingsData.mNumVelocitySteps = 10;
	physicsSettingsData.mNumPositionSteps = 2;
	physicsSettingsData.mBaumgarte = 0.2f;

	physicsSettingsData.mSpeculativeContactDistance = 0.02f;
	physicsSettingsData.mPenetrationSlop = 0.02f;
	physicsSettingsData.mMinVelocityForRestitution = 1.0f;
	physicsSettingsData.mTimeBeforeSleep = 0.5f;
	physicsSettingsData.mPointVelocitySleepThreshold = 0.03f;

	physicsSettingsData.mDeterministicSimulation = true;
	physicsSettingsData.mConstraintWarmStart = true;
	physicsSettingsData.mUseBodyPairContactCache = true;
	physicsSettingsData.mUseManifoldReduction = true;
	physicsSettingsData.mUseLargeIslandSplitter = true;
	physicsSettingsData.mAllowSleeping = true;
	physicsSettingsData.mCheckActiveEdges = true;

	// Set the new physics settings
	physics_system->SetPhysicsSettings(physicsSettingsData);

	// Define gravity to act on the z-axis 
	// (so it is the same as Unreal's gravity)
	Vec3Arg newGravity(0.f, 0.f, -980.f);
	physics_system->SetGravity(newGravity);

	// A body activation listener gets notified when bodies activate and go 
	// to sleep
	// Note that this is called from a job so whatever you do here needs to be 
	// thread safe.
	// Registering one is entirely optional.
	physics_system->SetBodyActivationListener(body_activation_listener);

	// A contact listener gets notified when bodies (are about to) collide, 
	// and when they separate again.
	// Note that this is called from a job so whatever you do here needs to 
	// be thread safe.
	// Registering one is entirely optional.
	physics_system->SetContactListener(contact_listener);

	// The main way to interact with the bodies in the physics system is 
	// through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though 
	// we're not planning to access bodies from multiple threads)
	body_interface = &physics_system->GetBodyInterface();

	TArray<FString> initializationActorsInfoLines;
	initializationActorsInfo.ParseIntoArrayLines
		(initializationActorsInfoLines);

	// for each line, create a new body with according to the
	// body's type, id and initial location
	for (int i = 0; i < initializationActorsInfoLines.Num(); i++)
	{
		// Split info with ";" delimiter
		TArray<FString> actorInfoList;
		initializationActorsInfoLines[i].ParseIntoArray(actorInfoList,
			TEXT(";"));

		// Check for errors
		if (actorInfoList.Num() < 5)
		{
			LPES_LOG_INFO(TEXT("Error on parsing addBody message info. Line "
				"with less than 5 params."));
			continue;
		}

		// Get the actor's type to be creates
		const FString actorType = actorInfoList[0];

		// Get the actor ID from the init info
		const int actorId =	FCString::Atoi(*actorInfoList[1]);
		const BodyID newBodyID(actorId);

		// We discard here actorInfoList[2] as it is the BodyType info, no
		// used here

		// Get actor initial pos
		const double initialPosX = FCString::Atof(*actorInfoList[3]);
		const double initialPosY = FCString::Atof(*actorInfoList[4]);
		const double initialPosZ = FCString::Atof(*actorInfoList[5]);

		const RVec3 bodyInitialPosition{ RVec3(initialPosX, initialPosY,
			initialPosZ) };

		// Check if we should create a floor
		if (actorType.Contains("floor"))
		{
			// Add new floor to the physics world
			AddNewFloorToPhysicsSystem(newBodyID, bodyInitialPosition);
			continue;
		}

		// Check if we should create a sphere
		if (actorType.Contains("sphere"))
		{
			// Add new sphere to the physics world
			AddNewSphereToPhysicsWorld(newBodyID, bodyInitialPosition, RVec3(), 
				RVec3());
			continue;
		}
	}

	// Optional step: Before starting the physics simulation you can optimize 
	// the broad phase. This improves collision detection performance 
	// (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. 
	// streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to 
	// keep the broad phase efficient.
	//physics_system->OptimizeBroadPhase();

	// Reset the step physics time measurement 
	PhysicsStepSimulationTimeMeasure = "";

	bIsInitialized = true;

	LPES_LOG_INFO(TEXT("Physics world has been initialized and is running."));
}

FString FPhysicsServiceImpl::StepPhysicsSimulation()
{
	// If you take larger steps than 1 / 60th of a second you need to do 
	// multiple collision steps in order to keep the simulation stable. 
	// Do 1 collision step per 1 / 60th of a second (round up).
	const int cCollisionSteps = 1;

	// If you want more accurate step results you can do multiple sub steps 
	// within a collision step. Usually you would set this to 1.
	const int cIntegrationSubSteps = 1;

	// We simulate the physics world in discrete time steps. 60 Hz is a good 
	// rate to update the physics system.
	const float cDeltaTime = 1.0f / 60.f;

	// response string
	FString stepPhysicsResponse = "";

	// Get pre step physics time (time spent updating physics)
	std::chrono::steady_clock::time_point preStepPhysicsTime =
		std::chrono::steady_clock::now();

	// Step the world
	LPES_LOG_INFO(TEXT("(Step: %d)"), StepPhysicsCounter);

	physics_system->Update(cDeltaTime, cCollisionSteps, cIntegrationSubSteps,
		temp_allocator, job_system);

	LPES_LOG_INFO(TEXT("Physics stepping finished."));

	// Get post physics update time
	std::chrono::steady_clock::time_point postStepPhysicsTime =
		std::chrono::steady_clock::now();

	// Calculate the microsseconds all step physics simulation
	// (considering communication )took
	std::stringstream ss;
	ss << std::chrono::duration_cast<std::chrono::microseconds>
		(postStepPhysicsTime - preStepPhysicsTime).count();
	const std::string elapsedTime = ss.str();

	// Get the step phyiscs time (time spent updating physics on 
	// services) in FString
	const FString ElapsedPhysicsTimeMicroseconds =
		UTF8_TO_TCHAR(elapsedTime.c_str());

	// Append the step physics time to the current step measurement
	PhysicsStepSimulationTimeMeasure += ElapsedPhysicsTimeMicroseconds + "\n";

	// For each body on the physics system:
	for (auto& bodyId : BodyIdList)
	{
		FString bodyStepResultInfo{};

		// Apend the body Id as the first info on the body physics response
		bodyStepResultInfo += FString::Printf(TEXT("%d;"), bodyId.GetIndex());

		// Output current position of the sphere
		RVec3 position = body_interface->GetCenterOfMassPosition(bodyId);

		const FString actorStepPhysicsPositionResult = 
			FString::Printf(TEXT("%f;%f;%f;"), position.GetX(), 
			position.GetY(), position.GetZ());

		// Append the body's physics position result
		bodyStepResultInfo += actorStepPhysicsPositionResult;

		// Output current rotation of the sphere
		RVec3 rotation = body_interface->GetRotation(bodyId).GetEulerAngles();

		const FString actorStepPhysicsRotationResult =
			FString::Printf(TEXT("%f;%f;%f;"), rotation.GetX(), 
			rotation.GetY(), rotation.GetZ());

		// Append the the body's physics rotation result
		bodyStepResultInfo += actorStepPhysicsRotationResult;

		// Get the linear and angular velocity
		const auto linearVelocity{ body_interface->GetLinearVelocity(bodyId) };
		const auto angularVelocity
			{ body_interface->GetAngularVelocity(bodyId) };

		// Create the velocities string
		const FString actorStepPhysicsVelocitiesResult =
			FString::Printf(TEXT("%f;%f;%f;%f;%f;%f\n"), linearVelocity.GetX(), 
			linearVelocity.GetY(), linearVelocity.GetZ(), 
			angularVelocity.GetX(), angularVelocity.GetY(), 
			angularVelocity.GetZ());

		// Append the the body's physics velocity result
		bodyStepResultInfo += actorStepPhysicsVelocitiesResult;

		// Append the body step result info to the step physics response
		stepPhysicsResponse += bodyStepResultInfo;
	}

	//LPES_LOG_INFO(TEXT("(Step: %d) StepPhysics response: %s"), 
		//stepPhysicsCounter,	*stepPhysicsResponse);

	// Count the step
	StepPhysicsCounter++;

	return stepPhysicsResponse;
}

FString FPhysicsServiceImpl::AddNewSphereToPhysicsWorld(BodyID newBodyId, 
	RVec3 newBodyInitialPosition, RVec3 newBodyInitialLinearVelocity, 
	RVec3 newBodyInitialAngularVelocity)
{
	LPES_LOG_INFO(TEXT("NewSphere addition to physics world requested."));

	// Check if body interface is valid
	if (!body_interface)
	{
		LPES_LOG_INFO(TEXT("No body interface valid when adding new sphere to "
			"world."));
		return "No body interface valid when adding new sphere to world.\n";
	}

	// Create the settings for the body itself
	BodyCreationSettings sphere_settings(new SphereShape(50.f),
		newBodyInitialPosition, Quat::sIdentity(),
		EMotionType::Dynamic, Layers::MOVING);

	// Set the sphere's restitution 
	sphere_settings.mRestitution = 1.f;
	sphere_settings.mMassPropertiesOverride.mMass = 10.f;

	// Create the actual rigid body
	// Note that if we run out of bodies this can return nullptr
	Body* newSphereBody = body_interface->CreateBodyWithID(newBodyId,
		sphere_settings);

	// Check for errors
	if (!newSphereBody)
	{
		FString creationErrorString = FString::Printf(TEXT("Fail in creation "
			"of body with id %d"), newBodyId.GetIndexAndSequenceNumber());
		return creationErrorString;
	}

	// Add the body's ID to the list of IDs
	BodyIdList.push_back(newBodyId);

	// Set the body's linear velocity
	newSphereBody->SetLinearVelocity(newBodyInitialLinearVelocity);

	// Set the body's angular velocity
	newSphereBody->SetAngularVelocity(newBodyInitialAngularVelocity);

	// Add the new sphere to the world
	body_interface->AddBody(newSphereBody->GetID(), EActivation::Activate);

	return "New sphere body created successfully.";
}

FString FPhysicsServiceImpl::AddNewFloorToPhysicsSystem(const BodyID newBodyId, 
	const RVec3 newBodyInitialPosition)
{
	LPES_LOG_INFO(TEXT("NewFloor addition to physics world requested."));

	// Create the settings for the collision volume (the shape)
	BoxShapeSettings floor_shape_settings(Vec3(1000.0f, 1000.f, 100.0f));

	// Create the shape
	ShapeSettings::ShapeResult floor_shape_result =
		floor_shape_settings.Create();

	// We don't expect an error here, but you can check floor_shape_result for 
	// HasError() / GetError()
	ShapeRefC floor_shape = floor_shape_result.Get();

	// Create the settings for the body itself. Note that here you can also set 
	// other properties like the restitution / friction.
	BodyCreationSettings floor_settings(floor_shape, newBodyInitialPosition,
		Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);

	// Create the actual rigid body
	// Note that if we run out of bodies this can return nullptr
	Body* floor = body_interface->CreateBodyWithID(newBodyId, floor_settings);

	// Check if floor was created successfully
	if (!floor)
	{
		FString creationErrorString = FString(TEXT("Fail in creation of body "
			"with id %d."), newBodyId.GetIndexAndSequenceNumber());
		return creationErrorString;
	}

	// Set the floor's friction and add a small rotation on y-axis
	floor->SetFriction(1.0f);
	//floor->AddRotationStep(RVec3(0.f, -0.01f, 0.f));

	// Add it to the world
	body_interface->AddBody(floor->GetID(), EActivation::DontActivate);

	return "New floor body created successfully.";
}

FString FPhysicsServiceImpl::RemoveBodyByID(const BodyID bodyToRemoveID)
{
	LPES_LOG_INFO(TEXT("Remove body by ID requested for id %d."),
		bodyToRemoveID.GetIndex());

	// Check if body interface is valid
	if (!body_interface)
	{
		return "No body interface valid when removing body by ID.";
	}

	// Remove the ID from the list
	BodyIdList.erase(std::remove(BodyIdList.begin(), BodyIdList.end(),
		bodyToRemoveID), BodyIdList.end());

	// Remove the body by its ID and destroy it
	body_interface->RemoveBody(bodyToRemoveID);
	body_interface->DestroyBody(bodyToRemoveID);

	return "Body removal processed successfully";
}

void FPhysicsServiceImpl::ClearPhysicsSystem()
{
	LPES_LOG_INFO(TEXT("Cleaning physics system..."));

	for (auto& bodyId : BodyIdList)
	{
		// Remove the sphere from the physics system. Note that the sphere 
		// itself keeps all of its state and can be re-added at any time.
		body_interface->RemoveBody(bodyId);

		// Destroy the sphere. After this the sphere ID is no longer valid.
		body_interface->DestroyBody(bodyId);
	}

	// Unregisters all types with the factory and cleans up the default 
	// material
	UnregisterTypes();

	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;

	if (contact_listener) delete contact_listener;
	if (physics_system) delete physics_system;

	bIsInitialized = false;

	LPES_LOG_INFO(TEXT("Physics system was cleared. Exiting process..."));
}
