// Fill out your copyright notice in the Description page of Project Settings.


#include "MapFilePicker.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "ImageUtils.h"

UMapFilePicker::UMapFilePicker()
{

}

FString UMapFilePicker::OpenFileDialog()
{
	const UEngine* Engine = GEngine;
	if (Engine != nullptr)
	{
		TObjectPtr<UGameViewportClient> GameView = Engine->GameViewport;
		if (GameView != nullptr)
		{
			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
			if (DesktopPlatform != nullptr)
			{
				TArray<FString> OutFiles;
				const bool bSelected = DesktopPlatform->OpenFileDialog(
					GameView->GetWindow()->GetNativeWindow()->GetOSWindowHandle(),
					TEXT("Choose a picture of map"),
					FPaths::ProjectSavedDir(),
					TEXT(""),
					TEXT("Pictures (*.png)|*.png"),
					EFileDialogFlags::None,
					OutFiles);

				if (bSelected && OutFiles.Num() > 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("ProjectPath %s"), *FPaths::ProjectSavedDir());
					return FPaths::ConvertRelativePathToFull(OutFiles[0]);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("UMapFilePicker::OpenFileDialog DesktopPlatform is nullptr"));
			}

		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UMapFilePicker::OpenFileDialog() GameViewportClient is nullptr"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UMapFilePicker::OpenFileDialog() Engine is nullptr"));
	}

	return "";
}

UTexture2D* UMapFilePicker::LoadFileToTexture(const FString& FilePath)
{
	FImage Image;

	if (FImageUtils::LoadImage(*FilePath, Image))
	{
		return FImageUtils::CreateTexture2DFromImage(Image);
	}

	return nullptr;
}
