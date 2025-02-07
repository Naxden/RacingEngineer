// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainGenerator.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "RacingEngineerGameInstance.h"
#include "TrackGenerator.h"
#include "Components/SplineComponent.h"
#include "Runtime/Foliage/Public/FoliageInstancedStaticMeshComponent.h"

using UE::Math::TVector;
// Sets default values
ATerrainGenerator::ATerrainGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	if (ProceduralMesh != nullptr)
	{
		ProceduralMesh->SetCanEverAffectNavigation(false);
		SetRootComponent(ProceduralMesh);
	}

	TerrainWalls.Reserve(4);
	
	for (uint8 i = 0; i < 4; i++)
	{
		UStaticMeshComponent* Wall = CreateDefaultSubobject<UStaticMeshComponent>(FName(ToString(static_cast<EWall>(i)) + "Wall"));

		if (Wall != nullptr)
		{
			TerrainWalls.Emplace(Wall);

			Wall->SetCanEverAffectNavigation(false);
			Wall->SetupAttachment(GetRootComponent());
			Wall->SetVisibility(false);
		}
	}

	GrassFoliageComponent = CreateDefaultSubobject<UFoliageInstancedStaticMeshComponent>(TEXT("GrassFoliageInstancedStaticMesh"));
	if (GrassFoliageComponent != nullptr)
	{
		GrassFoliageComponent->SetupAttachment(GetRootComponent());
		GrassFoliageComponent->SetCanEverAffectNavigation(false);
		GrassFoliageComponent->SetVisibility(true);
	}

	RockInstancedStaticMeshComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RockInstancedStaticMesh"));
	if (RockInstancedStaticMeshComponent != nullptr)
	{
		RockInstancedStaticMeshComponent->SetupAttachment(GetRootComponent());
		RockInstancedStaticMeshComponent->SetVisibility(true);
	}

	TreesInstancedStaticMeshComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreesInstancedStaticMesh"));
	if (TreesInstancedStaticMeshComponent != nullptr)
	{
		TreesInstancedStaticMeshComponent->SetupAttachment(GetRootComponent());
		TreesInstancedStaticMeshComponent->SetVisibility(true);
	}
}

// Called when the game starts or when spawned
void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void ATerrainGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATerrainGenerator::DoWork(const FWorkerData& Data, const FOnWorkFinished Callback)
{
	const uint64 VerticesNum = Data.TextureWidth * Data.TextureHeight;
	const float VertScaleXYNum = Data.VertScale.X * Data.VertScale.Y;
	GrassFoliageProbability *= VertScaleXYNum / VerticesNum;
	RocksProbability *= VertScaleXYNum / VerticesNum;
	TreesProbability *= VertScaleXYNum / VerticesNum;

	URacingEngineerGameInstance* RacingEngineerGameInstance = Cast<URacingEngineerGameInstance>(GetGameInstance());
	if (RacingEngineerGameInstance != nullptr)
	{
		if (RacingEngineerGameInstance->bLightWeightMode)
		{
			GrassFoliageProbability /= 3.0;
			RocksProbability /= 3.0;
			TreesProbability /= 3.0;
		}
	}

	GrassFoliageLocations.Reserve(VerticesNum * GrassFoliageProbability);
	RocksLocations.Reserve(VerticesNum * RocksProbability);
	TreesLocations.Reserve(VerticesNum * TreesProbability);

	CreateTerrain(Data);

	AsyncTask(ENamedThreads::GameThread, [this, Data, Callback]
	{
		ProceduralMesh->CreateMeshSection(
				0,
				Vertices,
				TriangleIndices,
				Normals,
				UV,
				TArray<FColor>(),
				TArray<FProcMeshTangent>(),
				true);

		ProceduralMesh->SetMaterial(0, MeshMaterial);
		ProceduralMesh->SetCanEverAffectNavigation(true);

		SetupWalls(Data.TextureWidth, Data.TextureHeight, Data.VertScale);

		SpawnInstancedMeshes(GrassFoliageLocations, GrassFoliageComponent, false);
		SpawnInstancedMeshes(RocksLocations, RockInstancedStaticMeshComponent, true);
		SpawnInstancedMeshes(TreesLocations, TreesInstancedStaticMeshComponent, true);

		if (Callback.IsBound())
		{
			Callback.Execute();
		}
	});
}


void ATerrainGenerator::CreateTerrain(const FWorkerData& Data)
{
	UV = CalculateUVs(Data.TextureWidth, Data.TextureHeight);
	Vertices = CalculateVertices(Data.TextureWidth, Data.TextureHeight, Data.VertScale);
	AlterVerticesHeight(Vertices, Data);
	TriangleIndices = CalculateTriangles(Data.TextureWidth, Data.TextureHeight);

	if (UseBuiltInNormalsAndTangents)
	{
		TArray<FProcMeshTangent> Tangents;
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, TriangleIndices, UV, Normals, Tangents);
	}
	else
	{
		Normals = CalculateNormals(Vertices, TriangleIndices, Data.TextureWidth, Data.TextureHeight);
	}
}

