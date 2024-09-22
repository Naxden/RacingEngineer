// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingEngineerWheelFront.h"
#include "UObject/ConstructorHelpers.h"

URacingEngineerWheelFront::URacingEngineerWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}