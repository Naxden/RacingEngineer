// Fill out your copyright notice in the Description page of Project Settings.


#include "TrackCheckpoint.h"

#include "Components/StaticMeshComponent.h"

// Sets default values
ATrackCheckpoint::ATrackCheckpoint()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Create and set up the static mesh component
	CheckpointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointMesh"));
	if (CheckpointMesh != nullptr)
	{
		SetRootComponent(CheckpointMesh);
		CheckpointMesh->SetGenerateOverlapEvents(true);
		CheckpointMesh->OnComponentBeginOverlap.AddDynamic(this, &ATrackCheckpoint::OnOverlapBegin);

		if (BasicMaterial != nullptr)
		{
			CheckpointMesh->SetMaterial(0, BasicMaterial);
		}
	}
}

// Called when the game starts or when spawned
void ATrackCheckpoint::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATrackCheckpoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATrackCheckpoint::SetMaterialToTarget() const
{
	if (CheckpointMesh != nullptr && TargetMaterial != nullptr)
	{
		CheckpointMesh->SetMaterial(0, TargetMaterial);
	}
}

void ATrackCheckpoint::SetMaterialToBasic() const
{
	if (CheckpointMesh != nullptr && BasicMaterial != nullptr)
	{
		CheckpointMesh->SetMaterial(0, BasicMaterial);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ATrackCheckpoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != nullptr && OtherActor != this)
	{
		if (OnPawnOverlappedWTrackCheckpoint.IsBound())
		{
			OnPawnOverlappedWTrackCheckpoint.Broadcast(CheckpointIndex);
		}
	}
}