void ATerrainGenerator::AlterVerticesHeight(TArray<FVector>& outVertices, const FWorkerData& Data)
{
	for (uint32 y = 0; y < Data.TextureHeight; y++)
	{
		for (uint32 x = 0; x < Data.TextureWidth; x++)
		{
			constexpr uint8 Offset = 127;
			FVector& CurrentVert = outVertices[y * Data.TextureWidth + x];

			const uint8 HeightValue = Data.HeightData[y * Data.TextureWidth + x];
			;
			CurrentVert.Z += (HeightValue - Offset) / 255.0 * Data.VertScale.Z;
			
			float MeshWidth = 0.0f;
			float MeshHeight = 0.0f;

			if (TrackGenerator.IsValid())
			{
				const FVector MeshSize = TrackGenerator->GetTrackMeshSize();
				MeshWidth = MeshSize.Y;
				MeshHeight = MeshSize.Z * MeshHeightScalar;
			}

			if (Data.TrackSpline != nullptr)
			{
				const FVector ClosestSplinePos = Data.TrackSpline->FindLocationClosestToWorldLocation(CurrentVert, ESplineCoordinateSpace::World);
				const float Distance = FVector::Dist(CurrentVert, ClosestSplinePos);
				const float MeshOffset = MeshWidth * 0.35f;

				// If it's under the track mesh with some offset
				if (Distance <= MeshWidth / 2 + MeshOffset)
				{
					CurrentVert.Z = ClosestSplinePos.Z - MeshHeight;
				}
				// If it's near the track mesh but not under it
				else if (Distance <= MeshWidth + MeshOffset)
				{
					const float Alpha = (MeshWidth + MeshOffset) / Distance - 1.0f;
					CurrentVert.Z = FMath::Lerp(CurrentVert.Z, ClosestSplinePos.Z - MeshHeight, Alpha);
				}
				else
				{
					TryAddFoliageLocation(CurrentVert);
				}
			}
		}
	}
}

void ATerrainGenerator::TryAddFoliageLocation(const FVector& LocationToSpawn)
{
	if (FMath::FRand() < GrassFoliageProbability && GrassFoliageLocations.Num() < GrassFoliageLocations.Max())
	{
		GrassFoliageLocations.Emplace(LocationToSpawn);
	}
	else if (FMath::FRand() < RocksProbability && RocksLocations.Num() < RocksLocations.Max())
	{
		RocksLocations.Emplace(LocationToSpawn);
	}
	else if (FMath::FRand() < TreesProbability && TreesLocations.Num() < TreesLocations.Max())
	{
		TreesLocations.Emplace(LocationToSpawn);
	}
}

