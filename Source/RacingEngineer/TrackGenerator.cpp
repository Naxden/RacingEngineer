// Fill out your copyright notice in the Description page of Project Settings.


#include "TrackGenerator.h"

#include "TerrainGenerator.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Async/Async.h"

// Sets default values
ATrackGenerator::ATrackGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ATrackGenerator::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ATrackGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATrackGenerator::DoWork(const FWorkerData& Data, const FOnWorkFinished Callback)
{
	PrepareTrackSplineMeshData(Data.TrackSpline, GetTrackMeshSize());

	AsyncTask(ENamedThreads::GameThread, [this, Callback]
	{
		SpawnTrackBasedOnPreparedData(TrackMeshSpawnData, Callback);
	});
}

FVector ATrackGenerator::GetTrackMeshSize() const
{
	if (TrackMesh != nullptr)
	{
		return TrackMesh->GetBoundingBox().GetSize();
	}

	return FVector::ZeroVector;
}

#pragma region SpawningMesh

void ATrackGenerator::SpawnMeshPerSplinePoint(const USplineComponent* TrackSpline, const FVector& MeshOffset)
{
	const uint32 NumberOfSplinePoints = TrackSpline->GetNumberOfSplinePoints();
	//																### remove to loop
	for (uint32 SplineIndex = 0; SplineIndex < NumberOfSplinePoints - 1; SplineIndex++)
	{
		const FName SplineMeshName = *FString::Printf(TEXT("TrackMesh%d"), SplineIndex);
		USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), SplineMeshName);

		SplineMeshComponent->SetStaticMesh(TrackMesh);
		SplineMeshComponent->SetMobility(EComponentMobility::Stationary);
		SplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
		SplineMeshComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

		const FVector StartPos = TrackSpline->GetLocationAtSplinePoint(SplineIndex, ESplineCoordinateSpace::Local);
		const FVector EndPos = TrackSpline->GetLocationAtSplinePoint((SplineIndex + 1) % NumberOfSplinePoints, ESplineCoordinateSpace::Local);
		const FVector StartTangent = TrackSpline->GetTangentAtSplinePoint(SplineIndex, ESplineCoordinateSpace::Local);
		const FVector EndTangent = TrackSpline->GetTangentAtSplinePoint((SplineIndex + 1) % NumberOfSplinePoints, ESplineCoordinateSpace::Local);

		FVector PosDirectionOffset = EndTangent - StartPos;
		PosDirectionOffset = FVector::CrossProduct(PosDirectionOffset, FVector::UpVector);
		PosDirectionOffset.Normalize();
		PosDirectionOffset *= MeshOffset.Y;

		SplineMeshComponent->SetStartAndEnd(StartPos - PosDirectionOffset, StartTangent, EndPos - PosDirectionOffset, EndTangent);
		//SplineMeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
}

void ATrackGenerator::SpawnMeshBasedOnMeshLength(const USplineComponent* TrackSpline, const FVector& MeshSize)
{
	const float MeshLength = MeshSize.X;

	if (TrackSpline != nullptr)
	{
		const uint32 MeshesToSpawn = FMath::FloorToInt32(TrackSpline->GetSplineLength() / MeshLength);
		for (uint32 MeshCounter = 0; MeshCounter < MeshesToSpawn; MeshCounter++)
		{
			const float StartDistance = MeshLength * MeshCounter;
			const float EndDistance = MeshCounter + 1 != MeshesToSpawn ? MeshLength * (MeshCounter + 1) : TrackSpline->GetSplineLength();

			const FVector StartPos = TrackSpline->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
			const FVector EndPos = TrackSpline->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
			const FVector StartTangent = TrackSpline->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
			const FVector EndTangent = TrackSpline->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);

			const FName SplineMeshName = *FString::Printf(TEXT("TrackMesh%d"), MeshCounter);
			USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), SplineMeshName);

			SplineMeshComponent->SetStaticMesh(TrackMesh);
			SplineMeshComponent->SetMobility(EComponentMobility::Stationary);
			SplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
			SplineMeshComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			SplineMeshComponent->SetSplineUpDir(FVector::UpVector);

			SplineMeshComponent->SetStartAndEnd(StartPos, StartTangent * TangentScalar, EndPos, EndTangent * TangentScalar);
			SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
			SplineMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATrackGenerator::SpawnMeshBasedOnMeshLength TrackSpline is nullptr"));
	}
}


