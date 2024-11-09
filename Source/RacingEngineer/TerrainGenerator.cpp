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

void ATerrainGenerator::GetVerticesHeightFromTexture(const uint32 Size)
{
	if (HeightMapTexture != nullptr)
	{
		FRenderCommandFence RenderFence;
		
		ENQUEUE_RENDER_COMMAND(ReadColorsFromTexture)(
			[Size, this](FRHICommandListImmediate& RHICmdList)
			{
				FTextureResource* Resource = HeightMapTexture->GetResource();
				uint32 Stride = 0;
				void* MidData = RHILockTexture2D(
					Resource->GetTexture2DRHI(),
					0,
					RLM_ReadOnly,
					Stride,
					false
				);

				uint8* ColorData = static_cast<uint8*>(MidData);

				for (uint32 y = 0; y < Size; y++)
				{
					for (uint32 x = 0; x < Size; x++)
					{
						double Offset = 125.0;

						if (MidData != nullptr)
						{
							// Calculate the index of the pixel in the texture
							const int32 PixelIndex = y * Size + x;
							const int32 PixelOffset = PixelIndex * 4; // Each pixel has 4 bytes (RGBA)

							uint8 value = 0;
							switch (TextureChannel)
							{
							case EColorChannel::Blue:
								value = ColorData[PixelOffset];
								break;
							case EColorChannel::Green:
								value = ColorData[PixelOffset + 1];
								break;
							case EColorChannel::Red:
								value = ColorData[PixelOffset + 2];
								break;
							case EColorChannel::Alpha:
								value = ColorData[PixelOffset + 3];
								break;
							default:
								break;
							}

							// Calculate the Offset based on the RGBA values
							Offset = (value) / 255.0 * VertAlterationScale;
						}

						Vertices[y * Size + x].Z += Offset - VertAlterationScale;
					}
				}

				RHIUnlockTexture2D(Resource->GetTexture2DRHI(), 0, false);
			});

		RenderFence.BeginFence();
		RenderFence.Wait();
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

		Resolution = HeightMapResource->GetSizeX();
	}

	CalculateVertices(Resolution);
	GetVerticesHeightFromTexture(Resolution);
	CalculateTriangles(Resolution);

	TArray<FProcMeshTangent> Tangents;
	TArray<FProcMeshTangent> Tangents1;

	const uint32 TimerStart = FPlatformTime::Cycles();

	if (UseBuiltInNormalsAndTangents)
	{
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, TriangleIndices, UV, Normals, Tangents);
	}
	else
	{	
		CalculateNormals(Resolution);
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

void ATerrainGenerator::CalculateVertices(const uint32 Size)
{
	const uint32 VertCount = Size * Size;
	Vertices.Empty(VertCount);
	const TVector LocalScale = GetTransform().GetScale3D();
	TVector LocalPosition = GetTransform().GetLocation();

	LocalPosition -= UE::Math::TVector<double>(Size / 2 * VertSpacing, Size / 2 * VertSpacing, 0);

	for (uint32 y = 0; y < Size; y++)
	{
		for (uint32 x = 0; x < Size; x++)
		{
			Vertices.Add
			(
			FVector
				(
				x * VertSpacing * LocalScale.X + LocalPosition.X, 
				y * VertSpacing * LocalScale.Y + LocalPosition.Y, 
				0
				)
			);
		}
	}
}


void ATerrainGenerator::CalculateTriangles(const uint32 Size)
{
	const uint32 TriangleNodesCount = (Size - 1) * (Size - 1) * 2 * 3;
	TriangleIndices.Empty(TriangleNodesCount);

	for (uint32 y = 0; y < Size - 1; y++)
	{
		for (uint32 x = 0; x < Size - 1; x++)
		{
			TriangleIndices.Add(x + y * Size);
			TriangleIndices.Add(x + (y + 1) * Size);
			TriangleIndices.Add(x + 1 + y * Size);

			TriangleIndices.Add(x + 1 + y * Size);
			TriangleIndices.Add(x + (y + 1) * Size);
			TriangleIndices.Add(x + 1 + (y + 1) * Size);
		}
	}
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

void ATerrainGenerator::CalculateNormals(const uint32 Size)
{
	const uint32 NormalCount = Size * Size;
	const uint32 TriangleIndicesCount = (Size - 1) * (Size - 1) * 2 * 3;
	Normals.Empty(NormalCount);
	Normals.AddZeroed(NormalCount);

	check(Vertices.Num() == NormalCount)
	check(TriangleIndices.Num() == TriangleIndicesCount)

	for (uint32 i = 0; i < TriangleIndicesCount; i += 3)
	{
		const uint32 I0 = TriangleIndices[i];
		const uint32 I1 = TriangleIndices[i + 1];
		const uint32 I2 = TriangleIndices[i + 2];
		
		FVector Normal = GetNormal
		(
			Vertices[I0],
			Vertices[I1],
			Vertices[I2]
		);

		Normals[I0] += Normal;
		Normals[I1] += Normal;
		Normals[I2] += Normal;
	}

	for (size_t i = 0; i < NormalCount; i++)
	{
		NormalizeVector(Normals[i]);
	}
}

// Called every frame
void ATerrainGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

