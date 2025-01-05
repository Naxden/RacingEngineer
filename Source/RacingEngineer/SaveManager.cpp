// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveManager.h"

#include "ImageUtils.h"
#include "RacingEngineerSaveGame.h"
#include "Kismet/GameplayStatics.h"

URacingEngineerSaveGame* USaveManager::CreateSaveSlot(const FString& SaveSlotName, const FString& MapTexturePath)
{
	URacingEngineerSaveGame* SaveGame = Cast<URacingEngineerSaveGame>(UGameplayStatics::CreateSaveGameObject(URacingEngineerSaveGame::StaticClass()));

	if (SaveGame != nullptr)	
	{
		SaveGame->SaveSlotName = SaveSlotName;

		FString SaveDirectory = FPaths::ProjectSavedDir();
		const FString DestinationPath = FPaths::Combine(SaveDirectory, FPaths::GetCleanFilename(MapTexturePath));

		if (FPlatformFileManager::Get().GetPlatformFile().CopyFile(*DestinationPath, *MapTexturePath))
		{
			UE_LOG(LogTemp, Log, TEXT("USaveManager::CreateSaveSlot File copied successfully to %s"), *DestinationPath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("USaveManager::CreateSaveSlot Failed to copy file to %s"), *DestinationPath);
			return nullptr;
		}

		SaveGame->MapTexturePath = DestinationPath;
		if (UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0))
		{
			UE_LOG(LogTemp, Log, TEXT("USaveManager::CreateSaveSlot Save game object created successfully"));
			return SaveGame;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("USaveManager::CreateSaveSlot Failed to save game object to slot"));
			return nullptr;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USaveManager::CreateSaveSlot Failed to create save game object"));
		return nullptr;
	}
}

bool USaveManager::LoadSaveSlot(const FString& SaveSlotName, URacingEngineerSaveGame*& OutSaveGame, UTexture2D*& OutMapTexture)
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		URacingEngineerSaveGame* SaveGame = Cast<URacingEngineerSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));

		if (SaveGame != nullptr)
		{
			OutSaveGame = SaveGame;

			FImage MapImage;

			if (FImageUtils::LoadImage(*SaveGame->MapTexturePath, MapImage))
			{
				OutMapTexture = FImageUtils::CreateTexture2DFromImage(MapImage);
				
				if (OutMapTexture != nullptr)
				{
					return true;
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("USaveManager::LoadSaveSlot Failed to create texture from image"));
					return false;
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("USaveManager::LoadSaveSlot Failed to load image from %s"), *SaveGame->MapTexturePath);
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("USaveManager::LoadSaveSlot Failed to load save game object"));
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USaveManager::LoadSaveSlot Save game object does not exist"));
		return false;
	}
	
}

TArray<FString> USaveManager::GetSaveSlotNames()
{
	TArray<FString> SaveSlotNames;

	const FString SaveSlotsDir = FPaths::Combine(FPaths::ProjectSavedDir(), SaveSlotsDirName);
	FPlatformFileManager::Get().GetPlatformFile().FindFiles(SaveSlotNames, *SaveSlotsDir, *SaveSlotExtension);

	for (FString& SaveSlotName : SaveSlotNames)
	{
		SaveSlotName = FPaths::GetBaseFilename(SaveSlotName);
	}

	return SaveSlotNames;
}
