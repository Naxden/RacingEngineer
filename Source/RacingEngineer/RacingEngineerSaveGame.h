// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RacingEngineerSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class RACINGENGINEER_API URacingEngineerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	URacingEngineerSaveGame();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData")
	float BestLapTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData")
	FString MapTexturePath = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveGameData")
	FString SaveSlotName = TEXT("");
};
