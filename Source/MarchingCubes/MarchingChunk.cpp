// Fill out your copyright notice in the Description page of Project Settings.


#include "MarchingChunk.h"

#include "DrawDebugHelpers.h"

#include "VectorTypes.h"

AMarchingChunk::AMarchingChunk()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AMarchingChunk::BeginPlay()
{
	Super::BeginPlay();

	// Initialize size of array to number of points per chunk (x * y * z)
	Weights.SetNum(GridMetrics.PointsPerChunk * GridMetrics.PointsPerChunk * GridMetrics.PointsPerChunk);
	
	DrawDebugBoxes();
}

void AMarchingChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMarchingChunk::DrawDebugBoxes()
{
	if (Weights.Num() == 0) {
		return;
	}
	for (int x = 0; x < GridMetrics.PointsPerChunk; x++) {
		for (int y = 0; y < GridMetrics.PointsPerChunk; y++) {
			for (int z = 0; z < GridMetrics.PointsPerChunk; z++) {
				
				int index = x + GridMetrics.PointsPerChunk * (y + GridMetrics.PointsPerChunk * z);
				
				Weights[index] = GenerateNoise(FVector(x, y, z));
				
				UWorld* World = GetWorld(); // Get a reference to the current world
				FColor Color = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor::White, Weights[index]).ToFColor(true);

				if (World)
				{
					// Call DrawDebugBox to draw the box
					DrawDebugPoint(
						World,
						FVector(x,y,z) * GridMetrics.Distance,
						8,
						Color,
						true
					);
				}
			}
		}
	}
}

void AMarchingChunk::GenerateRandomVals()
{
	UE_LOG(LogTemp, Warning, TEXT("Number of vertices: %i \n"), Weights.Num());
	for (int i = 0; i < Weights.Num(); i++)
	{
		Weights[i] = FMath::FRandRange(0.0f, 1.0f);
		
		UE_LOG(LogTemp, Warning, TEXT("Vert: %f \n"), Weights[i]);
	}
}

float AMarchingChunk::GenerateNoise(FVector pos)
{
	Noise = new FastNoiseLite();
	Noise->NoiseType_OpenSimplex2;
	Noise->FractalType_Ridged;
	Noise->SetFrequency(Frequency);
	Noise->SetFractalOctaves(Octaves);
	
	float Ground = -pos.Z + (GroundPercent * GridMetrics.PointsPerChunk);
	
	float n = Ground + Noise->GetNoise(pos.X, pos.Y, pos.Z) * Amplitude;
	UE_LOG(LogTemp, Warning, TEXT("NoiseVal: %f \n"), n);
	
	return n;
}
