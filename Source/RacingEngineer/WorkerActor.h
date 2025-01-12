// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "WorkerActor.generated.h"

DECLARE_DELEGATE(FOnWorkFinished);

USTRUCT()
struct FWorkerData
{
	GENERATED_BODY()

	TArray<FColor> HeightTextureColors;
	const USplineComponent* TrackSpline;
	FVector VertScale;

	FWorkerData()
		: HeightTextureColors()
		, TrackSpline(nullptr)
		, VertScale(FVector::ZeroVector)
	{
	}

	FWorkerData(const TArray<FColor>& InHeightTextureColors, const USplineComponent* InTrackSpline, const FVector& InVertScale)
		: HeightTextureColors(InHeightTextureColors)
		, TrackSpline(InTrackSpline)
		, VertScale(InVertScale)
	{
	}
};

UCLASS()
class RACINGENGINEER_API AWorkerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWorkerActor();

	virtual void DoWork(const FWorkerData& Data, const FOnWorkFinished Callback);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
