// GAM 415 - Module 4: Procedural plane built from hard coded vertices, triangles and UVs
//
// Graphics background for this class:
//  - Every mesh the GPU draws is a list of vertices (positions plus attributes such as UVs)
//    and an index list that says which three vertices form each triangle. The GPU only
//    rasterizes triangles; a modeling package's quads are split ("triangulated") on import.
//  - UProceduralMeshComponent lets C++ fill those two lists at runtime. CreateMeshSection
//    copies them into a vertex buffer and an index buffer on the GPU and builds a render
//    proxy, so after that the plane is drawn like any static mesh.
//  - Winding order matters: the rasterizer decides front or back face from the order the
//    three vertices appear on screen, and back faces are culled for a one sided material.
//    The index order below (origin -> +Y -> +X, as in the tutorial) makes the plane visible
//    from above (+Z); reversing it would make it visible only from below.
//  - UVs are a second set of per-vertex coordinates in 0..1 texture space. Without them
//    every pixel samples the same texel, which is why a textured material looks blank
//    until UV0 is filled in.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProcPlane.generated.h"

class UProceduralMeshComponent;   // forward declaration: the header only stores a pointer
class UMaterialInterface;

/**
 *  Actor that builds a 100 x 100 plane (two triangles) from arrays set in the Blueprint.
 *
 *  ProcPlane_BP class defaults:
 *    Vertices  = (0,0,0) (0,100,0) (100,0,0) (100,100,0)
 *    Triangles = 0,1,2, 2,1,3
 *    UV0       = (0,0) (0,1) (1,0) (1,1)
 */
UCLASS()
class FIRSTPERSON415_API AProcPlane : public AActor
{
	GENERATED_BODY()

public:
	AProcPlane();

protected:
	virtual void BeginPlay() override;

public:
	/**
	 *  Called when the actor is placed or spawned in the editor/world.
	 *  Building the mesh here (and in PostLoad) means the plane is visible in the editor
	 *  viewport before Play, not only after BeginPlay.
	 */
	virtual void PostActorCreated() override;

	/** Called when a saved level that contains this actor is loaded. */
	virtual void PostLoad() override;

	/**
	 *  Vertex positions in local space (cm). These become the vertex buffer.
	 *  Index 0 is the origin; the plane spans 100 cm in X and Y.
	 */
	UPROPERTY(EditAnywhere, Category = "Procedural")
	TArray<FVector> Vertices;

	/**
	 *  Index buffer: every three entries are one triangle, pointing into Vertices.
	 *  0,1,2 then 2,1,3 draws the two halves of the quad with the same winding so both
	 *  face the same way.
	 */
	UPROPERTY(EditAnywhere, Category = "Procedural")
	TArray<int> Triangles;

	/**
	 *  Texture coordinates, one per vertex, in 0..1. They describe where each vertex sits in
	 *  the texture, not in the world, so a 100 cm or a 500 cm plane both use (1,1) for the far
	 *  corner. The rasterizer interpolates them across each triangle for the pixel shader.
	 */
	UPROPERTY(EditAnywhere, Category = "Procedural")
	TArray<FVector2D> UV0;

	/**
	 *  Material for section 0 (Plane_MAT, a grid checker texture). Optional, so it is checked
	 *  before use; a null material would fall back to the engine default material.
	 */
	UPROPERTY(EditAnywhere, Category = "Procedural")
	UMaterialInterface* PlaneMat;

	/** Uploads Vertices / Triangles / UV0 as mesh section 0 and applies PlaneMat. */
	void CreateMesh();

private:
	/** Component that owns the generated GPU buffers and the render proxy. */
	UPROPERTY(VisibleAnywhere, Category = "Procedural")
	UProceduralMeshComponent* procMesh;
};
