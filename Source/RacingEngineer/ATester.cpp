// Fill out your copyright notice in the Description page of Project Settings.


#include "ATester.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

using UE::Math::TVector;
// Sets default values
ATester::ATester()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	ProceduralMesh->SetupAttachment(GetRootComponent());
}

void ATester::AlterVertices(const uint32 Size)
{
	const uint8* Pixels = nullptr;
	FTexture2DMipMap* Mip = nullptr;
	bool usedTexture = false;

	if (HeightMapTexture != nullptr)
	{
		// Get the resource of the HeightMapTexture
		Mip = &HeightMapTexture->GetPlatformData()->Mips[0];
		void* Data = Mip->BulkData.Lock(LOCK_READ_ONLY);

		auto mipSize = Mip->BulkData.GetBulkDataSize();
		UE_LOG(LogTemp, Warning, TEXT("Mip's element size in bytes: %d"), Mip->BulkData.GetElementSize());
		UE_LOG(LogTemp, Warning, TEXT("Mip size in bytes: %lld"), mipSize);

		Pixels = static_cast<uint8*>(Data);
	}

	for (uint32 y = 0; y < Size; y++)
	{
		for (uint32 x = 0; x < Size; x++)
		{
			double Offset;

			if (Mip != nullptr)
			{
				// Calculate the index of the pixel in the texture
				const int32 PixelIndex = y * Size + x;
				const int32 PixelOffset = PixelIndex * 4; // Each pixel has 4 bytes (RGBA)

				// Get the RGBA values of the pixel
				const uint8 Red = Pixels[PixelOffset];
				//const uint8 Green = Pixels[PixelOffset + 1];
				//const uint8 Blue = Pixels[PixelOffset + 2];
				//const uint8 Alpha = Pixels[PixelOffset + 3];

				// Calculate the Offset based on the RGBA values
				Offset = (Red) / 255.0 * VertAlterationScale;

				usedTexture = true;
			}
			else
			{
				Offset = sin((x * y) / pow(Size, 2) * PI) * VertAlterationScale;

				usedTexture = false;
			}

			Vertices[y * Size + x].Z += Offset - VertAlterationScale;
		}
	}

	if (Mip != nullptr)
	{
		Mip->BulkData.Unlock();
	}

	UE_LOG(LogTemp, Warning, TEXT("%hs Used texture to generate terrain"), usedTexture ? "" : "NOT");
}

// Called when the game starts or when spawned
void ATester::BeginPlay()
{
	Super::BeginPlay();

	if (HeightMapTexture != nullptr)
	{
		const auto HeightMapResource = HeightMapTexture->GetResource();

		check(HeightMapResource->GetSizeX() == HeightMapResource->GetSizeY())

		Resolution = HeightMapResource->GetSizeX();
	}

	CalculateVertices(Resolution);
	AlterVertices(Resolution);
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

void ATester::CalculateVertices(const uint32 Size)
{
	const uint32 VertCount = Size * Size;
	Vertices.Empty(VertCount);
	const TVector LocalScale = GetTransform().GetScale3D();
	TVector LocalPosition = GetTransform().GetLocation();

	LocalPosition -= UE::Math::TVector<double>((Size / 2) * VertSpacing, (Size / 2) * VertSpacing, 0);

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


void ATester::CalculateTriangles(const uint32 Size)
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

FVector ATester::GetNormal(const FVector& V0, const FVector& V1, const FVector& V2)
{
	const FVector Edge1 = V1 - V0;
	const FVector Edge2 = V2 - V0;

	FVector crossVector = FVector::CrossProduct(Edge2, Edge1);

	NormalizeVector(crossVector);

	return crossVector;
}

void ATester::CalculateNormals(const uint32 Size)
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
void ATester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