void ATrackGenerator::PrepareTrackSplineMeshData(const USplineComponent* TrackSpline, const FVector& MeshSize)
{
	const float MeshLength = MeshSize.X;

	if (TrackSpline != nullptr)
	{
		const uint32 MeshesToSpawn = FMath::FloorToInt32(TrackSpline->GetSplineLength() / MeshLength);
		TrackMeshSpawnData.Reserve(MeshesToSpawn);

		for (uint32 MeshCounter = 0; MeshCounter < MeshesToSpawn; MeshCounter++)
		{
			const float StartDistance = MeshLength * MeshCounter;
			const float EndDistance = MeshCounter + 1 != MeshesToSpawn ? MeshLength * (MeshCounter + 1) : TrackSpline->GetSplineLength();

			FTrackSplineSpawnData TrackSplineMeshData;

			TrackSplineMeshData.StartPos = TrackSpline->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
			TrackSplineMeshData.EndPos = TrackSpline->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
			TrackSplineMeshData.StartTangent = TrackSpline->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
			TrackSplineMeshData.EndTangent = TrackSpline->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);

			TrackSplineMeshData.SplineMeshName = *FString::Printf(TEXT("TrackMesh%d"), MeshCounter);

			TrackMeshSpawnData.Emplace(TrackSplineMeshData);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATrackGenerator::SpawnMeshBasedOnMeshLength TrackSpline is nullptr"));
	}
}

void ATrackGenerator::SpawnTrackBasedOnPreparedData(TArray<FTrackSplineSpawnData>& TrackSpawnData, FOnWorkFinished Callback)
{
	SpawnBatchTrackMeshTimer.BindLambda([this, &TrackSpawnData, Callback]()
		{
			for (int32 i = 0; i < BatchSize && TrackMeshSpawned < TrackSpawnData.Num(); i++, TrackMeshSpawned++)
			{
				const FTrackSplineSpawnData& TrackSplineMeshData = TrackSpawnData[TrackMeshSpawned];

				USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this,
					USplineMeshComponent::StaticClass(), TrackSplineMeshData.SplineMeshName);

				SplineMeshComponent->SetStaticMesh(TrackMesh);
				SplineMeshComponent->SetMobility(EComponentMobility::Stationary);
				SplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
				SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
				SplineMeshComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				SplineMeshComponent->SetSplineUpDir(FVector::UpVector);
				SplineMeshComponent->SetStartAndEnd(TrackSplineMeshData.StartPos, TrackSplineMeshData.StartTangent * TangentScalar,
					TrackSplineMeshData.EndPos, TrackSplineMeshData.EndTangent * TangentScalar);
				SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
				SplineMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
			}

			if (TrackMeshSpawned < TrackSpawnData.Num())
			{
				GetWorld()->GetTimerManager().SetTimerForNextTick(SpawnBatchTrackMeshTimer);
			}

			if (TrackMeshSpawned == TrackSpawnData.Num())
			{
				TrackMeshSpawnData.Empty();

				if (Callback.IsBound())
				{
					Callback.Execute();
				}
			}
		});

	// Start the timer to process the first batch
	GetWorld()->GetTimerManager().SetTimerForNextTick(SpawnBatchTrackMeshTimer);
}

void ATrackGenerator::CreateMeshOnSpline(const USplineComponent* TrackSplineComponent)
{
	if (TrackSplineComponent != nullptr && TrackMesh != nullptr)
	{
		auto& MeshComponents = GetRootComponent()->GetAttachChildren();
		UE_LOG(LogTemp, Log, TEXT("Number of Components attached this RootComponent: %d BEFORE"), MeshComponents.Num());

		for (int i = 0; i < MeshComponents.Num(); i++)
		{
			USceneComponent* MeshComponent = MeshComponents[i].Get();
			if (MeshComponent != nullptr && MeshComponent->IsRegistered())
			{
				USplineMeshComponent* SplineMeshComponent = Cast<USplineMeshComponent>(MeshComponent);
				if (SplineMeshComponent != nullptr)
				{
					SplineMeshComponent->DestroyComponent();
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("Number of Components attached to TrackSpline: %d After"), MeshComponents.Num());

		SpawnMeshBasedOnMeshLength(TrackSplineComponent, GetTrackMeshSize());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATrackGenerator::CreateMeshOnSpline TrackSplineComponent or TrackMesh is nullptr"));
	}
}

#pragma endregion