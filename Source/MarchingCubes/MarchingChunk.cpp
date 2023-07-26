// Fill out your copyright notice in the Description page of Project Settings.


#include "MarchingChunk.h"

#include "Utility/MarchingTable.h"
#include "DrawDebugHelpers.h"

AMarchingChunk::AMarchingChunk()
{
	PrimaryActorTick.bCanEverTick = false;
	
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	RootComponent = ProceduralMesh;
	
	ProceduralMesh->SetRelativeScale3D(FVector(GridMetrics.Distance));

	// Initialize size of array to number of cubes in our grid (x * y * z)
	Weights.SetNum(GridMetrics.PointsPerChunk * GridMetrics.PointsPerChunk * GridMetrics.PointsPerChunk);
}

void AMarchingChunk::BeginPlay()
{
	Super::BeginPlay();

	//Seed = FMath::Rand();

	UE_LOG(LogTemp, Warning, TEXT("X: %i, Y: %i"), InitialX, InitialY);
	//DrawDebugBoxes();
}

void AMarchingChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMarchingChunk::March(FVector id)
{
	// Check whether we are inside of our grid
	if (id.X >= (GridMetrics.PointsPerChunk - 1) || id.Y >= (GridMetrics.PointsPerChunk) - 1 || id.Z >= (GridMetrics.PointsPerChunk - 1))
	{
		return;
	}

	// Next, get the noise values at the corners of our cubes
	float CubeValues[8] = {
		Weights[IndexFromCoord(id.X,		id.Y,		  id.Z + 1)],
		Weights[IndexFromCoord(id.X + 1,	id.Y,		  id.Z + 1)],
		Weights[IndexFromCoord(id.X + 1,	id.Y,			id.Z)],
		Weights[IndexFromCoord(id.X,		id.Y,			id.Z)],
		Weights[IndexFromCoord(id.X,		id.Y + 1,	  id.Z + 1)],
		Weights[IndexFromCoord(id.X + 1,	id.Y + 1,	  id.Z + 1)],
		Weights[IndexFromCoord(id.X + 1,	id.Y + 1,		id.Z)],
		Weights[IndexFromCoord(id.X,		id.Y + 1,		id.Z)]
	 };

	// Get the cube configuration
	int CubeIndex = 0;
	if (CubeValues[0] < IsoLevel) CubeIndex |= 1;
	if (CubeValues[1] < IsoLevel) CubeIndex |= 2;
	if (CubeValues[2] < IsoLevel) CubeIndex |= 4;
	if (CubeValues[3] < IsoLevel) CubeIndex |= 8;
	if (CubeValues[4] < IsoLevel) CubeIndex |= 16;
	if (CubeValues[5] < IsoLevel) CubeIndex |= 32;
	if (CubeValues[6] < IsoLevel) CubeIndex |= 64;
	if (CubeValues[7] < IsoLevel) CubeIndex |= 128;

	// Get the triangle indices
	const int* Edges = TriTable[CubeIndex];
	
	for (int i = 0; Edges[i] != -1; i += 3)
	{
		// First edge lies between vertex e00 and vertex e01
		int e00 = EdgeConnections[Edges[i]][0];
		int e01 = EdgeConnections[Edges[i]][1];

		// Second edge lies between vertex e10 and vertex e11
		int e10 = EdgeConnections[Edges[i + 1]][0];
		int e11 = EdgeConnections[Edges[i + 1]][1];

		// Third edge lies between vertex e20 and vertex e21
		int e20 = EdgeConnections[Edges[i + 2]][0];
		int e21 = EdgeConnections[Edges[i + 2]][1];

		FTriangle Tri;

		// Interpolate between the points on the edge to find the exact position for the vertices of the triangles
		Tri.a = InterpolateVertex(CornerOffsets[e00], CubeValues[e00], CornerOffsets[e01], CubeValues[e01]) + id;
		Tri.b = InterpolateVertex(CornerOffsets[e10], CubeValues[e10], CornerOffsets[e11], CubeValues[e11]) + id;
		Tri.c = InterpolateVertex(CornerOffsets[e20], CubeValues[e20], CornerOffsets[e21], CubeValues[e21]) + id;
		
		// Add our triangle to the list.
		Triangles.Add(Tri);
	}
}

void AMarchingChunk::UpdateMesh()
{
	if (ProceduralMesh)
	{
		// Update the procedural mesh component with the modified vertex data

		// Recalculate normals
		Normals = CalcAverageNormals(Verts, Tris);
		
		// Notify the procedural mesh component that it needs to update
		ProceduralMesh->UpdateMeshSection(0, Verts, 	Normals,
		UVMap,
		TArray<FColor>(),
		TArray<FProcMeshTangent>());

		ProceduralMesh->SetMaterial(0, Material);
	}
}

void AMarchingChunk::ClearMesh()
{
	TArray<FVector> EmptyVertices;
	TArray<FVector> EmptyNormals;
	TArray<FVector2D> EmptyUVs;
	TArray<FColor> EmptyVertexColors;
	TArray<FProcMeshTangent> EmptyTangents;

	// Clear existing mesh sections by updating them with empty data
	ProceduralMesh->UpdateMeshSection(0, EmptyVertices, EmptyNormals, EmptyUVs, EmptyVertexColors, EmptyTangents);
}

