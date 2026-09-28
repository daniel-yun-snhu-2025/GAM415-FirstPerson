// GAM 415 - Module 4: Perlin noise procedural terrain
// See PerlinProcTerrain.h for the graphics background (grid, noise, vertex buffer updates).

#include "PerlinProcTerrain.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"

APerlinProcTerrain::APerlinProcTerrain()
{
	// The terrain only changes when a projectile hits it, never per frame.
	PrimaryActorTick.bCanEverTick = false;

	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Procedural Mesh"));
	RootComponent = ProcMesh;
}

void APerlinProcTerrain::BeginPlay()
{
	Super::BeginPlay();

	CreateVertices();
	CreateTriangles();

	// Upload the grid as one section with collision, so projectiles and the player hit the
	// generated surface itself.
	ProcMesh->CreateMeshSection(sectionID, Vertices, Triangles, Normals, UV0, UpVertexColors, TArray<FProcMeshTangent>(), true);
	ProcMesh->SetMaterial(0, Mat);
}

void APerlinProcTerrain::AlterMesh(FVector impactPoint)
{
	// Vertices are stored in the actor's local space, the hit point is in world space.
	// Subtracting the actor location lines them up, so the hole appears where the shot landed
	// even when the terrain is not placed at the world origin.
	FVector tempVector = impactPoint - this->GetActorLocation();

	bool bChanged = false;
	for (int i = 0; i < Vertices.Num(); i++)
	{
		// Distance from this vertex to the hit point; inside the radius it gets pushed down.
		if (FVector(Vertices[i] - tempVector).Size() < radius)
		{
			Vertices[i] = Vertices[i] - Depth;
			bChanged = true;
		}
	}

	// The tutorial calls UpdateMeshSection inside the loop, once per moved vertex. Each call
	// re-uploads the whole vertex buffer, so it is done once after the loop instead. Only
	// positions change; the index buffer and the triangle count stay the same.
	if (bChanged)
	{
		ProcMesh->UpdateMeshSection(sectionID, Vertices, Normals, UV0, UpVertexColors, TArray<FProcMeshTangent>());
	}
}

void APerlinProcTerrain::CreateVertices()
{
	// Row by row: for each X, walk every Y. The <= gives XSize + 1 by YSize + 1 vertices,
	// one more than the number of cells in each direction.
	for (int X = 0; X <= XSize; X++)
	{
		for (int Y = 0; Y <= YSize; Y++)
		{
			// The + 0.1 keeps the samples off whole numbers, where Perlin noise is always 0.
			float Z = FMath::PerlinNoise2D(FVector2D(X * NoiseScale + 0.1, Y * NoiseScale + 0.1)) * ZMultiplier;
			Vertices.Add(FVector(X * Scale, Y * Scale, Z));
			UV0.Add(FVector2D(X * UVScale, Y * UVScale));
		}
	}
}

void APerlinProcTerrain::CreateTriangles()
{
	// Vertex is the index of the current cell's corner at (X, Y). The vertex on the next row
	// is YSize + 1 further along the array, because each row holds YSize + 1 vertices.
	int Vertex = 0;

	for (int X = 0; X < XSize; X++)
	{
		for (int Y = 0; Y < YSize; Y++)
		{
			// Two triangles per cell, same winding order as the procedural plane.
			Triangles.Add(Vertex);
			Triangles.Add(Vertex + 1);
			Triangles.Add(Vertex + YSize + 1);
			Triangles.Add(Vertex + 1);
			Triangles.Add(Vertex + YSize + 2);
			Triangles.Add(Vertex + YSize + 1);

			Vertex++;
		}

		// Skip the last vertex of the row: it has no cell to its +Y side.
		Vertex++;
	}
}
