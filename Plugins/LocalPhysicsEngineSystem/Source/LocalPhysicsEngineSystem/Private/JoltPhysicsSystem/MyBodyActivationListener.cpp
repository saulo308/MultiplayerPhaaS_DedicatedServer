// Fill out your copyright notice in the Description page of Project Settings.


#include "LocalPhysicsEngineSystem/Public/JoltPhysicsSystem/MyBodyActivationListener.h"

void FMyBodyActivationListener::OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData)
{
	UE_LOG(LogTemp, Warning, TEXT("A body got activated"));
}

void FMyBodyActivationListener::OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData)
{
	UE_LOG(LogTemp, Warning, TEXT("A body went to sleep"));
}
