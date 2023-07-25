// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Engine/StaticMesh.h"
#include "Utility/FastNoiseLite.h"
#include "Utility/GridMetrics.h"
#include "Materials/MaterialInterface.h"

#include "ProceduralMeshComponent.h"
#include "Math/UnrealMathUtility.h"

#include "MarchingChunk.generated.h"

struct FTriangle
{
	FVector a;
	FVector b;
	FVector c;
};

UCLASS()
class MARCHINGCUBES_API AMarchingChunk : public AActor
{
	GENERATED_BODY()
public:	
	AMarchingChunk();
	virtual void Tick(float DeltaTime) override;
	
	int IndexFromCoord(int x, int y, int z) const;

	void UpdateMesh();
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category=Mesh)
	UMaterialInterface* Material;
	
private:
	FVector InterpolateVertex(FVector edgeVertex1, float valueAtVertex1, FVector edgeVertex2, float valueAtVertex2) const;

	void Initialize();
	void March(FVector id);
	void PopulateTerrainMap();
	void GenerateMeshData(TArray<FTriangle> triangles);
	void ConstructMesh();
	
	void DrawDebugBoxes();
	void GenerateRandomVals();
	float GenerateNoise(FVector pos);
	TArray<FVector2D> GenerateUVMap();
	TArray<FVector> CalcAverageNormals(TArray<FVector> verts, TArray<int32> tris);
	
public:
	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UVMap;

	float time = 6.0;
	
	TArray<FTriangle> Triangles;
	
	TArray<float> Weights;
	FGridMetrics GridMetrics;

	UPROPERTY(EditAnywhere, Category=Mesh)
	UProceduralMeshComponent* ProceduralMesh;

	FastNoiseLite* Noise;
	UPROPERTY(EditAnywhere, Category=Marching)
	float IsoLevel = 0.5f;
	
	// Seed for random variation (Default: 1337)
	UPROPERTY(EditAnywhere, Category=Noise)
	int32 Seed = 1337;
	// The amplitude determines how high our terrain will reach, high amplitude means high mountains.
	UPROPERTY(EditAnywhere, Category=Noise)
	float Amplitude = 5.0f;
	// Frequency tells us where to sample noise, when we have a high frequency, we will sample the noise further away from eachother. This can give us quite chaotic noise as the sampling will be less coherent.
	UPROPERTY(EditAnywhere, Category=Noise)
	float Frequency = 0.005f;
	// Octaves gives us details, when the noise is generated for a point, we will essentially regenerate it but at a smaller sample size and this for the amount of octaves.
	UPROPERTY(EditAnywhere, Category=Noise)
	int Octaves = 8;
	// The ground percent just tells us where along the height of the chunk we want to be above ground. Note: must be from 0-1
	UPROPERTY(EditAnywhere, Category=Noise)
	float GroundPercent = 0.2f;
	UPROPERTY(EditAnywhere, Category=Noise)
	float HardFloorZ = 3.f;
	UPROPERTY(EditAnywhere, Category=Noise)
	int TerraceHeight = 5;

	int GetTriangleCount() const { return Triangles.Num(); }
};


