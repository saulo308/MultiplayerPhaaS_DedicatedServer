// Fill out your copyright notice in the Description page of Project Settings.


#include "LocalPhysicsEngineSystem/Public/JoltPhysicsSystem/ObjectLayerPairFilterImpl.h"

bool FObjectLayerPairFilterImpl::ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const
{
	switch (inObject1)
	{
	case Layers::NON_MOVING:
		return inObject2 == Layers::MOVING; // Non moving only collides with moving
	case Layers::MOVING:
		return true; // Moving collides with everything
	default:
		return false;
	}
}