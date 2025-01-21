// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorkerActor.h"
#include "GameFramework/Actor.h"
#include "TerrainGenerator.generated.h"

class UFoliageInstancedStaticMeshComponent;
class ATrackGenerator;
class USplineComponent;
struct FProcMeshTangent;
class UProceduralMeshComponent;

UENUM()
enum class EColorChannel : uint8
{
	Red,
	Green,
	Blue,
	Alpha
};

UENUM()
enum class EWall : uint8
{
	North,
	East,
	South,
	West
};

FORCEINLINE FString ToString(EWall Wall)
{
	switch (Wall)
	{
	case EWall::North:
		return "North";
	case EWall::East:
		return "East";
	case EWall::South:
		return "South";
	case EWall::West:
		return "West";
	default:
		return "Unknown";
	}
}

UCLASS()
class RACINGENGINEER_API ATerrainGenerator : public AWorkerActor
{
	GENERATED_BODY()
	
public:

	UFUNCTION()
	static TArray<int32> CalculateTriangles(uint32 Size);

	UFUNCTION()
	static FVector GetNormal(const FVector& V0, const FVector& V1, const FVector& V2);

	UFUNCTION()
	static TArray<FVector> CalculateNormals(const TArray<FVector>& Verts, const TArray<int32> Triangles, const uint32 Size);

	UFUNCTION()
	static TArray<FVector2D> CalculateUVs(const uint32 Size);

	ATerrainGenerator();

	UFUNCTION()
	TArray<FVector> CalculateVertices(const uint32 Size, const FVector& VertScale) const;
	UFUNCTION()
	void AlterVerticesHeight(TArray<FVector>& outVertices, const USplineComponent* TrackSpline, const uint32 Size, const TArray<FColor>& TexColors, const
	                         FVector& VertScale);

	void SetupWalls(const uint32 TextureWidth, const uint32 TextureHeight, const FVector& VertScale);

	virtual void DoWork(const FWorkerData& Data, const FOnWorkFinished Callback) override;

	void TryAddFoliageLocation(const FVector& LocationToSpawn);

	void SpawnGrassFoliage(TArray<FVector>& Locations);
	void SpawnActors(TArray<FVector>& Locations, const TSubclassOf<AActor>& ActorClass);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	UFUNCTION()
	void CreateTerrain(const TArray<FColor>& HeightTextureColors, const USplineComponent* TrackSpline, const FVector& VertScale);

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere)
	bool UseBuiltInNormalsAndTangents = false;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* MeshMaterial;

	UPROPERTY(EditAnywhere)
	EColorChannel TextureChannel = EColorChannel::Red;

	TArray<FVector> Vertices;

	TArray<int32> TriangleIndices;

	TArray<FVector2D> UV;

	TArray<FVector> Normals;

	UPROPERTY(EditAnywhere)
	TWeakObjectPtr<ATrackGenerator> TrackGenerator;

	UPROPERTY(EditAnywhere)
	float MeshHeightScalar = 0.1f;

	UPROPERTY(VisibleAnywhere)
	TArray<UStaticMeshComponent*> TerrainWalls;

	UPROPERTY(EditAnywhere)
	UStaticMesh* TerrainWallMesh;

	UPROPERTY(EditAnywhere)
	UFoliageInstancedStaticMeshComponent* GrassFoliageComponent;
	UPROPERTY(EditAnywhere)
	float GrassFoliageProbability = 0.0f;
	TArray<FVector> GrassFoliageLocations;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> RockBlueprintClass;
	UPROPERTY(EditAnywhere)
	float RocksProbability = 0.0f;
	TArray<FVector> RocksLocations;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> TreeBlueprintClass;
	UPROPERTY(EditAnywhere)
	float TreesProbability = 0.0f;
	TArray<FVector> TreesLocations;

};
