// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RacingEngineerGameInstance.generated.h"

class URacingEngineerSaveGame;
/**
 * 
 */
UCLASS()
class RACINGENGINEER_API URacingEngineerGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance")
	bool bStartedFromMainMenu = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance")
	TObjectPtr<UTexture2D> SelectedMapTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameInstance")
	TObjectPtr<URacingEngineerSaveGame> SelectedSaveSlot;
};