int AMarchingChunk::IndexFromCoord(int x, int y, int z) const
{
	return x + GridMetrics.PointsPerChunk * (y + GridMetrics.PointsPerChunk * z);
}

FVector AMarchingChunk::InterpolateVertex(FVector edgeVertex1, float valueAtVertex1, FVector edgeVertex2, float valueAtVertex2) const
{
	return (edgeVertex1 + (IsoLevel - valueAtVertex1) * (edgeVertex2 - edgeVertex1) / (valueAtVertex2 - valueAtVertex1));
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
			}
		}
	}
}

void AMarchingChunk::GenerateMeshData(TArray<FTriangle> triangles)
{
	// Invert the normals by changing the order of vertex indices in Tris array.
	// Instead of adding the indices in order, add them in reverse order.
	for (int32 i = 0; i < triangles.Num(); i++)
	{
		int32 startIndex = i * 3;

		Verts.Add(FVector(triangles[i].a.X, triangles[i].a.Y, triangles[i].a.Z));
		Verts.Add(FVector(triangles[i].b.X, triangles[i].b.Y, triangles[i].b.Z));
		Verts.Add(FVector(triangles[i].c.X, triangles[i].c.Y, triangles[i].c.Z));

		// Add indices in reverse order to invert the normals.
		Tris.Add(startIndex + 2);
		Tris.Add(startIndex + 1);
		Tris.Add(startIndex);
	}
	Normals = CalcAverageNormals(Verts, Tris);
	UVMap = GenerateUVMap();
	ConstructMesh();
}

void AMarchingChunk::Initialize()
{
	Triangles.Empty();
	for (int x = 0; x < GridMetrics.PointsPerChunk; x++)
	{
		for (int y = 0; y < GridMetrics.PointsPerChunk; y++)
		{
			for (int z = 0; z < GridMetrics.PointsPerChunk; z++)
			{
				March(FVector(x,y,z));
			}
		}
	}
	GenerateMeshData(Triangles);
}

void AMarchingChunk::ConstructMesh()
{
	if (ProceduralMesh)
	{
		ProceduralMesh->ClearAllMeshSections();
	
		ProceduralMesh->CreateMeshSection(0,
		Verts,
		Tris,
		Normals,
		UVMap,
		TArray<FColor>(),
		TArray<FProcMeshTangent>(),
		true);
	}
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
					DrawDebugPoint(
						World,
						FVector(x,y,z) * GridMetrics.Distance,
						2,
						Color,
						true
					);
				}
			}
		}
	}
}

float AMarchingChunk::GenerateNoise(FVector pos)
{
	Noise = new FastNoiseLite();
	
	Noise->SetSeed(Seed);
	
	Noise->SetSeed(Seed);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	Noise->SetFractalType(FastNoiseLite::FractalType_Ridged);
	Noise->SetFrequency(Frequency);
	Noise->SetFractalOctaves(Octaves);
	
	float Ground = -pos.Z + (GroundPercent * GridMetrics.PointsPerChunk);
	
	float NoiseValue = Noise->GetNoise(pos.X + InitialX * GridMetrics.PointsPerChunk - 1,
									pos.Y + InitialY * GridMetrics.PointsPerChunk - 1,
									pos.Z) * Amplitude;
	
	float HardFloorInfluence = FMath::Clamp((((HardFloorZ - pos.Z) * 3.0f)),0,1) * 40.0f; // Adjust the multiplier as needed
	float Terracing = static_cast<int>(pos.Z) % TerraceHeight;
	
	float n = + Ground + NoiseValue + HardFloorInfluence + Terracing;
	
	return n;
}

TArray<FVector2D> AMarchingChunk::GenerateUVMap()
{
	TArray<FVector2D> UV;
	float UVScale = 1.0f;
	for (int x = 0; x < GridMetrics.PointsPerChunk; x++)
	{
		for (int y = 0; y < GridMetrics.PointsPerChunk; y++)
		{
			// Scale the x and y values to the range [0, 1]
			float u = static_cast<float>(x) / (GridMetrics.PointsPerChunk - 1);
			float v = static_cast<float>(y) / (GridMetrics.PointsPerChunk - 1);

			UV.Add(FVector2D(u * UVScale, v * UVScale));
		}
	}
	return UV;
}

TArray<FVector> AMarchingChunk::CalcAverageNormals(TArray<FVector> verts, TArray<int32> tris)
{
	TArray<FVector> vertexNormals;
	vertexNormals.Init(FVector::ZeroVector, verts.Num());
	// Iterates through each triangle in the mesh
	for (int32 i = 0; i < tris.Num(); i += 3)
	{
		int32 i0 = tris[i + 2];
		int32 i1 = tris[i + 1];
		int32 i2 = tris[i];

		FVector v1 = verts[i1] - verts[i0];
		FVector v2 = verts[i2] - verts[i0];

		FVector faceNormal = FVector::CrossProduct(v1, v2).GetSafeNormal();

		// Accumulate the face normal to the corresponding vertices.
		vertexNormals[i0] += faceNormal;
		vertexNormals[i1] += faceNormal;
		vertexNormals[i2] += faceNormal;
	}
	// Normalize the accumulated normals for each vertex to get the average normals.
	for (FVector& normal : vertexNormals)
	{
		normal.Normalize();
	}
	return vertexNormals;
}