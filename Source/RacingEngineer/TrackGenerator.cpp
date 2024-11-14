// Fill out your copyright notice in the Description page of Project Settings.


#include "TrackGenerator.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ATrackGenerator::ATrackGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* BaseRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(BaseRootComponent);

	SplineComponent = CreateDefaultSubobject<USplineComponent>("Spline");
	if (SplineComponent != nullptr)
	{
		SplineComponent->SetupAttachment(GetRootComponent());
	}
}

void ATrackGenerator::OnConstruction(const FTransform& Transform)
{
	CreateMeshOnSpline();
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

void ATrackGenerator::DoWork(const TArray<FColor>& HeightTextureColors, const FVector& VertScale, FOnWorkFinished Callback)
{
	const uint32 Height = sqrt(HeightTextureColors.Num());
	const uint32 Width = Height;

	TrackNodes = CreateTrack(HeightTextureColors, Width);

	if (SplineComponent != nullptr)
	{
		SplineComponent->ClearSplinePoints();
		CreateTrackSpline(HeightTextureColors, Height, Width, VertScale);
		SplineComponent->SetClosedLoop(true);
	}

	CreateMeshOnSpline();

	Super::DoWork(HeightTextureColors, VertScale, Callback);
}


#pragma region TextureToSpline

FVector2D ATrackGenerator::AddDirectionToPosition(const FVector2D& Vector2D, const Direction& Direction)
{
	FVector2D MoveVec;

	switch (Direction)
	{
	case Direction::UpLeft:
		MoveVec = FVector2D(-1.0, -1.0);
		break;
	case Direction::Up:
		MoveVec = FVector2D(0.0, -1.0);
		break;
	case Direction::UpRight:
		MoveVec = FVector2D(1.0, -1.0);
		break;
	case Direction::Right:
		MoveVec = FVector2D(1.0, 0.0);
		break;
	case Direction::DownRight:
		MoveVec = FVector2D(1.0, 1.0);
		break;
	case Direction::Down:
		MoveVec = FVector2D(0.0, 1.0);
		break;
	case Direction::DownLeft:
		MoveVec = FVector2D(-1.0, 1.0);
		break;
	case Direction::Left:
		MoveVec = FVector2D(-1.0, 0.0);
		break;
	default:
		MoveVec = FVector2D(0.0, 0.0);
		break;
	}

	return Vector2D + MoveVec;
}

TArray<FTrackNode> ATrackGenerator::CreateTrack(const TArray<FColor>& HeightTextureColors, const uint32 TextureWidth)
{
	TArray<FTrackNode> TrackNodes;

	FTrackNode trackNode = FindFirstTrackNode(HeightTextureColors, TextureWidth);
	TrackNodes.Add(trackNode);
	constexpr uint8 MinNumberOfNodes = 3;

	for (uint8 i = 0; i < MinNumberOfNodes; i++)
	{
		trackNode = FindNextTrackNode(HeightTextureColors, TextureWidth, trackNode);
		TrackNodes.Add(trackNode);
	}

	while (ShouldFindAnotherTrackNode(TrackNodes))
	{
		trackNode = FindNextTrackNode(HeightTextureColors, TextureWidth, trackNode);
		TrackNodes.Add(trackNode);
	}

	return TrackNodes;
}

FTrackNode ATrackGenerator::FindFirstTrackNode(const TArray<FColor>& HeightTextureColors, const uint32 TextureWidth)
{
	for (uint32 y = TextureWidth * 3 / 4; y < TextureWidth; y++)
	{
		for (uint32 x = 0; x < TextureWidth / 2; x++)
		{
			FColor pixelColor = HeightTextureColors[y * TextureWidth + x];

			if (pixelColor.A < 255)
			{
				return FTrackNode(FVector2D(x, y), Direction::Left);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ATrackGenerator::FindFirstTrackNode Couldn't find the first node"));
	return FTrackNode(FVector2D(0, TextureWidth - 1), Direction::Left);
}

FTrackNode ATrackGenerator::FindNextTrackNode(const TArray<FColor>& HeightTextureColors, const uint32 TextureWidth, const FTrackNode& CurrentNode)
{
	Direction currentDirection = CurrentNode.PrevPointDirection + 1;

	for (int i = 0; i < 7; i++)
	{
		FVector2D NeighbourPos = AddDirectionToPosition(CurrentNode.Position, currentDirection);

		if (NeighbourPos.X > 0 && NeighbourPos.X < TextureWidth &&
			NeighbourPos.Y > 0 && NeighbourPos.Y < TextureWidth)
		{
			FColor pixelColor = HeightTextureColors[NeighbourPos.Y * TextureWidth + NeighbourPos.X];

			if (pixelColor.A < 255)
			{
				break;
			}
		}

		currentDirection = currentDirection + 1;
	}

	Direction directionToOrigin = currentDirection + 4;
	return FTrackNode(AddDirectionToPosition(CurrentNode.Position, currentDirection), directionToOrigin);
}

bool ATrackGenerator::ShouldFindAnotherTrackNode(const TArray<FTrackNode>& TrackNodes)
{
	check(TrackNodes.Num() >= 3);

	FTrackNode firstNode = TrackNodes[0];
	FTrackNode lastNode = TrackNodes.Last();

	return FVector2D::DistSquared(firstNode.Position, lastNode.Position) > 2;
}

#pragma endregion

void ATrackGenerator::CreateTrackSpline(const TArray<FColor>& HeightTextureColors, const uint32 Height, const uint32 Width, const FVector& VertScale)
{
	for (FTrackNode& TrackNode : TrackNodes)
	{
		uint8 HeightValue = HeightTextureColors[TrackNode.Position.Y * Width + TrackNode.Position.X].R;
		FVector SplinePos =
		{
			(TrackNode.Position.X - Width / 2) * VertScale.X,
			(TrackNode.Position.Y - Height / 2) * VertScale.Y,
			(HeightValue - 255 / 2) / 255.0 * VertScale.Z
		};

		SplineComponent->AddSplinePoint(SplinePos, ESplineCoordinateSpace::Local);
	}
}

#pragma region SpawningMesh

FVector ATrackGenerator::GetMeshLength() const
{
	if (TrackMesh != nullptr)
	{
		const FBox MeshBox = TrackMesh->GetBoundingBox();

		return { MeshBox.Max - MeshBox.Min };
	}

	return FVector::Zero();
}

void ATrackGenerator::SpawnMeshPerSplinePoint(const uint32 NumberOfSplinePoints, FVector MeshOffset)
{
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

		SplineMeshComponent->SetStartAndEnd(StartPos - PosDirectionOffset, StartTangent, EndPos - PosDirectionOffset, EndTangent);
		//SplineMeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
}

void ATrackGenerator::SpawnMeshBasedOnMeshLength(const FVector& MeshSize)
{
	const float MeshLength = MeshSize.X;
	uint32 MeshesToSpawnNumber = FMath::FloorToInt32(SplineComponent->GetSplineLength() / MeshLength);
	for (uint32 MeshCounter = 0; MeshCounter < MeshesToSpawnNumber; MeshCounter++)
	{
		FVector StartPos = SplineComponent->GetLocationAtDistanceAlongSpline(MeshLength * MeshCounter, ESplineCoordinateSpace::Local);
		FVector EndPos = SplineComponent->GetLocationAtDistanceAlongSpline(MeshLength * (MeshCounter + 1), ESplineCoordinateSpace::Local);
		const FVector StartTangent = SplineComponent->GetTangentAtDistanceAlongSpline(MeshLength * MeshCounter, ESplineCoordinateSpace::Local);
		const FVector EndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(MeshLength * (MeshCounter + 1), ESplineCoordinateSpace::Local);

		// Calculate the direction of the curve
		FVector startOffset = UKismetMathLibrary::GetRightVector(SplineComponent->GetRotationAtDistanceAlongSpline(MeshLength * MeshCounter, ESplineCoordinateSpace::Local)).GetSafeNormal();
		FVector endOffset = UKismetMathLibrary::GetRightVector(SplineComponent->GetRotationAtDistanceAlongSpline(MeshLength * (MeshCounter + 1), ESplineCoordinateSpace::Local)).GetSafeNormal();

		startOffset *= MeshSize.Y / 2;
		endOffset *= MeshSize.Y / 2;

		const FName SplineMeshName = *FString::Printf(TEXT("TrackMesh%d"), MeshCounter);
		USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), SplineMeshName);

		SplineMeshComponent->SetStaticMesh(TrackMesh);
		SplineMeshComponent->SetMobility(EComponentMobility::Stationary);
		SplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
		SplineMeshComponent->AttachToComponent(SplineComponent, FAttachmentTransformRules::KeepRelativeTransform);

		//SplineMeshComponent->SetStartAndEnd(StartPos - startOffset, StartTangent, EndPos - endOffset, EndTangent);
		SplineMeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMeshComponent->SetRelativeScale3D(MeshScale);
;
		/*FRotator testRot = SplineComponent->GetRotationAtDistanceAlongSpline(MeshLength * MeshCounter, ESplineCoordinateSpace::Local);

		if (ActorToSpawn.Get() != nullptr)
		{
			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorToSpawn, StartPos + GetTransform().GetLocation(), testRot);
			if (SpawnedActor)
			{
				SpawnedActor->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
				SpawnedActor->SetActorLabel(FString::Printf(TEXT("Tester%d"), MeshCounter));
			}
		}*/
	}
	
}

void ATrackGenerator::CreateMeshOnSpline()
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

		/*TArray<AActor*> Testers;
		GetAttachedActors(Testers);
		int number = Testers.Num();
		for (int i = 0; i < number; i++)
		{
			GetWorld()->DestroyActor(Testers[i]);
		}*/

		const uint32 NumberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();
		FVector MeshOffset = GetMeshLength() / 2.0;

		//SpawnMeshPerSplinePoint(NumberOfSplinePoints, MeshOffset);
		SpawnMeshBasedOnMeshLength(GetMeshLength());
	}
}

#pragma endregion