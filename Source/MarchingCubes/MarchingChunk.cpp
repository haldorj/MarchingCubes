// Fill out your copyright notice in the Description page of Project Settings.


#include "MarchingChunk.h"

#include "DrawDebugHelpers.h"
#include "VectorTypes.h"

AMarchingChunk::AMarchingChunk()
{
	PrimaryActorTick.bCanEverTick = true;
	
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	ProceduralMesh->SetupAttachment(GetRootComponent());
	
	// Initialize size of array to number of points per chunk (x * y * z)
	Weights.SetNum(GridMetrics.PointsPerChunk * GridMetrics.PointsPerChunk * GridMetrics.PointsPerChunk);
}

void AMarchingChunk::BeginPlay()
{
	Super::BeginPlay();

	PopulateTerrainMap();
	BuildMesh();
	//DrawDebugBoxes();
}

void AMarchingChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMarchingChunk::March(FVector id)
{
	if (id.X >= (GridMetrics.PointsPerChunk - 1) || id.Y >= (GridMetrics.PointsPerChunk) - 1 || id.Z >= (GridMetrics.PointsPerChunk - 1))
	{
		UE_LOG(LogTemp, Warning, TEXT("Exited function"));
		return;
	}
	
	float CubeValues[8] = {
		Weights[IndexFromCoord(id.X, id.Y, id.Z + 1)],
		Weights[IndexFromCoord(id.X + 1, id.Y, id.Z + 1)],
		Weights[IndexFromCoord(id.X + 1, id.Y, id.Z)],
		Weights[IndexFromCoord(id.X, id.Y, id.Z)],
		Weights[IndexFromCoord(id.X, id.Y + 1, id.Z + 1)],
		Weights[IndexFromCoord(id.X + 1, id.Y + 1, id.Z + 1)],
		Weights[IndexFromCoord(id.X + 1, id.Y + 1, id.Z)],
		Weights[IndexFromCoord(id.X, id.Y + 1, id.Z)]
	 };

	int CubeIndex = 0;
	if (CubeValues[0] < GroundPercent) CubeIndex |= 1;
	if (CubeValues[1] < GroundPercent) CubeIndex |= 2;
	if (CubeValues[2] < GroundPercent) CubeIndex |= 4;
	if (CubeValues[3] < GroundPercent) CubeIndex |= 8;
	if (CubeValues[4] < GroundPercent) CubeIndex |= 16;
	if (CubeValues[5] < GroundPercent) CubeIndex |= 32;
	if (CubeValues[6] < GroundPercent) CubeIndex |= 64;
	if (CubeValues[7] < GroundPercent) CubeIndex |= 128;

	const int* edges = TriTable[CubeIndex];

	for (int i = 0; edges[i] != -1; i += 3)
	{
		// First edge lies between vertex e00 and vertex e01
		int e00 = EdgeConnections[edges[i]][0];
		int e01 = EdgeConnections[edges[i]][1];

		// Second edge lies between vertex e10 and vertex e11
		int e10 = EdgeConnections[edges[i + 1]][0];
		int e11 = EdgeConnections[edges[i + 1]][1];

		// Third edge lies between vertex e20 and vertex e21
		int e20 = EdgeConnections[edges[i + 2]][0];
		int e21 = EdgeConnections[edges[i + 2]][1];

		FTriangle Tri;
		
		Tri.a = InterpolateVertex(CornerOffsets[e00], CubeValues[e00], CornerOffsets[e01], CubeValues[e01]) + id;
		Tri.b = InterpolateVertex(CornerOffsets[e10], CubeValues[e10], CornerOffsets[e11], CubeValues[e11]) + id;
		Tri.c = InterpolateVertex(CornerOffsets[e20], CubeValues[e20], CornerOffsets[e21], CubeValues[e21]) + id;

		Triangles.Add(Tri);
		
		//Triangles.Add(Vertices.Num() - 2);

		UE_LOG(LogTemp, Warning, TEXT("Completed Marching"));
	}
}

int AMarchingChunk::IndexFromCoord(int x, int y, int z)
{
	return x + GridMetrics.PointsPerChunk * (y + GridMetrics.PointsPerChunk * z);
}

FVector AMarchingChunk::InterpolateVertex(FVector edgeVertex1, float valueAtVertex1, FVector edgeVertex2, float valueAtVertex2)
{
	return (edgeVertex1 + (GroundPercent - valueAtVertex1) * (edgeVertex2 - edgeVertex1) / (valueAtVertex2 - valueAtVertex1));
}

void AMarchingChunk::PopulateTerrainMap()
{
	if (Weights.Num() == 0) {
		return;
	}
	for (int x = 0; x < GridMetrics.PointsPerChunk; x++)
	{
		for (int y = 0; y < GridMetrics.PointsPerChunk; y++)
		{
			for (int z = 0; z < GridMetrics.PointsPerChunk; z++)
			{
				int index = x + GridMetrics.PointsPerChunk * (y + GridMetrics.PointsPerChunk * z);
				
				Weights[index] = GenerateNoise(FVector(x, y, z));
				
				March(FVector(x,y,z));
			}
		}
	}
}

void AMarchingChunk::ClearMeshData()
{
	
}

void AMarchingChunk::BuildMesh()
{
	TArray<FVector> Verts;
	TArray<int32> Tris;

	for (int32 i = 0; i < Triangles.Num(); i++) {
		int32 startIndex = i * 3;

		Verts.Add(FVector(Triangles[i].a.X, Triangles[i].a.Y, Triangles[i].a.Z));
		Verts.Add(FVector(Triangles[i].b.X, Triangles[i].b.Y, Triangles[i].b.Z));
		Verts.Add(FVector(Triangles[i].c.X, Triangles[i].c.Y, Triangles[i].c.Z));

		Tris.Add(startIndex);
		Tris.Add(startIndex + 1);
		Tris.Add(startIndex + 2);
	}
	ProceduralMesh->CreateMeshSection(0,
	Verts,
	Tris,
	TArray<FVector>(),
	TArray<FVector2D>(),
	TArray<FColor>(),
	TArray<FProcMeshTangent>(),
	true);
}

void AMarchingChunk::DrawDebugBoxes()
{
	if (Weights.Num() == 0) {
		return;
	}
	for (int x = 0; x < GridMetrics.PointsPerChunk; x++)
	{
		for (int y = 0; y < GridMetrics.PointsPerChunk; y++)
		{
			for (int z = 0; z < GridMetrics.PointsPerChunk; z++)
			{
				int index = x + GridMetrics.PointsPerChunk * (y + GridMetrics.PointsPerChunk * z);
				
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
	//UE_LOG(LogTemp, Warning, TEXT("Number of vertices: %i \n"), Weights.Num());
	for (int i = 0; i < Weights.Num(); i++)
	{
		Weights[i] = FMath::FRandRange(0.0f, 1.0f);
		
		//UE_LOG(LogTemp, Warning, TEXT("Vert: %f \n"), Weights[i]);
	}
}

float AMarchingChunk::GenerateNoise(FVector pos)
{
	Noise = new FastNoiseLite();
	Noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	Noise->SetFractalType(FastNoiseLite::FractalType_Ridged);
	Noise->SetFrequency(Frequency);
	Noise->SetFractalOctaves(Octaves);
	
	float Ground = -pos.Z + (GroundPercent * GridMetrics.PointsPerChunk);
	
	float n = Ground + Noise->GetNoise(pos.X, pos.Y, pos.Z) * Amplitude;
	//UE_LOG(LogTemp, Warning, TEXT("NoiseVal: %f \n"), n);
	
	return n;
}
