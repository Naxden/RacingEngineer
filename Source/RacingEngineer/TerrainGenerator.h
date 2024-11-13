// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorkerActor.h"
#include "GameFramework/Actor.h"
#include "TerrainGenerator.generated.h"

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

	ATerrainGenerator();

	UFUNCTION()
	TArray<FVector> CalculateVertices(uint32 Size) const;
	UFUNCTION()
	void AlterVerticesHeight(TArray<FVector>& outVertices, const uint32 Size, const TArray<FColor>& TexColors) const;

	virtual void DoWork(const TArray<FColor>& HeightTextureColors, FOnWorkFinished Callback) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	UFUNCTION()
	void CreateTerrain(const TArray<FColor>& HeightTextureColors);

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere)
	bool UseBuiltInNormalsAndTangents = false;

	UPROPERTY(EditAnywhere)
	FVector VertSpacingScale = FVector::OneVector;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* MeshMaterial;

	UPROPERTY(EditAnywhere)
	EColorChannel TextureChannel;

	TArray<FVector> Vertices;

	TArray<int32> TriangleIndices;

	TArray<FVector2D> UV;

	TArray<FVector> Normals;
};