void ATerrainGenerator::SetupWalls(const uint32 TextureWidth, const uint32 TextureHeight, const FVector& VertScale)
{
	double Width = (TextureWidth - 1) * VertScale.X;
	double Height = (TextureHeight - 1) * VertScale.Y;

	if (TerrainWallMesh != nullptr)
	{
		const FVector MeshSize = TerrainWallMesh->GetBounds().BoxExtent;
		
		if (TerrainWalls.Num() == 4)
		{
			for (uint8 i = 0; i < 4; i++)
			{
				EWall CurrentWall = static_cast<EWall>(i);

				UStaticMeshComponent* Wall = TerrainWalls[i];
				if (Wall != nullptr)
				{
					Wall->SetStaticMesh(TerrainWallMesh);
					Wall->SetVisibility(false);

					switch (CurrentWall)
					{
					case EWall::North:
						Wall->SetWorldScale3D(FVector(Width / (MeshSize.X * 2), 1, VertScale.Z / MeshSize.Z * 1.2));
						Wall->SetWorldLocationAndRotation(
							GetActorLocation() + FVector(0, -Height / 2, -VertScale.Z / 2),
							GetActorRotation()
						);
						break;

					case EWall::East:
						Wall->SetWorldScale3D(FVector(Height / (MeshSize.X * 2), 1, VertScale.Z / MeshSize.Z * 1.2));
						Wall->SetWorldLocationAndRotation(
							GetActorLocation() + FVector(Width / 2, 0, -VertScale.Z / 2),
							GetActorRotation() + FRotator(0, 90, 0)
						);
						break;

					case EWall::South:
						Wall->SetWorldScale3D(FVector(Width / (MeshSize.X * 2), 1, VertScale.Z / MeshSize.Z * 1.2));
						Wall->SetWorldLocationAndRotation(
							GetActorLocation() + FVector(0.0, Height / 2, -VertScale.Z / 2),
							GetActorRotation() + FRotator(0, 180, 0)
						);
						break;

					case EWall::West:
						Wall->SetWorldScale3D(FVector(Height / (MeshSize.X * 2), 1, VertScale.Z / MeshSize.Z * 1.2));
						Wall->SetWorldLocationAndRotation(
							GetActorLocation() + FVector(-Width / 2, 0, -VertScale.Z / 2),
							GetActorRotation() + FRotator(0, 270, 0)
						);
						break;
					}

					Wall->SetMobility(EComponentMobility::Stationary);
				}
				else 
				{
					UE_LOG(LogTemp, Error, TEXT("ATerrainGenerator::SetupWalls %sWall is nullptr"), *ToString(CurrentWall));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ATerrainGenerator::SetupWalls TerrainWalls array is not initialized properly"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATerrainGenerator::SetupWalls TerrainWallMesh is nullptr"));
	}
}

void ATerrainGenerator::SpawnInstancedMeshes(TArray<FVector>& Locations, UInstancedStaticMeshComponent* InstancedStaticMeshComponent, bool bUpdateNavigation)
{
	if (InstancedStaticMeshComponent != nullptr)
	{
		TArray<FTransform> Transforms;
		Transforms.Reserve(Locations.Num());

		for (const FVector& Location : Locations)
		{
			float RandomScale = FMath::RandRange(0.5, 3.0);
			float RandomYaw = FMath::RandRange(0, 360);
			FTransform Transform(Location);

			Transform.SetScale3D(FVector(RandomScale));
			Transform.SetRotation(FRotator(0, RandomYaw, 0).Quaternion());

			Transforms.Emplace(Transform);
		}

		InstancedStaticMeshComponent->AddInstances(Transforms, false, true);
		Locations.Empty();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATerrainGenerator::SpawnFoliage GrassFoliageComponent is nullptr"));
	}
}

TArray<FVector> ATerrainGenerator::CalculateVertices(const uint32 Width, const uint32 Height, const FVector& VertScale) const
{
	TArray<FVector> Verts;
	const uint64 VertCount = Width * Height;
	Verts.Reserve(VertCount);

	TVector LocalPosition = GetTransform().GetLocation();

	LocalPosition -= FVector(Width / 2.0 * VertScale.X, Height/ 2.0 * VertScale.Y, 0.0);

	for (uint32 y = 0; y < Height; y++)
	{
		for (uint32 x = 0; x < Width; x++)
		{
			Verts.Emplace
			(
			x * VertScale.X + LocalPosition.X,
			y * VertScale.Y + LocalPosition.Y,
				LocalPosition.Z
			);
		}
	}

	return Verts;
}

TArray<FVector2D> ATerrainGenerator::CalculateUVs(const uint32 Width, const uint32 Height)
{
	TArray<FVector2D> UVs;
	UVs.Reserve(Width * Height);
	for (uint32 y = 0; y < Height; y++)
	{
		for (uint32 x = 0; x < Width; x++)
		{
			UVs.Emplace(x / (Width - 1.0), y / (Height - 1.0));
		}
	}
	return UVs;
}

TArray<int32> ATerrainGenerator::CalculateTriangles(const uint32 Width, const uint32 Height)
{
	const uint32 TriangleNodesCount = (Width - 1) * (Height - 1) * 2 * 3;
	TArray<int32> TriangleNodes;
	TriangleNodes.Reserve(TriangleNodesCount);

	for (uint32 y = 0; y < Height - 1; y++)
	{
		for (uint32 x = 0; x < Width - 1; x++)
		{
			TriangleNodes.Emplace(x + y * Width);
			TriangleNodes.Emplace(x + (y + 1) * Width);
			TriangleNodes.Emplace(x + 1 + y * Width);

			TriangleNodes.Emplace(x + 1 + y * Width);
			TriangleNodes.Emplace(x + (y + 1) * Width);
			TriangleNodes.Emplace(x + 1 + (y + 1) * Width);
		}
	}

	return TriangleNodes;
}

FORCEINLINE void NormalizeVector(FVector& v)
{
	if (!v.Normalize())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to Normalize %s"), *v.ToString());
	}
}

FVector ATerrainGenerator::GetNormal(const FVector& V0, const FVector& V1, const FVector& V2)
{
	const FVector Edge1 = V1 - V0;
	const FVector Edge2 = V2 - V0;

	FVector crossVector = FVector::CrossProduct(Edge2, Edge1);

	NormalizeVector(crossVector);

	return crossVector;
}

TArray<FVector> ATerrainGenerator::CalculateNormals(const TArray<FVector>& Verts, const TArray<int32>& Triangles, const uint32 Width, const uint32 Height)
{
	const uint32 NormalCount = Width * Height;
	const uint32 TriangleIndicesCount = (Width - 1) * (Height - 1) * 2 * 3;

	check(Verts.Num() == NormalCount)
	check(Triangles.Num() == TriangleIndicesCount)

	TArray<FVector> Normals;
	Normals.Empty(NormalCount);
	Normals.AddZeroed(NormalCount);

	for (uint32 i = 0; i < TriangleIndicesCount; i += 3)
	{
		const uint32 I0 = Triangles[i];
		const uint32 I1 = Triangles[i + 1];
		const uint32 I2 = Triangles[i + 2];
		
		FVector Normal = GetNormal
		(
			Verts[I0],
			Verts[I1],
			Verts[I2]
		);

		Normals[I0] += Normal;
		Normals[I1] += Normal;
		Normals[I2] += Normal;
	}

	for (size_t i = 0; i < NormalCount; i++)
	{
		NormalizeVector(Normals[i]);
	}

	return Normals;
}