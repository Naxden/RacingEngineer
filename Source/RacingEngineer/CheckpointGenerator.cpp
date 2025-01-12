// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckpointGenerator.h"

#include "TrackCheckpoint.h"
#include "Async/Async.h"

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

void ACheckpointGenerator::DoWork(const FWorkerData& Data, const FOnWorkFinished Callback)
{
	//SpawnedTrackCheckpoints = SpawnCheckpointsAlongSpline(Data.TrackSpline);
	
	PrepareCheckpointData(Data.TrackSpline, DistanceBetweenCheckpoints);

	AsyncTask(ENamedThreads::GameThread, [this, Callback]
		{
			SpawnCheckpointsBasedOnPreparedData(CheckpointSpawnData, Callback);

		});

}

void ACheckpointGenerator::StartTimer()
{
	if (SpawnedTrackCheckpoints.Num() > 1)
	{
		if (SpawnedTrackCheckpoints[1].IsValid())
		{
			SpawnedTrackCheckpoints[1]->SetMaterialToTarget();
			TargetCheckpointIndex = 1;

			UpdateTimer(0.0f);

			bTimerStarted = true;
		}
	}
}

void ACheckpointGenerator::UpdateTimer(float TimerTime)
{
	LapTime = TimerTime;

	if (OnTimerUpdateEvent.IsBound())
	{
		OnTimerUpdateEvent.Broadcast(LapTime);
	}
}

void ACheckpointGenerator::PrepareCheckpointData(const USplineComponent* TrackSpline, float CheckpointDistance)
{
	if (TrackSpline != nullptr)
	{
		const uint32 CheckpointsToSpawn = FMath::FloorToInt32(TrackSpline->GetSplineLength() / CheckpointDistance);
		CheckpointSpawnData.Reserve(CheckpointsToSpawn);

		for (uint32 CheckpointCounter = 0; CheckpointCounter < CheckpointsToSpawn; CheckpointCounter++)
		{
			const float Distance = CheckpointCounter * CheckpointDistance;
			const FVector Location = TrackSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			const FRotator Rotation = TrackSpline->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			FCheckpointSpawnData CheckpointData;
			CheckpointData.Location = Location;
			CheckpointData.Rotation = Rotation;

			CheckpointSpawnData.Emplace(CheckpointData);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ACheckpointGenerator::PrepareCheckpointData Track spline is nullptr"));
	}
}

void ACheckpointGenerator::SpawnCheckpointsBasedOnPreparedData(TArray<FCheckpointSpawnData>& CheckpointsData,
	FOnWorkFinished Callback)
{
	SpawnBatchCheckpointTimer.BindLambda([this, &CheckpointsData, Callback]
		{
			for (int32 i = 0; i < BatchSize && CheckpointSpawned < CheckpointsData.Num(); i++, CheckpointSpawned++)
			{
				const FCheckpointSpawnData& CheckpointData = CheckpointsData[CheckpointSpawned];
				ATrackCheckpoint* Checkpoint = GetWorld()->SpawnActor<ATrackCheckpoint>(TrackCheckpointClass, CheckpointData.Location, CheckpointData.Rotation);
				if (Checkpoint != nullptr)
				{
					Checkpoint->SetCheckpointIndex(CheckpointSpawned);
					Checkpoint->OnPawnOverlappedWTrackCheckpoint.AddUObject(this, &ACheckpointGenerator::OnCheckpointOverlapped);
					Checkpoint->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform,
						*FString::Printf(TEXT("TrackCheckpoint%d"), CheckpointSpawned));

					SpawnedTrackCheckpoints.Emplace(Checkpoint);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("ACheckpointGenerator::SpawnCheckpointsBasedOnPreparedData Failed to spawn checkpoint"));
				}
			}

			if (CheckpointSpawned < CheckpointsData.Num())
			{
				GetWorld()->GetTimerManager().SetTimerForNextTick(SpawnBatchCheckpointTimer);
			}

			if (CheckpointSpawned == CheckpointsData.Num())
			{
				CheckpointsData.Empty();

				if (Callback.IsBound())
				{
					Callback.Execute();
				}
			}
		});


	// Start the timer to process the first batch
	GetWorld()->GetTimerManager().SetTimerForNextTick(SpawnBatchCheckpointTimer);
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
	if (CheckpointIndex == TargetCheckpointIndex)
	{
		const ATrackCheckpoint* OverlappedCheckpoint = SpawnedTrackCheckpoints[CheckpointIndex].Get();
		if (OverlappedCheckpoint != nullptr)
		{
			OverlappedCheckpoint->SetMaterialToBasic();
		}

		TargetCheckpointIndex = (TargetCheckpointIndex + 1) % SpawnedTrackCheckpoints.Num();

		if (CheckpointIndex == 0)
		{
			if (OnLapFinishedEvent.IsBound())
			{
				OnLapFinishedEvent.Broadcast(LapTime);
			}

			UpdateTimer(0.0f);
		}

		const ATrackCheckpoint* TargetCheckpoint = SpawnedTrackCheckpoints[TargetCheckpointIndex].Get();
		if (TargetCheckpoint != nullptr)
		{
			TargetCheckpoint->SetMaterialToTarget();
		}
	}
}