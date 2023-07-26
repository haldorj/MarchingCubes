// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarchingChunk.h"
#include "Utility/GridMetrics.h"
#include "GameFramework/Actor.h"
#include "ChunkSpawner.generated.h"

UCLASS()
class MARCHINGCUBES_API AChunkSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AChunkSpawner();
	// virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

	void SpawnChunk();

private:
	UPROPERTY(VisibleAnywhere, Category = "Spawning")
	AMarchingChunk* SpawnedChunk;
	
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AMarchingChunk> ChunkBP;

	FGridMetrics* GridMetrics;
};
