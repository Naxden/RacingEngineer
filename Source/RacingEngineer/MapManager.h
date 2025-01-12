// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapManager.generated.h"

class USplineComponent;
class AWorkerActor;
class ASplineTrackGenerator;
class ATerrainGenerator;
class ATrackGenerator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInitializationUpdate, float, CompletePercentage);

UENUM()
enum class EDirection : uint8
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

inline EDirection operator+(const EDirection& Dir, const int& Val)
{
	return static_cast<EDirection>((static_cast<int>(Dir) + Val) % 8);
}

USTRUCT()
struct FTrackNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FVector2D Position = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere)
	EDirection PrevPointDirection = EDirection::Left;
};

UCLASS()
class RACINGENGINEER_API AMapManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void InitializeMap(bool StartedFromMainMenu = false);

	UPROPERTY(BlueprintAssignable)
	FOnInitializationUpdate OnInitializationUpdate;

	UFUNCTION(BlueprintCallable)
	void MovePlayerToStart();

	UFUNCTION(BlueprintCallable)
	void SetHeightMapTexture(UTexture2D* Texture);

	TArray<FColor> GetColorsFromTexture(UTexture2D* Texture);

	static void GetColors(TArray<FColor>& ColorData, void* SrcData, uint32 TextureWidth, uint32 TextureHeight);

	static FTrackNode FindFirstTrackNode(const TArray<FColor>& HeightTextureColors, const uint32 TextureHeight, const uint32 TextureWidth);
	static FTrackNode FindNextTrackNode(const TArray<FColor>& HeightTextureColors, const uint32 TextureWidth, const FTrackNode& CurrentNode);
	static bool ShouldFindAnotherTrackNode(const TArray<FVector2D>& TrackNodes);
	static FVector2D AddDirectionToPosition(const FVector2D& Vector2D, const EDirection& Direction);

	static TArray<FVector2D> CreateTrack(const TArray<FColor>& HeightTextureColors, const uint32 TextureHeight,
		const uint32 TextureWidth, const uint8 SkipNodesCount);

	static void CreateTrackSpline(USplineComponent* Spline, const TArray<FVector2D>& Nodes, const TArray<FColor>& HeightTextureColors,
		const uint32 Height, const uint32 Width, const FVector& VertScale);

	static uint8 CalculateNodeToSkip(const uint32 TextureHeight, const uint32 TextureWidth);

private:
	void WorkerFinished();

private:
	UPROPERTY()
	USplineComponent* SplineComponent;

	UPROPERTY(EditAnywhere)
	UTexture2D* HeightMapTexture;

	UPROPERTY(EditAnywhere)
	uint8 NodeToSkip = 1;

	UPROPERTY(EditAnywhere)
	FVector VertSpacingScale = FVector::OneVector;

	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<AWorkerActor>> Workers;

	TArray<FColor> TextureColors;

	std::atomic_uint8_t FinishedWorkersCounter = 0;

	TArray<FVector2D> TrackNodes;

	uint32 TextureHeight = 0;
	uint32 TextureWidth = 0;
};
