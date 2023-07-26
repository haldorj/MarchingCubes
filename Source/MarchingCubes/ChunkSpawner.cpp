// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkSpawner.h"


AChunkSpawner::AChunkSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

}

void AChunkSpawner::BeginPlay()
{
	Super::BeginPlay();
	SpawnChunk();
	
}

void AChunkSpawner::SpawnChunk()
{
	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		
		float Dist = 100 * (32 - 1);

		int Iterations = 3;
		
		for (int x = -Iterations; x <= Iterations; x++)
		{
			for (int y = -Iterations; y <= Iterations; y++)
			{
				// Set the location and rotation where you want to spawn the actor
				FVector SpawnLocation = FVector(x * Dist, y * Dist, 0.f);
				FRotator SpawnRotation = FRotator::ZeroRotator;
        
				SpawnedChunk = World->SpawnActor<AMarchingChunk>(ChunkBP, SpawnLocation, SpawnRotation, SpawnParams);
				if (SpawnedChunk)
				{
					SpawnedChunk->InitialX = x;
					SpawnedChunk->InitialY = y;

					SpawnedChunk->PopulateTerrainMap();
					SpawnedChunk->Initialize();
					SpawnedChunk->UpdateMesh();
				}
			}
		}
	}
}

