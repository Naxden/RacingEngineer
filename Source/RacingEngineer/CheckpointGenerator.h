// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorkerActor.h"
#include "CheckpointGenerator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerUpdate, float, TimerValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLapFinished, float, LapTimeValue);

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

	virtual void DoWork(const TArray<FColor>& HeightTextureColors, const USplineComponent* TrackSpline, const FVector& VertScale, const FOnWorkFinished Callback) override;

	UPROPERTY(BlueprintAssignable)
	FOnTimerUpdate OnTimerUpdateEvent;

	UPROPERTY(BlueprintAssignable)
	FOnLapFinished OnLapFinishedEvent;

private:
	TArray<TWeakObjectPtr<ATrackCheckpoint>> SpawnCheckpointsAlongSpline(const USplineComponent* TrackSpline);

	void OnCheckpointOverlapped(uint16 CheckpointIndex);

	void UpdateTimer(float DeltaTime);
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

	uint16 TargetCheckpointIndex = 0;
};
