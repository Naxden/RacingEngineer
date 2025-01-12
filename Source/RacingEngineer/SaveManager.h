// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SaveManager.generated.h"


class URacingEngineerSaveGame;
/**
 * 
 */
UCLASS()
class RACINGENGINEER_API USaveManager : public UObject
{
	GENERATED_BODY()

	static inline FString SaveSlotsDirName = TEXT("SaveGames");
	static inline FString SaveSlotExtension = TEXT("sav");
public:

	UFUNCTION(BlueprintCallable, Category = "Racing Enginner Saves")
	static URacingEngineerSaveGame* CreateSaveSlot(const FString& SaveSlotName, const FString& MapTexturePath);

	UFUNCTION(BlueprintCallable, Category = "Racing Enginner Saves")
	static bool LoadSaveSlot(const FString& SaveSlotName, URacingEngineerSaveGame*& OutSaveGame, UTexture2D*& OutMapTexture);

	UFUNCTION(BlueprintCallable, Category = "Racing Enginner Saves")
	static bool OverrideSaveSlot(URacingEngineerSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "Racing Enginner Saves")
	static bool DeleteSaveSlot(const FString& SaveSlotName);

	UFUNCTION(BlueprintCallable, Category = "Racing Enginner Saves")
	static TArray<FString> GetSaveSlotNames();
};
