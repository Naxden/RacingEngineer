// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATester.generated.h"

struct FProcMeshTangent;
class UProceduralMeshComponent;

UCLASS()
class RACINGENGINEER_API ATester : public AActor
{
	GENERATED_BODY()
	
public:
	UFUNCTION()
	void CalculateTriangles(uint32 Size);
	static FVector GetNormal(const FVector& V0, const FVector& V1, const FVector& V2);
	void CalculateNormals(uint32 Size);
	UFUNCTION()
	void CalculateVertices(uint32 Size);
	// Sets default values for this actor's properties
	ATester();

protected:
	void AlterVertices(uint32 Size);
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere)
	bool UseBuiltInNormalsAndTangents = false;

	UPROPERTY(EditAnywhere)
	int Resolution = 3;

	UPROPERTY(EditAnywhere)
	int VertSpacing = 10;

	UPROPERTY(EditAnywhere)
	double VertAlterationScale = 10;

	UPROPERTY(EditAnywhere)
	UTexture2D* HeightMapTexture;

	UPROPERTY()
	TArray<FVector> Vertices;

	UPROPERTY()
	TArray<int32> TriangleIndices;

	UPROPERTY()
	TArray<FVector2D> UV;

	UPROPERTY()
	TArray<FVector> Normals;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* MeshMaterial;
};
