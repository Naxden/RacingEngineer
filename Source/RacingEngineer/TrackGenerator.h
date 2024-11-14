// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorkerActor.h"
#include "GameFramework/Actor.h"
#include "TrackGenerator.generated.h"

UENUM()
enum class Direction : uint8
{
	Left = 0,
	DownLeft = 1,
	Down = 2,
	DownRight = 3,
	Right = 4,
	UpRight = 5,
	Up = 6,
	UpLeft = 7,
};

inline Direction operator+(const Direction& Dir, const int& Val)
{
	return static_cast<Direction>((static_cast<int>(Dir) + Val) % 8);
}

USTRUCT()
struct FTrackNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FVector2D Position = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere)
	Direction PrevPointDirection = Direction::Left;
};

class USplineComponent;

UCLASS()
class RACINGENGINEER_API ATrackGenerator : public AWorkerActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrackGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnConstruction(const FTransform& Transform) override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void DoWork(const TArray<FColor>& HeightTextureColors, const FVector& VertScale, FOnWorkFinished Callback) override;

	UFUNCTION()
	static FVector2D AddDirectionToPosition(const FVector2D& Vector2D, const Direction& Direction);

	UFUNCTION()
	static TArray<FTrackNode> CreateTrack(const TArray<FColor>& HeightTextureColors, const uint32 TextureWidth);

public:
	UPROPERTY(EditAnywhere)
	USplineComponent* SplineComponent;

	UPROPERTY(EditAnywhere)
	UStaticMesh* TrackMesh;

	UPROPERTY(EditAnywhere)
	FVector MeshScale = FVector::OneVector;

private:
	UFUNCTION()
	static FTrackNode FindFirstTrackNode(const TArray<FColor>& HeightTextureColors, const uint32 TextureWidth);
	UFUNCTION()
	static FTrackNode FindNextTrackNode(const TArray<FColor>& HeightTextureColors, const uint32 TextureWidth, const FTrackNode& CurrentNode);
	UFUNCTION()
	static bool ShouldFindAnotherTrackNode(const TArray<FTrackNode>& TrackNodes);

	void CreateTrackSpline(const TArray<FColor>& HeightTextureColors, const uint32 Height, const uint32 Width, const FVector& VertScale);

	void SpawnMeshBasedOnMeshLength(const FVector& MeshSize);
	void CreateMeshOnSpline();

	FVector GetMeshLength() const;
	void SpawnMeshPerSplinePoint(uint32 NumberOfSplinePoints, FVector MeshOffset);

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(VisibleInstanceOnly)
	TArray<FTrackNode> TrackNodes;
};
