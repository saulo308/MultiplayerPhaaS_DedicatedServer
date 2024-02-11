// Fill out your copyright notice in the Description page of Project Settings.


#include "LocalPhysicsEngineSystem/Public/JoltPhysicsSystem/PhysicsServiceImpl.h"

FPhysicsServiceImpl::FPhysicsServiceImpl()
{

}

void FPhysicsServiceImpl::InitPhysicsSystem(const std::string initializationActorsInfo)
{
	UE_LOG(LogTemp, Warning, TEXT("Initializing physics system..."));

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

	// We need a temp allocator for temporary allocations during the physics update. We're
	// pre-allocating 10 MB to avoid having to do allocations during the physics update. 
	// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	// malloc / free.
	temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);

	// We need a job system that will execute physics jobs on multiple threads. Typically
	// you would implement the JobSystem interface yourself and let Jolt Physics run on top
	// of your own job scheduler. JobSystemThreadPool is an example implementation.
	job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodies = 65536;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodyPairs = 65536;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const uint cMaxContactConstraints = 10240;

	// Now we can create the actual physics system.
	physics_system = new PhysicsSystem();
	physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

	PhysicsSettings PhysicsSettingsData;
	PhysicsSettingsData.mNumVelocitySteps = 10;
	PhysicsSettingsData.mNumPositionSteps = 2;
	PhysicsSettingsData.mBaumgarte = 0.2f;

	PhysicsSettingsData.mSpeculativeContactDistance = 0.02f;
	PhysicsSettingsData.mPenetrationSlop = 0.02f;
	PhysicsSettingsData.mMinVelocityForRestitution = 1.0f;
	PhysicsSettingsData.mTimeBeforeSleep = 0.5f;
	PhysicsSettingsData.mPointVelocitySleepThreshold = 0.03f;

	PhysicsSettingsData.mDeterministicSimulation = true;
	PhysicsSettingsData.mConstraintWarmStart = true;
	PhysicsSettingsData.mUseBodyPairContactCache = true;
	PhysicsSettingsData.mUseManifoldReduction = true;
	PhysicsSettingsData.mUseLargeIslandSplitter = true;
	PhysicsSettingsData.mAllowSleeping = true;
	PhysicsSettingsData.mCheckActiveEdges = true;

	physics_system->SetPhysicsSettings(PhysicsSettingsData);

	Vec3Arg newGravity(0.f, 0.f, -980.f);
	physics_system->SetGravity(newGravity);

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	//MyBodyActivationListener body_activation_listener;
	physics_system->SetBodyActivationListener(body_activation_listener);

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	contact_listener = new FMyContactListener();
	physics_system->SetContactListener(contact_listener);


	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	body_interface = &physics_system->GetBodyInterface();

	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape). 
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	BoxShapeSettings floor_shape_settings(Vec3(1000.f, 1000.f, 100.f));

	// Create the shape
	ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	BodyCreationSettings floor_settings(floor_shape, RVec3(0.0_r, 0.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);

	// Create the actual rigid body
	Body* floor = body_interface->CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr
	floor_id = floor->GetID();

	floor->SetFriction(1.0f);
	floor->AddRotationStep(RVec3(0.f, -0.01f, 0.f));

	// Add it to the world
	body_interface->AddBody(floor->GetID(), EActivation::DontActivate);

	// Split actors info from initialization into lines
	std::stringstream initializationStringStream(initializationActorsInfo);
	std::vector<std::string> initializationActorsInfoLines;

	std::string line;
	while (std::getline(initializationStringStream, line))
	{
		initializationActorsInfoLines.push_back(line);
	}

	// for each line (begin from 1 as first is only "Init"), create a box boddy with it's ID
	for (int i = 1; i < initializationActorsInfoLines.size(); i++)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Initializing: " << initializationActorsInfoLines[i] << std::endl;

		// Split info with ";" delimiter
		std::stringstream actorInfoStringStream(initializationActorsInfoLines[i]);
		std::vector<std::string> actorInfoList;

		std::string actorInfoData;
		while (std::getline(actorInfoStringStream, actorInfoData, ';'))
		{
			actorInfoList.push_back(actorInfoData);
		}

		// Check for errors
		if (actorInfoList.size() < 4)
		{
			//std::cout << "Error on parsing initialization actor info. Less than 4 params\n";
			return;
		}

		// Get actor initial pos
		const double initialPosX = std::stod(actorInfoList[1]);
		const double initialPosY = std::stod(actorInfoList[2]);
		const double initialPosZ = std::stod(actorInfoList[3]);

		//std::cout << "Creating...\n";

		// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
		Vec3Arg boxHalfSize(0.5f, 0.5f, 0.5f);
		BodyCreationSettings box_settings(new SphereShape(50.f), RVec3(initialPosX, initialPosY, initialPosZ), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
		box_settings.mRestitution = 1.f;

		// Get the actor ID and create a BodyID
		const int actorId = std::stoi(actorInfoList[0]);
		const BodyID newActorBodyID(actorId);
		BodyIdList.push_back(newActorBodyID);

		// Create the actual rigid body
		Body* newActorBody = body_interface->CreateBodyWithID(newActorBodyID, box_settings); // Note that if we run out of bodies this can return nullptr
		if (!newActorBody)
		{
			//std::cout << "Fail in creation of body " << actorId << std::endl;
			continue;
		}

		// Add it to the world
		body_interface->AddBody(newActorBody->GetID(), EActivation::Activate);
	}

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	physics_system->OptimizeBroadPhase();

	bIsInitialized = true;

	UE_LOG(LogTemp, Warning, TEXT("Physics system is up and running."));
}

