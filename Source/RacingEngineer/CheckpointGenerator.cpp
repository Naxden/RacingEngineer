// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckpointGenerator.h"

#include "TrackCheckpoint.h"

ACheckpointGenerator::ACheckpointGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACheckpointGenerator::Tick(float DeltaTime)
{
	if (bTimerStarted)
	{
		UpdateTimer(LapTime + DeltaTime);
	}

	Super::Tick(DeltaTime);
}

void ACheckpointGenerator::DoWork(const TArray<FColor>& HeightTextureColors, const USplineComponent* TrackSpline,
                                  const FVector& VertScale, const FOnWorkFinished Callback)
{
	SpawnedTrackCheckpoints = SpawnCheckpointsAlongSpline(TrackSpline);

	if (SpawnedTrackCheckpoints.Num() > 1)
	{
		SpawnedTrackCheckpoints[1]->SetMaterialToTarget();
		TargetCheckpointIndex = 1;

		bTimerStarted = true;
	}

	Super::DoWork(HeightTextureColors, TrackSpline, VertScale, Callback);
}

void ACheckpointGenerator::UpdateTimer(float TimerTime)
{
	LapTime = TimerTime;

	if (OnTimerUpdateEvent.IsBound())
	{
		OnTimerUpdateEvent.Broadcast(LapTime);
	}
}

TArray<TWeakObjectPtr<ATrackCheckpoint>> ACheckpointGenerator::SpawnCheckpointsAlongSpline(const USplineComponent* TrackSpline)
{
	TArray<TWeakObjectPtr<ATrackCheckpoint>> Checkpoints;

	if (TrackSpline != nullptr)
	{
		const uint32 CheckpointsToSpawn = FMath::FloorToInt32(TrackSpline->GetSplineLength() / DistanceBetweenCheckpoints);

		Checkpoints.Reserve(CheckpointsToSpawn);

		for (uint32 CheckpointCounter = 0; CheckpointCounter < CheckpointsToSpawn; ++CheckpointCounter)
		{
			const float Distance = CheckpointCounter * DistanceBetweenCheckpoints;
			const FVector Location = TrackSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			const FRotator Rotation = TrackSpline->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

			ATrackCheckpoint* Checkpoint = GetWorld()->SpawnActor<ATrackCheckpoint>(TrackCheckpointClass, Location, Rotation);
			if (Checkpoint != nullptr)
			{
				Checkpoint->SetCheckpointIndex(CheckpointCounter);
				Checkpoint->OnPawnOverlappedWTrackCheckpoint.AddUObject(this, &ACheckpointGenerator::OnCheckpointOverlapped);
				Checkpoint->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform,
					*FString::Printf(TEXT("TrackCheckpoint%d"), CheckpointCounter));

				Checkpoints.Emplace(Checkpoint);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ACheckpointGenerator::SpawnCheckpointsAlongSpline Failed to spawn checkpoint"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ACheckpointGenerator::SpawnCheckpointsAlongSpline Track spline is nullptr"));
	}

	return Checkpoints;
}

void ACheckpointGenerator::OnCheckpointOverlapped(uint16 CheckpointIndex)
{
	UE_LOG(LogTemp, Log, TEXT("Received info, player collided with %d"), CheckpointIndex);

	if (CheckpointIndex == TargetCheckpointIndex)
	{
		SpawnedTrackCheckpoints[CheckpointIndex]->SetMaterialToBasic();

		TargetCheckpointIndex++;

		if (TargetCheckpointIndex == SpawnedTrackCheckpoints.Num())
		{
			TargetCheckpointIndex = 0;

			if (OnLapFinishedEvent.IsBound())
			{
				OnLapFinishedEvent.Broadcast(LapTime);
			}

			UpdateTimer(0.0f);
		}

		SpawnedTrackCheckpoints[TargetCheckpointIndex]->SetMaterialToTarget();
	}
}