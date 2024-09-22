// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingEngineerGameMode.h"
#include "RacingEngineerPlayerController.h"

ARacingEngineerGameMode::ARacingEngineerGameMode()
{
	PlayerControllerClass = ARacingEngineerPlayerController::StaticClass();
}
