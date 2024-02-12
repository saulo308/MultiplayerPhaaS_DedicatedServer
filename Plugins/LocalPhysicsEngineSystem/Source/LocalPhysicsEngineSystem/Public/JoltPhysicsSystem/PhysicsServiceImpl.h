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

    /**
    * Initializes a physics system. Any Body that may exist on the
    * initialization should be given on the param. This will create the
    * physics world and set all the needed settings for it.
    *
    * The param's template should be:
    * "Init\n
    * bodyType; Id_0; posX_0; posY_0; posZ_0\n
    * bodyType; Id_1; posX_1; posY_1; posZ_1\n
    * bodyType; Id_2; posX_2; posY_2; posZ_2\n
    * ...
    * MessageEnd"
    */
    void InitPhysicsSystem(const FString& initializationActorsInfo);

    /**
    * Steps the current physics system simulation by one frame.
    *
    * @return The step physics simulation result. This will send each actor's
    * Id, position and rotation of the current physics system state back to
    * the client.
    */
    FString StepPhysicsSimulation();

    /**
    * Clears the current physics system. This will shut the created physics
    * system down
    */
    void ClearPhysicsSystem();

    /**
    * Adds a new sphere to the physics world. This will add a Body to the
    * current running physics system, given its BodyId and initial position.
    * The sphere will be dynamic and movable.
    *
    * @param newBodyId The BodyID of the sphere to add to the physics world
    * @param newBodyInitialPosition The sphere's initial position on the
    * physics world
    *
    * @return The result of the sphere's addition. May return a failure message
    * if the sphere could not be added successfully
    */
    FString AddNewSphereToPhysicsWorld(BodyID newBodyId, 
        RVec3 newBodyInitialPosition,
        RVec3 newBodyInitialLinearVelocity,
        RVec3 newBodyInitialAngularVelocity);

    /**
    * Adds a new floor to the physics world. This will add a Body to the
    * current running physics system, given its BodyId and initial position.
    * This floor will be static and should not move.
    *
    * @param newBodyId The BodyID of the floor to add to the physics world
    * @param newBodyInitialPosition The floor's initial position on the
    * physics world
    *
    * @return The result of the floor's addition. May return a failure message
    * if the floor could not be added successfully
    */
    FString AddNewFloorToPhysicsSystem(const BodyID newBodyId,
        const RVec3 newBodyInitialPosition);

    /** */
    FString GetSimulationMeasures() const
        { return PhysicsStepSimulationTimeMeasure; }

    /**
    * Removes a Body from the current running physics world. Thus, this body
    * will be removed from the simulation
    *
    * @param bodyToRemoveID The BodyID from the body to remove from the
    * physics world
    *
    * @return The result of the body's removal. May return a failure message
    * if the body could not be removed successfully
    */
    FString RemoveBodyByID(const BodyID bodyToRemoveID);

private:
    // Callback for traces, connect this to your own trace function if you 
    // have one
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
    // Callback for asserts, connect this to your own assert handler if you
    //  have one
    static bool AssertFailedImpl(const char* inExpression,
        const char* inMessage, const char* inFile, uint inLine)
    {
        // Print to the TTY
        cout << inFile << ":" << inLine << ": (" << inExpression << ") "
            << (inMessage != nullptr ? inMessage : "") << endl;

        // Breakpoint
        return true;
    };

#endif // JPH_ENABLE_ASSERTS

public:
    TempAllocator* temp_allocator = nullptr;
    JobSystem* job_system = nullptr;

    /**
    * Create mapping table from object layer to broadphase layer
    * Note: As this is an interface, PhysicsSystem will take a reference to
    * this so this instance needs to stay alive!
    */
    FBroadPhaseLayerInterfaceImpl broad_phase_layer_interface;

    /**
    * Create class that filters object vs broadphase layers
    * Note: As this is an interface, PhysicsSystem will take a reference to
    * this so this instance needs to stay alive!
    */
    FObjectBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

    /**
    * Create class that filters object vs object layers
    * Note: As this is an interface, PhysicsSystem will take a reference to
    * this so this instance needs to stay alive!
    */
    FObjectLayerPairFilterImpl object_vs_object_layer_filter;

    /** The current created BodyInterface of JoltPhysics */
    BodyInterface* body_interface = nullptr;

    /** The running physics system */
    PhysicsSystem* physics_system = nullptr;

    /** The implemented body activation listener on the physics system */
    FMyBodyActivationListener* body_activation_listener = nullptr;

    /** The implemented contact listener on the physics system */
    FMyContactListener* contact_listener = nullptr;

    /**
    * The list of BodyID from all the bodies on the current running physics
    * system. Used to query each body location and rotation on each physics
    * step
    */
    std::vector<BodyID> BodyIdList;

    /** Flag that indicates if the physics system is initialized */
    bool bIsInitialized = false;

    /**
    * The step physics counter. Will count the number of steps as it
    * increases at each step physics call.
    */
    uint32 StepPhysicsCounter = 0;

    /**
    * The current physics step time measure without communication overhead.
    * Used to test the overall system
    */
    FString PhysicsStepSimulationTimeMeasure = "";
};
