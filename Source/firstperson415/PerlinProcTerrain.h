// GAM 415 - Module 4: Procedural terrain from Perlin noise that projectiles can dig into
//
// Graphics background for this class:
//  - The terrain is a regular grid of (XSize + 1) x (YSize + 1) vertices. Each grid cell is
//    a quad drawn as two triangles, so the index buffer holds XSize * YSize * 6 indices.
//  - Height comes from 2D Perlin noise: a smooth, repeatable random function. Sampling it
//    at X * NoiseScale, Y * NoiseScale gives neighboring vertices similar heights, so the
//    surface rolls instead of looking like static.
//  - UVs are generated from the same grid indices, so UVScale sets how many times the
//    material's texture repeats across the terrain.
//  - Digging moves vertices on the CPU and calls UpdateMeshSection, which re-uploads the
//    vertex buffer. The index buffer and triangle count never change, only positions do.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PerlinProcTerrain.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 *  Grid terrain generated at BeginPlay. AShooterProjectile calls AlterMesh with the hit
 *  point, which lowers every vertex within radius by Depth.
 *
 *  Values used in ProcTerrainMap (from the tutorial): XSize 100, YSize 100, ZMultiplier 500,
 *  NoiseScale 0.1, Scale 100, UVScale 1, radius 200, Depth (0,0,50).
 */
UCLASS()
class FIRSTPERSON415_API APerlinProcTerrain : public AActor
{
	GENERATED_BODY()

public:
	APerlinProcTerrain();

	/** Number of grid cells along X. Vertices along X = XSize + 1. */
	UPROPERTY(EditAnywhere, Category = "Procedural", Meta = (ClampMin = 0))
	int XSize = 0;

	/** Number of grid cells along Y. Vertices along Y = YSize + 1. */
	UPROPERTY(EditAnywhere, Category = "Procedural", Meta = (ClampMin = 0))
	int YSize = 0;

	/** Height multiplier. Perlin noise returns roughly -1..1, so this is the peak height in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural", Meta = (ClampMin = 0))
	float ZMultiplier = 1.0f;

	/**
	 *  How far apart neighboring vertices sample the noise. Small values zoom in on the noise
	 *  (smooth hills), large values zoom out (sharp ridges).
	 */
	UPROPERTY(EditAnywhere, Category = "Procedural", Meta = (ClampMin = 0))
	float NoiseScale = 1.0f;

	/** Distance between vertices in cm, i.e. the size of one grid cell. */
	UPROPERTY(EditAnywhere, Category = "Procedural", Meta = (ClampMin = 0.000001))
	float Scale = 0;

	/** UV step per grid cell. 1 = the texture repeats once per cell. */
	UPROPERTY(EditAnywhere, Category = "Procedural", Meta = (ClampMin = 0.000001))
	float UVScale = 0;

	/** Dig radius in cm around the impact point. */
	UPROPERTY(EditAnywhere, Category = "Procedural")
	float radius;

	/** Offset subtracted from each vertex inside the radius. (0,0,50) digs 50 cm down; a negative Z builds up. */
	UPROPERTY(EditAnywhere, Category = "Procedural")
	FVector Depth;

protected:
	virtual void BeginPlay() override;

	/** Material for the terrain section. If empty, the engine default (world grid) material is used. */
	UPROPERTY(EditAnywhere, Category = "Procedural")
	UMaterialInterface* Mat;

public:
	/**
	 *  Lowers every vertex within radius of impactPoint by Depth and re-uploads the vertex
	 *  buffer. impactPoint is in world space.
	 */
	UFUNCTION()
	void AlterMesh(FVector impactPoint);

private:
	UPROPERTY(VisibleAnywhere, Category = "Procedural")
	UProceduralMeshComponent* ProcMesh;

	TArray<FVector> Vertices;
	TArray<int> Triangles;
	TArray<FVector2D> UV0;
	TArray<FVector> Normals;
	TArray<FColor> UpVertexColors;

	/** Mesh section that holds the whole grid. One section = one draw call and one vertex buffer. */
	int sectionID = 0;

	/** Fills Vertices and UV0 row by row from the noise function. */
	void CreateVertices();

	/** Fills Triangles with two triangles per grid cell. */
	void CreateTriangles();
};
