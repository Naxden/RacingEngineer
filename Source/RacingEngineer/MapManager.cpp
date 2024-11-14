// Fill out your copyright notice in the Description page of Project Settings.


#include "MapManager.h"

#include "TerrainGenerator.h"
#include "TrackGenerator.h"

// Sets default values
AMapManager::AMapManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AMapManager::BeginPlay()
{
	Super::BeginPlay();

	if (HeightMapTexture != nullptr)
	{
		TextureColors = GetColorsFromTexture(HeightMapTexture);
	}

	if (TerrainGenerator != nullptr)
	{
		TerrainGenerator->DoWork(TextureColors, VertSpacingScale, FOnWorkFinished::CreateUObject(this, &AMapManager::WorkerFinished));
	}

	if (TrackGenerator != nullptr)
	{
		TrackGenerator->DoWork(TextureColors, VertSpacingScale, FOnWorkFinished::CreateUObject(this, &AMapManager::WorkerFinished));
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

		const uint32 Width = Texture->GetSizeX();
		const uint32 Height = Texture->GetSizeY();

		ColorData.SetNumUninitialized(Width * Height);

		ENQUEUE_RENDER_COMMAND(ReadColorsFromTexture)(
			[&ColorData, Width, Height, Texture](FRHICommandListImmediate& RHICmdList)
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

