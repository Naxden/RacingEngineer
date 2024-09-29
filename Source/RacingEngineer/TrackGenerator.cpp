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
		for (uint32 SplineIndex = 0; SplineIndex < NumberOfSplinePoints; SplineIndex++)
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

			SplineMeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
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
	TrackMapArray.Empty(64 * 64);
	TrackMapArray.AddZeroed(64 * 64);

	if (SplineComponent != nullptr)
	{
		TQueue<FVector> TrackMapQueue;
		//// Creating a track array
		//for (uint32 y = 0; y < 64; y++)
		//{
		//	for (uint32 x = 0; x < 64; x++)
		//	{
		//		if (x * x + y * y > 250 && x * x + y * y < 260) // x * x + y * y == 256
		//		{
		//			TrackMapArray[x + y * 64] = 1;
		//		}
		//	}
		//}
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
		constexpr double VertSpacing = 250.0;
		FVector SplinePos;
		while (TrackMapQueue.Dequeue(SplinePos))
		{
			SplineComponent->AddSplinePoint(SplinePos * VertSpacing, ESplineCoordinateSpace::Local);
		}

		CreateMeshToSpline();
	}

}



// Called every frame
void ATrackGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

