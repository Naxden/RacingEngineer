// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackCheckpoint.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPawnOverlappedWTrackCheckpointmMulticastDelegate, uint16);

UCLASS()
class RACINGENGINEER_API ATrackCheckpoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrackCheckpoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void SetMaterialToTarget() const;

	UFUNCTION()
	void SetMaterialToBasic() const;

	void SetCheckpointIndex(uint16 Index) { CheckpointIndex = Index; }

	FOnPawnOverlappedWTrackCheckpointmMulticastDelegate OnPawnOverlappedWTrackCheckpoint;
private:
	// Overlap event
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Static mesh component
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* CheckpointMesh;

	// Basic material
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* BasicMaterial;

	// Current material
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* TargetMaterial;

	uint16 CheckpointIndex;
};
