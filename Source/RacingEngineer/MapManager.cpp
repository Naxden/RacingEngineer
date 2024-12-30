// Fill out your copyright notice in the Description page of Project Settings.


#include "MapManager.h"

#include "TrackGenerator.h"
#include "Components/SplineComponent.h"

// Sets default values
AMapManager::AMapManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>("TrackSpline");
	if (SplineComponent != nullptr)
	{
		SplineComponent->SetupAttachment(GetRootComponent());
	}
}

// Called when the game starts or when spawned
void AMapManager::BeginPlay()
{
	Super::BeginPlay();

	ScheduleWorkers();
}

void AMapManager::ScheduleWorkers()
{
	if (HeightMapTexture != nullptr && SplineComponent != nullptr)
	{
		TextureColors = GetColorsFromTexture(HeightMapTexture);

		SplineComponent->ClearSplinePoints();
		TrackNodes = CreateTrack(TextureColors, TextureHeight, TextureWidth, NodeToSkip);
		CreateTrackSpline(SplineComponent, TrackNodes, TextureColors, TextureHeight, TextureWidth, VertSpacingScale);
		SplineComponent->SetClosedLoop(true);

		for (const TObjectPtr<AWorkerActor>& Worker : Workers)
		{
			const uint32 TimerStart = FPlatformTime::Cycles();

			Worker->DoWork(TextureColors, SplineComponent, VertSpacingScale, FOnWorkFinished::CreateUObject(this, &AMapManager::WorkerFinished));

			const uint32 TimerStop = FPlatformTime::Cycles();

			UE_LOG(LogTemp, Warning, TEXT("Worker %s elapsed time %fms"), *Worker->GetActorNameOrLabel(),
				FPlatformTime::ToMilliseconds(TimerStop - TimerStart));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMapManager::BeginPlay() HeightMapTexture or SplineComponent is nullptr"))
	}
}

// Called every frame
void AMapManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

TArray<FColor> AMapManager::GetColorsFromTexture(UTexture2D* Texture)
{
	TArray<FColor> ColorData;

	if (Texture != nullptr)
	{
		FRenderCommandFence RenderFence;

		TextureHeight = Texture->GetSizeX();
		TextureWidth = Texture->GetSizeY();

		ColorData.SetNumUninitialized(TextureWidth * TextureHeight);

		ENQUEUE_RENDER_COMMAND(ReadColorsFromTexture)(
			[&ColorData, Width = TextureWidth, Height = TextureHeight, Texture](FRHICommandListImmediate& RHICmdList)
			{
				FTextureResource* Resource = Texture->GetResource();
				uint32 Stride = 0;
				void* MipData = RHILockTexture2D(
					Resource->GetTexture2DRHI(),
					0,
					RLM_ReadOnly,
					Stride,
					false
				);

				EPixelFormat TexturePixelFormat = Texture->GetPlatformData()->PixelFormat;
				if (TexturePixelFormat == PF_B8G8R8A8)
				{
					 GetColors(ColorData, MipData, Width, Height);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("AMapManager::GetColorsFromTexture() unhandeled PixelFormat"))
				}

				RHIUnlockTexture2D(Resource->GetTexture2DRHI(), 0, false);
			});

		RenderFence.BeginFence();
		RenderFence.Wait();

	}

	return ColorData;
}

void AMapManager::GetColors(TArray<FColor>& ColorData, void* SrcData, uint32 TextureWidth, uint32 TextureHeight)
{
	constexpr uint32 BufferSize = 256;
	const uint64 TextureResolution = TextureWidth * TextureHeight;

	if (TextureWidth < BufferSize / sizeof(FColor))
	{
		for (uint32 y = 0; y < TextureHeight; y++)
		{
			FColor* Dst = ColorData.GetData() + y * TextureWidth;
			const FColor* Src = static_cast<FColor*>(SrcData) + y * BufferSize / sizeof(FColor);

			FMemory::Memcpy(Dst, Src, TextureWidth * sizeof(FColor));
		}
	}
	else
	{
		FMemory::Memcpy(ColorData.GetData(), SrcData, TextureResolution * sizeof(FColor));
	}
}

void AMapManager::WorkerFinished()
{
	++FinishedWorkersCounter;
	UE_LOG(LogTemp, Log, TEXT("Worker has finished its work"));
}

#pragma region TextureToSpline

FVector2D AMapManager::AddDirectionToPosition(const FVector2D& Vector2D, const EDirection& Direction)
{
	FVector2D MoveVec;

	switch (Direction)
	{
	case EDirection::UpLeft:
		MoveVec = FVector2D(-1.0, -1.0);
		break;
	case EDirection::Up:
		MoveVec = FVector2D(0.0, -1.0);
		break;
	case EDirection::UpRight:
		MoveVec = FVector2D(1.0, -1.0);
		break;
	case EDirection::Right:
		MoveVec = FVector2D(1.0, 0.0);
		break;
	case EDirection::DownRight:
		MoveVec = FVector2D(1.0, 1.0);
		break;
	case EDirection::Down:
		MoveVec = FVector2D(0.0, 1.0);
		break;
	case EDirection::DownLeft:
		MoveVec = FVector2D(-1.0, 1.0);
		break;
	case EDirection::Left:
		MoveVec = FVector2D(-1.0, 0.0);
		break;
	default:
		MoveVec = FVector2D(0.0, 0.0);
		break;
	}

	return Vector2D + MoveVec;
}

