// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RacingEngineerPawn.h"
#include "RacingEngineerSportsCar.generated.h"

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class RACINGENGINEER_API ARacingEngineerSportsCar : public ARacingEngineerPawn
{
	GENERATED_BODY()
	
public:

	ARacingEngineerSportsCar();
};
