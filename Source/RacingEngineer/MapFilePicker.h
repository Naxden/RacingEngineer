// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapFilePicker.generated.h"

#undef LoadImage
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class RACINGENGINEER_API UMapFilePicker : public UObject
{
	GENERATED_BODY()

public:

	UMapFilePicker();

	UFUNCTION(BlueprintCallable, Category = "Map File Loading")
	static UPARAM(DisplayName = "File Path") FString OpenFileDialog();

	UFUNCTION(BlueprintCallable, Category = "Map File Loading")
	static UTexture2D* LoadFileToTexture(const FString& FilePath);
};