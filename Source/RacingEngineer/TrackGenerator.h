// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackGenerator.generated.h"

class USplineComponent;

UCLASS()
class RACINGENGINEER_API ATrackGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrackGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnConstruction(const FTransform& Transform) override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	USplineComponent* SplineComponent;

	UPROPERTY(EditAnywhere)
	UStaticMesh* TrackMesh;

private:
	UPROPERTY()
	TArray<FVector> SplineTangents;


	UPROPERTY(EditAnywhere)
	double VertSpacing = 250.0;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ActorToSpawn;

	void SpawnMeshBasedOnMeshLength(const FVector& MeshSize);
	void CreateMeshToSpline();

	FVector GetMeshLength() const;
	void SpawnMeshPerSplinePoint(uint32 NumberOfSplinePoints, FVector MeshOffset);
};
