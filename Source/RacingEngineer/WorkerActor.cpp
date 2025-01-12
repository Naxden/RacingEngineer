// Fill out your copyright notice in the Description page of Project Settings.


#include "WorkerActor.h"
#include "Async/Async.h"

// Sets default values
AWorkerActor::AWorkerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void AWorkerActor::DoWork(const FWorkerData& Data, const FOnWorkFinished Callback)
{
}

// Called when the game starts or when spawned
void AWorkerActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWorkerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

