// GAM 415 - Module 4: Procedural mesh copied from a static mesh
//
// Graphics background for this class:
//  - A static mesh asset already stores its geometry as render data: a vertex buffer
//    (position, normal, tangent, UVs) and an index buffer per LOD and section.
//  - UKismetProceduralMeshLibrary::GetSectionFromStaticMesh reads one LOD/section of that
//    data back into CPU arrays. Handing those arrays to a UProceduralMeshComponent gives an
//    editable copy of the shape, so it can later be sliced or have its vertices moved at
//    runtime, which a static mesh cannot do without re-importing.
//  - Normals and tangents are copied too. The pixel shader uses them to build the tangent
//    space for lighting and normal maps, so the copy is lit exactly like the original.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"   // full include: FProcMeshTangent is a struct stored by value, a forward declaration is not enough
#include "ProcMeshFromStatic.generated.h"

class UStaticMeshComponent;

/**
 *  Actor that copies the geometry of baseMesh into a procedural mesh.
 *  In ProcMeshFromStatic_BP the base mesh is hidden in game and has no collision, so what
 *  is seen and collided with is the procedural copy.
 */
UCLASS()
class FIRSTPERSON415_API AProcMeshFromStatic : public AActor
{
	GENERATED_BODY()

public:
	AProcMeshFromStatic();

protected:
	virtual void BeginPlay() override;

public:
	/** Rebuild the copy when the actor is placed, so it shows in the editor viewport. */
	virtual void PostActorCreated() override;

	/** Rebuild the copy when a saved level is loaded. */
	virtual void PostLoad() override;

	/** Vertex positions read from the static mesh (vertex buffer, position stream). */
	UPROPERTY()
	TArray<FVector> Vertices;

	/** Triangle indices read from the static mesh (index buffer). */
	UPROPERTY()
	TArray<int> Triangles;

	/** Per-vertex normals, used for lighting. */
	UPROPERTY()
	TArray<FVector> Normals;

	/** First UV channel, used to map textures. */
	TArray<FVector2D> UV0;

	/** Linear (float) vertex colors, kept for the linear color version of the section functions. */
	UPROPERTY()
	TArray<FLinearColor> VertexColors;

	/** 8 bit vertex colors; left empty, so the section defaults to white. */
	TArray<FColor> UpVertexColors;

	/** Per-vertex tangents; together with the normals they form the tangent basis for normal mapping. */
	TArray<FProcMeshTangent> Tangents;

	/** Source shape. Its render data is read, then the component itself stays hidden. */
	UPROPERTY(EditAnywhere, Category = "Procedural")
	UStaticMeshComponent* baseMesh;

private:
	/** The editable copy that is actually rendered. */
	UPROPERTY(VisibleAnywhere, Category = "Procedural")
	UProceduralMeshComponent* procMesh;

	/** Reads LOD 0, section 0 of baseMesh into the arrays above, then builds the procedural section. */
	void GetMeshData();

	/** Uploads the arrays as procedural mesh section 0 (with collision). */
	void CreateMesh();
};