TArray<FVector2D> AMapManager::CreateTrack(const TArray<FColor>& HeightTextureColors, const uint32 TextureHeight, const uint32 TextureWidth, const uint8 SkipNodesCount)
{
	TArray<FVector2D> TrackNodes;

	FTrackNode TrackNode = FindFirstTrackNode(HeightTextureColors, TextureHeight, TextureWidth);
	TrackNodes.Add(TrackNode.Position);
	constexpr uint8 MinNumberOfNodes = 3;

	for (uint8 i = 0; i < MinNumberOfNodes; i++)
	{
		TrackNode = FindNextTrackNode(HeightTextureColors, TextureWidth, TrackNode);
		TrackNodes.Add(TrackNode.Position);
	}

	while (ShouldFindAnotherTrackNode(TrackNodes))
	{
		TrackNode = FindNextTrackNode(HeightTextureColors, TextureWidth, TrackNode);
		TrackNodes.Add(TrackNode.Position);
	}

	if (SkipNodesCount > 0)
	{
		TArray<FVector2D> TrackNodesReduced;
		TrackNodesReduced.Reserve(TrackNodes.Num() / SkipNodesCount);
		// Reduce the number of nodes
		for (SIZE_T i = 0; i < TrackNodes.Num(); i += SkipNodesCount)
		{
			TrackNodesReduced.Add(TrackNodes[i]);
		}

		return TrackNodesReduced;
	}
	else
	{
		return TrackNodes;
	}
}

FTrackNode AMapManager::FindFirstTrackNode(const TArray<FColor>& HeightTextureColors, const uint32 TextureHeight, const uint32 TextureWidth)
{
	for (uint32 y = TextureHeight * 3 / 4; y < TextureHeight; y++)
	{
		for (uint32 x = 0; x < TextureWidth / 2; x++)
		{
			FColor pixelColor = HeightTextureColors[y * TextureWidth + x];

			if (pixelColor.A < 255)
			{
				return FTrackNode(FVector2D(x, y), EDirection::Left);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("AMapManager::FindFirstTrackNode Couldn't find the first node"));
	return FTrackNode(FVector2D(0, TextureWidth - 1), EDirection::Left);
}

FTrackNode AMapManager::FindNextTrackNode(const TArray<FColor>& HeightTextureColors, const uint32 TextureWidth, const FTrackNode& CurrentNode)
{
	EDirection CurrentDirection = CurrentNode.PrevPointDirection + 1;
	bool bFound = false;

	for (int i = 0; i < 7; i++)
	{
		const FVector2D NeighbourPos = AddDirectionToPosition(CurrentNode.Position, CurrentDirection);

		if (NeighbourPos.X > 0 && NeighbourPos.X < TextureWidth &&
			NeighbourPos.Y > 0 && NeighbourPos.Y < TextureWidth)
		{
			const FColor PixelColor = HeightTextureColors[NeighbourPos.Y * TextureWidth + NeighbourPos.X];

			if (PixelColor.A < 255)
			{
				bFound = true;
				break;
			}
		}

		CurrentDirection = CurrentDirection + 1;
	}

	if (!bFound)
	{
		UE_LOG(LogTemp, Error, TEXT("AMapManager::FindNextTrackNode Couldn't find the next node"));
		check(false);
	}

	const EDirection DirectionToOrigin = CurrentDirection + 4;
	return FTrackNode(AddDirectionToPosition(CurrentNode.Position, CurrentDirection), DirectionToOrigin);
}

bool AMapManager::ShouldFindAnotherTrackNode(const TArray<FVector2D>& TrackNodes)
{
	check(TrackNodes.Num() >= 3);

	const FVector2D FirstNode = TrackNodes[0];
	const FVector2D LastNode = TrackNodes.Last();

	return FVector2D::DistSquared(FirstNode, LastNode) > 2;
}

void AMapManager::CreateTrackSpline(USplineComponent* Spline, const TArray<FVector2D>& Nodes, const TArray<FColor>& HeightTextureColors, const uint32 Height, const uint32 Width, const FVector& VertScale)
{
	if (Spline != nullptr)
	{
		for (const FVector2D& TrackNode : Nodes)
		{
			const uint8 HeightValue = HeightTextureColors[TrackNode.Y * Width + TrackNode.X].R;
			FVector SplinePos =
			{
				(TrackNode.X - Width / 2) * VertScale.X,
				(TrackNode.Y - Height / 2) * VertScale.Y,
				(HeightValue - 255 / 2) / 255.0 * VertScale.Z
			};

			Spline->AddSplinePoint(SplinePos, ESplineCoordinateSpace::Local);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AMapManager::CreateTrackSpline Spline is nullptr"));
	}
}

#pragma endregion
