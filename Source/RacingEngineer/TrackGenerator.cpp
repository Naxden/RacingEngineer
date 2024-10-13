// Fill out your copyright notice in the Description page of Project Settings.


#include "TrackGenerator.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

// Sets default values
ATrackGenerator::ATrackGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>("Spline");
	if (SplineComponent != nullptr)
	{
		SplineComponent->SetupAttachment(GetRootComponent());
	}
}

FVector ATrackGenerator::GetMeshLength() const
{
	if (TrackMesh != nullptr)
	{
		const FBox MeshBox = TrackMesh->GetBoundingBox();

		return { MeshBox.Max - MeshBox.Min };
	}

	return FVector::Zero();
}

void ATrackGenerator::CreateMeshToSpline()
{
	if (SplineComponent != nullptr && TrackMesh != nullptr)
	{
		auto& MeshComponents = SplineComponent->GetAttachChildren();
		UE_LOG(LogTemp, Warning, TEXT("Number of meshes attached: %d"), MeshComponents.Num());

		while (MeshComponents.Num() > 0)
		{
			USceneComponent* MeshComponent = MeshComponents[0].Get();
			if (MeshComponent != nullptr && MeshComponent->IsRegistered())
			{
				MeshComponent->DestroyComponent();
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Number of meshes attached: %d"), MeshComponents.Num());


		const uint32 NumberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();
		FVector MeshOffset = GetMeshLength() / 2.0;
		SplineTangents.Empty(NumberOfSplinePoints);

		//																### remove to loop
		for (uint32 SplineIndex = 0; SplineIndex < NumberOfSplinePoints - 1; SplineIndex++)
		{
			const FName SplineMeshName = *FString::Printf(TEXT("TrackMesh%d"), SplineIndex);
			USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), SplineMeshName);

			SplineMeshComponent->SetStaticMesh(TrackMesh);
			SplineMeshComponent->SetMobility(EComponentMobility::Stationary);
			SplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
			SplineMeshComponent->AttachToComponent(SplineComponent, FAttachmentTransformRules::KeepRelativeTransform);

			const FVector StartPos = SplineComponent->GetLocationAtSplinePoint(SplineIndex, ESplineCoordinateSpace::Local);
			const FVector EndPos = SplineComponent->GetLocationAtSplinePoint((SplineIndex + 1) % NumberOfSplinePoints, ESplineCoordinateSpace::Local);
			const FVector StartTangent = SplineComponent->GetTangentAtSplinePoint(SplineIndex, ESplineCoordinateSpace::Local);
			const FVector EndTangent = SplineComponent->GetTangentAtSplinePoint((SplineIndex + 1) % NumberOfSplinePoints, ESplineCoordinateSpace::Local);

			FVector PosDirectionOffset = EndTangent - StartPos;
			PosDirectionOffset = FVector::CrossProduct(PosDirectionOffset, FVector::UpVector);
			PosDirectionOffset.Normalize();
			PosDirectionOffset *= MeshOffset.Y;

			SplineTangents.Add(StartTangent);

			SplineMeshComponent->SetStartAndEnd(StartPos - PosDirectionOffset, StartTangent, EndPos - PosDirectionOffset, EndTangent);
			//SplineMeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
			SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		}
	}
}

void ATrackGenerator::OnConstruction(const FTransform& Transform)
{
	CreateMeshToSpline();
}


// Called when the game starts or when spawned
void ATrackGenerator::BeginPlay()
{
	Super::BeginPlay();

	TArray<uint8> TrackMapArray;
	constexpr uint32 Size = 64;
	TrackMapArray.Empty(Size * Size);
	TrackMapArray.AddZeroed(Size * Size);

	if (SplineComponent != nullptr)
	{
		TQueue<FVector> TrackMapQueue;
		// Fixed Spline Pos queue
		TrackMapQueue.Enqueue({ 16, 16, 0 });
		//TrackMapQueue.Enqueue({ 32, 16, 0 });
		TrackMapQueue.Enqueue({ 48, 16, 0 });
		//TrackMapQueue.Enqueue({ 48, 32, 0 });
		TrackMapQueue.Enqueue({ 48, 48, 0 });
		//TrackMapQueue.Enqueue({ 32, 48, 0 });
		TrackMapQueue.Enqueue({ 16, 48, 0 });
		TrackMapQueue.Enqueue({ 16, 32, 0 });

		SplineComponent->ClearSplinePoints();
		// Adding spline points to the array
		FVector SplinePos;
		while (TrackMapQueue.Dequeue(SplinePos))
		{
			SplinePos = (SplinePos - Size / 2) * VertSpacing;
			SplinePos.Z = 0.0;
			SplineComponent->AddSplinePoint(SplinePos, ESplineCoordinateSpace::Local);
		}

		CreateMeshToSpline();
	}
}

// Called every frame
void ATrackGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

