// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainGenerator.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

using UE::Math::TVector;
// Sets default values
ATerrainGenerator::ATerrainGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	ProceduralMesh->SetupAttachment(GetRootComponent());
}

TArray<FColor> ATerrainGenerator::GetColorsFromHeightMapTexture() const
{
	TArray<FColor> ColorData;

	if (HeightMapTexture != nullptr)
	{
		FRenderCommandFence RenderFence;

		const uint32 Width = HeightMapTexture->GetSizeX();
		const uint32 Height = HeightMapTexture->GetSizeY();

		ColorData.SetNumUninitialized(Width * Height);

		ENQUEUE_RENDER_COMMAND(ReadColorsFromTexture)(
			[&ColorData, Width, Height, this](FRHICommandListImmediate& RHICmdList)
			{
				constexpr uint32 BufferSize = 256;
				const uint64 TextureResolution = Width * Height;

				FTextureResource* Resource = HeightMapTexture->GetResource();
				uint32 Stride = 0;
				void* MipData = RHILockTexture2D(
					Resource->GetTexture2DRHI(),
					0,
					RLM_ReadOnly,
					Stride,
					false
				);

				EPixelFormat TexturePixelFormat = HeightMapTexture->GetPlatformData()->PixelFormat;
				if (TexturePixelFormat == PF_B8G8R8A8)
				{
					GetColors(ColorData, MipData, Width, Height);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("ATerrainGenerator::GetColorsFromHeightMapTexture() unhandeled PixelFormat"))
				}

				RHIUnlockTexture2D(Resource->GetTexture2DRHI(), 0, false);
			});

		RenderFence.BeginFence();
		RenderFence.Wait();

	}

	return ColorData;
}

void ATerrainGenerator::GetColors(TArray<FColor>& ColorData, void* MipData, uint32 Width, uint32 Height)
{
	constexpr uint32 BufferSize = 256;
	const uint64 TextureResolution = Width * Height;

	if (Width < BufferSize / sizeof(FColor))
	{
		for (uint32 y = 0; y < Height; y++)
		{
			FColor* Dst = ColorData.GetData() + y * Width;
			const FColor* Src = static_cast<FColor*>(MipData) + y * BufferSize / sizeof(FColor);

			FMemory::Memcpy(Dst, Src, Width * sizeof(FColor));
		}
	}
	else
	{
		FMemory::Memcpy(ColorData.GetData(), MipData, TextureResolution * sizeof(FColor));
	}
}

void ATerrainGenerator::AlterVerticesHeight(TArray<FVector>& outVertices, const uint32 Size, const TArray<FColor>& TexColors) const
{
	for (uint32 y = 0; y < Size; y++)
	{
		for (uint32 x = 0; x < Size; x++)
		{
			constexpr uint8 Offset = 127;
			FVector& currentVert = outVertices[y * Size + x];
			const FColor& currentColor = TexColors[y * Size + x];

			uint8 colorValue = 0;
			switch (TextureChannel)
			{
			case EColorChannel::Blue:
				colorValue = currentColor.B;
				break;
			case EColorChannel::Green:
				colorValue = currentColor.G;
				break;
			case EColorChannel::Red:
				colorValue = currentColor.R;
				break;
			case EColorChannel::Alpha:
				colorValue = currentColor.A;
				break;
			default:
				break;
			}

			currentVert.Z += (colorValue - Offset) / 255.0 * VertSpacingScale.Z;
		}
	}
}

// Called when the game starts or when spawned
void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (HeightMapTexture != nullptr)
	{
		const auto HeightMapResource = HeightMapTexture->GetResource();

		check(HeightMapResource->GetSizeX() == HeightMapResource->GetSizeY())

		TextureWidth = HeightMapResource->GetSizeX();
	}

	Vertices = CalculateVertices(TextureWidth);
	TextureColors = GetColorsFromHeightMapTexture();
	AlterVerticesHeight(Vertices, TextureWidth, TextureColors);
	TriangleIndices = CalculateTriangles(TextureWidth);

	TArray<FProcMeshTangent> Tangents1;

	const uint32 TimerStart = FPlatformTime::Cycles();

	if (UseBuiltInNormalsAndTangents)
	{
		TArray<FProcMeshTangent> Tangents;
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, TriangleIndices, UV, Normals, Tangents);
	}
	else
	{	
		Normals = CalculateNormals(Vertices, TriangleIndices, TextureWidth);
	}

	const uint32 TimerStop = FPlatformTime::Cycles();
	
	UE_LOG(LogTemp, Warning, TEXT("Elapsed time %fms"), FPlatformTime::ToMilliseconds(TimerStop - TimerStart));

	ProceduralMesh->CreateMeshSection(
		0, 
		Vertices, 
		TriangleIndices, 
		Normals, 
		UV, 
		TArray<FColor>(), 
		Tangents1,
		true);

	ProceduralMesh->SetMaterial(0, MeshMaterial);
}

TArray<FVector> ATerrainGenerator::CalculateVertices(const uint32 Size) const
{
	TArray<FVector> Verts;
	const uint64 VertCount = Size * Size;
	Verts.Reserve(VertCount);

	TVector LocalPosition = GetTransform().GetLocation();

	LocalPosition -= UE::Math::TVector<double>(Size / 2 * VertSpacingScale.X, Size / 2 * VertSpacingScale.Y, 0);

	for (uint32 y = 0; y < Size; y++)
	{
		for (uint32 x = 0; x < Size; x++)
		{
			Verts.Emplace
			(
			x * VertSpacingScale.X + LocalPosition.X, 
			y * VertSpacingScale.Y + LocalPosition.Y,
				LocalPosition.Z
			);
		}
	}

	return Verts;
}


TArray<int32> ATerrainGenerator::CalculateTriangles(const uint32 Size)
{
	const uint32 TriangleNodesCount = (Size - 1) * (Size - 1) * 2 * 3;
	TArray<int32> TriangleNodes;
	TriangleNodes.Reserve(TriangleNodesCount);

	for (uint32 y = 0; y < Size - 1; y++)
	{
		for (uint32 x = 0; x < Size - 1; x++)
		{
			TriangleNodes.Emplace(x + y * Size);
			TriangleNodes.Emplace(x + (y + 1) * Size);
			TriangleNodes.Emplace(x + 1 + y * Size);

			TriangleNodes.Emplace(x + 1 + y * Size);
			TriangleNodes.Emplace(x + (y + 1) * Size);
			TriangleNodes.Emplace(x + 1 + (y + 1) * Size);
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

TArray<FVector> ATerrainGenerator::CalculateNormals(const TArray<FVector>& Verts, const TArray<int32> Triangles, const uint32 Size)
{
	const uint32 NormalCount = Size * Size;
	const uint32 TriangleIndicesCount = (Size - 1) * (Size - 1) * 2 * 3;

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

// Called every frame
void ATerrainGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

