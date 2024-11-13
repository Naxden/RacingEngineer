// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapManager.generated.h"

class ATrackGenerator;
class ATerrainGenerator;

UCLASS()
class RACINGENGINEER_API AMapManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	static TArray<FColor> GetColorsFromTexture(UTexture2D* Texture);

	static void GetColors(TArray<FColor>& ColorData, void* SrcData, uint32 TextureWidth, uint32 TextureHeight);
private:
	void WorkerFinished();

private:
	UPROPERTY(EditAnywhere)
	UTexture2D* HeightMapTexture;

	UPROPERTY(EditAnywhere)
	TObjectPtr<ATerrainGenerator> TerrainGenerator;

	UPROPERTY(EditAnywhere)
	TObjectPtr<ATrackGenerator> TrackGenerator;

	TArray<FColor> TextureColors;

	std::atomic_uint8_t FinishedWorkersCounter = 0;
};
