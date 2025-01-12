// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorkerActor.h"
#include "CheckpointGenerator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerUpdate, float, TimerValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLapFinished, float, LapTimeValue);

USTRUCT()
struct FCheckpointSpawnData
{
	GENERATED_BODY()

	FVector Location;
	FRotator Rotation;
};

class ATrackCheckpoint;
/**
 * 
 */
UCLASS()
class RACINGENGINEER_API ACheckpointGenerator : public AWorkerActor
{
	GENERATED_BODY()
public:
	ACheckpointGenerator();

	virtual void Tick(float DeltaTime) override;

	virtual void DoWork(const FWorkerData& Data, const FOnWorkFinished Callback) override;

	UFUNCTION(BlueprintCallable)
	void StartTimer();

	UPROPERTY(BlueprintAssignable)
	FOnTimerUpdate OnTimerUpdateEvent;

	UPROPERTY(BlueprintAssignable)
	FOnLapFinished OnLapFinishedEvent;

private:
	TArray<TWeakObjectPtr<ATrackCheckpoint>> SpawnCheckpointsAlongSpline(const USplineComponent* TrackSpline);

	void OnCheckpointOverlapped(uint16 CheckpointIndex);

	void UpdateTimer(float DeltaTime);

	void PrepareCheckpointData(const USplineComponent* TrackSpline, float CheckpointDistance);
	void SpawnCheckpointsBasedOnPreparedData(TArray<FCheckpointSpawnData>& CheckpointsData, FOnWorkFinished Callback);

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<ATrackCheckpoint> TrackCheckpointClass;

	UPROPERTY(EditAnywhere)
	float DistanceBetweenCheckpoints = 200.0f;

	UPROPERTY(VisibleAnywhere)
	TArray<TWeakObjectPtr<ATrackCheckpoint>> SpawnedTrackCheckpoints;

	UPROPERTY(VisibleAnywhere)
	bool bTimerStarted = false;

	UPROPERTY()
	float LapTime = 0.0f;

	uint16 TargetCheckpointIndex = UINT16_MAX;

	UPROPERTY(EditAnywhere)
	int32 BatchSize = 5;

	TArray<FCheckpointSpawnData> CheckpointSpawnData;
	int32 CheckpointSpawned = 0;
	FTimerDelegate SpawnBatchCheckpointTimer;
};