FString FPhysicsServiceImpl::StepPhysicsSimulation()
{
	// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
	const int cCollisionSteps = 1;

	// If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.
	const int cIntegrationSubSteps = 1;

	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	const float cDeltaTime = 1.0f / 60.f;

	// Step the world
	physics_system->Update(cDeltaTime, cCollisionSteps, cIntegrationSubSteps, temp_allocator, job_system);

	// response string
	FString stepPhysicsResponse = "";

	// Foreach body:
	for (auto& bodyId : BodyIdList)
	{
		// Output current position of the sphere
		RVec3 position = body_interface->GetCenterOfMassPosition(bodyId);
		const FString actorStepPhysicsPositionResult = FString::Printf(TEXT("%f;%f;%f"),
			position.GetX(), position.GetY(), position.GetZ());

		RVec3 rotation = body_interface->GetRotation(bodyId).GetEulerAngles();

		const FString actorStepPhysicsRotationResult = FString::Printf(TEXT("%f;%f;%f"),
			rotation.GetX(), rotation.GetY(), rotation.GetZ());

		stepPhysicsResponse += FString::Printf(TEXT("%s;%s;%s\n"), 
			*FString(std::to_string(bodyId.GetIndex()).c_str()),
			*actorStepPhysicsPositionResult,
			*actorStepPhysicsRotationResult);
	}

	//std::cout << "StepPhysics response:\n" << stepPhysicsResponse << std::endl;
	return stepPhysicsResponse;
}

void FPhysicsServiceImpl::ClearPhysicsSystem()
{
	std::cout << "Cleaing physics system...\n";

	for (auto& bodyId : BodyIdList)
	{
		// Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
		body_interface->RemoveBody(bodyId);

		// Destroy the sphere. After this the sphere ID is no longer valid.
		body_interface->DestroyBody(bodyId);
	}

	// Remove and destroy the floor
	body_interface->RemoveBody(floor_id);
	body_interface->DestroyBody(floor_id);

	// Unregisters all types with the factory and cleans up the default material
	UnregisterTypes();

	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;

	if (contact_listener) delete contact_listener;
	if (physics_system) delete physics_system;

	bIsInitialized = false;

	UE_LOG(LogTemp, Warning, TEXT("Physics system was cleared. Exiting process..."));
}
