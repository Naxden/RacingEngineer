// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingEngineerWheelRear.h"
#include "UObject/ConstructorHelpers.h"

URacingEngineerWheelRear::URacingEngineerWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}