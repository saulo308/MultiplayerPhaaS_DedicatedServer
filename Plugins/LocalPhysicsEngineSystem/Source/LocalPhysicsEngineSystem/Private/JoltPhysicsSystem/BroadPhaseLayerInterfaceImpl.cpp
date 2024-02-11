// Fill out your copyright notice in the Description page of Project Settings.


#include "LocalPhysicsEngineSystem/Public/JoltPhysicsSystem/BroadPhaseLayerInterfaceImpl.h"

FBroadPhaseLayerInterfaceImpl::FBroadPhaseLayerInterfaceImpl()
{
	// Create a mapping table from object to broad phase layer
	mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
	mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
}

uint FBroadPhaseLayerInterfaceImpl::GetNumBroadPhaseLayers() const
{
	return BroadPhaseLayers::NUM_LAYERS;
}

BroadPhaseLayer FBroadPhaseLayerInterfaceImpl::GetBroadPhaseLayer(ObjectLayer inLayer) const
{
	return mObjectToBroadPhase[inLayer];
}
