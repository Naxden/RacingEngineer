// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "WorkerActor.generated.h"

DECLARE_DELEGATE(FOnWorkFinished);

UCLASS()
class RACINGENGINEER_API AWorkerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWorkerActor();

	virtual void DoWork(const TArray<FColor>& HeightTextureColors, const USplineComponent* TrackSpline, const FVector& VertScale, const
	                    FOnWorkFinished Callback);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
