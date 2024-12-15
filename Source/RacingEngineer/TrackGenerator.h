// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorkerActor.h"
#include "GameFramework/Actor.h"
#include "TrackGenerator.generated.h"

UCLASS()
class RACINGENGINEER_API ATrackGenerator : public AWorkerActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrackGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void DoWork(const TArray<FColor>& HeightTextureColors, const USplineComponent* TrackSpline, const FVector& VertScale, const
	                    FOnWorkFinished Callback) override;
	UFUNCTION()
	FVector GetTrackMeshSize() const;
public:
	UPROPERTY(EditAnywhere)
	UStaticMesh* TrackMesh;

private:
	void SpawnMeshBasedOnMeshLength(const USplineComponent* TrackSpline, const FVector& MeshSize);
	void CreateMeshOnSpline(const USplineComponent* TrackSplineComponent);
	void SpawnMeshPerSplinePoint(const USplineComponent* TrackSpline, const FVector& MeshOffset);

private:
	UPROPERTY(EditAnywhere)
	double TangentScalar = 0.5;
};
