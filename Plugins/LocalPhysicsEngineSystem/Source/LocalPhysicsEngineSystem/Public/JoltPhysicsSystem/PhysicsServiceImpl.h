// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "BroadPhaseLayerInterfaceImpl.h"
#include "MyBodyActivationListener.h"
#include "MyContactListener.h"
#include "ObjectLayerPairFilterImpl.h"
#include "ObjectBroadPhaseLayerFilterImpl.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

/**
 * 
 */
class LOCALPHYSICSENGINESYSTEM_API FPhysicsServiceImpl
{
public:
	FPhysicsServiceImpl();

	void InitPhysicsSystem(const std::string initializationActorsInfo);
	FString StepPhysicsSimulation();

	void ClearPhysicsSystem();

private:
	// Callback for traces, connect this to your own trace function if you have one
	static void TraceImpl(const char* inFMT, ...)
	{
		// Format the message
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);
	}

#ifdef JPH_ENABLE_ASSERTS
	// Callback for asserts, connect this to your own assert handler if you have one
	static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
	{
		// Print to the TTY
		cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << endl;

		// Breakpoint
		return true;
	};

#endif // JPH_ENABLE_ASSERTS

public:
	TempAllocator* temp_allocator = nullptr;
	JobSystem* job_system = nullptr;

	// Create mapping table from object layer to broadphase layer
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	FBroadPhaseLayerInterfaceImpl broad_phase_layer_interface;

	// Create class that filters object vs broadphase layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	FObjectBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

	// Create class that filters object vs object layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	FObjectLayerPairFilterImpl object_vs_object_layer_filter;

	BodyInterface* body_interface = nullptr;
	PhysicsSystem* physics_system = nullptr;

	FMyBodyActivationListener* body_activation_listener = nullptr;
	FMyContactListener* contact_listener = nullptr;

	BodyID floor_id = BodyID();
	BodyID sphere_id = BodyID();
	//BodyID cube_id = BodyID();
	std::vector<BodyID> BodyIdList;

	bool bIsInitialized = false;
};
