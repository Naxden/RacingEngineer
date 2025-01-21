// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveManager.h"

#include "ImageUtils.h"
#include "RacingEngineerSaveGame.h"
#include "Kismet/GameplayStatics.h"

URacingEngineerSaveGame* USaveManager::CreateSaveSlot(const FString& SaveSlotName, const FString& MapTexturePath)
{
	if (!SaveSlotName.IsEmpty())
	{
		if (!MapTexturePath.IsEmpty())
		{
			URacingEngineerSaveGame* SaveGame = Cast<URacingEngineerSaveGame>(UGameplayStatics::CreateSaveGameObject(URacingEngineerSaveGame::StaticClass()));

			if (SaveGame != nullptr)
			{
				SaveGame->SaveSlotName = SaveSlotName;

				const FString SaveSlotsDirPath = FPaths::Combine(FPaths::ProjectSavedDir(), SaveSlotsDirName);
				const FString MapImageDestinationPath = FPaths::Combine(SaveSlotsDirPath, SaveSlotName + FPaths::GetExtension(MapTexturePath, true));

				SaveGame->MapImagePath = MapImageDestinationPath;
				if (UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0))
				{
					UE_LOG(LogTemp, Log, TEXT("USaveManager::CreateSaveSlot Save game object created successfully"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("USaveManager::CreateSaveSlot Failed to save game object to slot"));
					return nullptr;
				}

				if (FPlatformFileManager::Get().GetPlatformFile().CopyFile(*MapImageDestinationPath, *MapTexturePath))
				{
					UE_LOG(LogTemp, Log, TEXT("USaveManager::CreateSaveSlot File copied successfully to %s"), *MapImageDestinationPath);
					return SaveGame;
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("USaveManager::CreateSaveSlot Failed to copy file to %s"), *MapImageDestinationPath);
					return nullptr;
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("USaveManager::CreateSaveSlot Failed to create save game object"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("USaveManager::CreateSaveSlot MapTexturePath is empty"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USaveManager::CreateSaveSlot SaveSlotName is empty"));
	}

	return nullptr;
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

			if (FImageUtils::LoadImage(*SaveGame->MapImagePath, MapImage))
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
				UE_LOG(LogTemp, Error, TEXT("USaveManager::LoadSaveSlot Failed to load image from %s"), *SaveGame->MapImagePath);
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

bool USaveManager::OverrideSaveSlot(URacingEngineerSaveGame* SaveGame)
{
	if (SaveGame != nullptr)
	{
		const FString SaveSlotName = SaveGame->SaveSlotName;

		if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
		{
			if (UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0))
			{
				UE_LOG(LogTemp, Log, TEXT("USaveManager::OverrideSaveSlot Save game object overriden successfully"));
				return true;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("USaveManager::OverrideSaveSlot Failed to override save game object"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("USaveManager::OverrideSaveSlot Save game object does not exist"));
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USaveManager::OverrideSaveSlot Save game object is nullptr"));
		return false;
	}
}

bool USaveManager::DeleteSaveSlot(const FString& SaveSlotName)
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		URacingEngineerSaveGame* SaveGame = Cast<URacingEngineerSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));

		if (SaveGame != nullptr)
		{
			if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*SaveGame->MapImagePath))
			{
				UE_LOG(LogTemp, Log, TEXT("USaveManager::DeleteSaveSlot MapImage file deleted successfully"));

				if (UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0))
				{
					UE_LOG(LogTemp, Log, TEXT("USaveManager::DeleteSaveSlot Save game object deleted successfully"));
					return true;
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("USaveManager::DeleteSaveSlot Failed to delete save game object"));
					return false;
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("USaveManager::DeleteSaveSlot Failed to delete file"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("USaveManager::DeleteSaveSlot Failed to load save game object"));
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USaveManager::DeleteSaveSlot Save game object does not exist"));
		return false;
	}
}

TArray<FString> USaveManager::GetSaveSlotNames()
{
	TArray<FString> SaveSlotNames;

	const FString SaveSlotsDirPath = FPaths::Combine(FPaths::ProjectSavedDir(), SaveSlotsDirName);
	FPlatformFileManager::Get().GetPlatformFile().FindFiles(SaveSlotNames, *SaveSlotsDirPath, *SaveSlotExtension);

	for (FString& SaveSlotName : SaveSlotNames)
	{
		SaveSlotName = FPaths::GetBaseFilename(SaveSlotName);
	}

	return SaveSlotNames;
}
