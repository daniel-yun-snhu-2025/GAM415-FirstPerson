// GAM 415 - Module 4: Procedural mesh copied from a static mesh
// See ProcMeshFromStatic.h for the graphics background.

#include "ProcMeshFromStatic.h"
#include "KismetProceduralMeshLibrary.h"   // GetSectionFromStaticMesh
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AProcMeshFromStatic::AProcMeshFromStatic()
{
	PrimaryActorTick.bCanEverTick = false;

	procMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Proc Mesh"));
	baseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base Mesh"));

	// Make the procedural mesh the root explicitly. SetupAttachment(GetRootComponent()) would
	// also end up as the root here, but it reads as if it were attached to something else.
	RootComponent = procMesh;
	baseMesh->SetupAttachment(procMesh);
}

void AProcMeshFromStatic::BeginPlay()
{
	Super::BeginPlay();
}

void AProcMeshFromStatic::PostActorCreated()
{
	Super::PostActorCreated();
	GetMeshData();
}

void AProcMeshFromStatic::PostLoad()
{
	Super::PostLoad();
	GetMeshData();
}

void AProcMeshFromStatic::GetMeshData()
{
	if (!baseMesh || !procMesh)
	{
		return;
	}

	// The library function needs the asset (UStaticMesh), not the component that renders it.
	UStaticMesh* mesh = baseMesh->GetStaticMesh();
	if (mesh)
	{
		// Copy LOD 0, section 0 of the asset's render data into CPU side arrays.
		UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(mesh, 0, 0, Vertices, Triangles, Normals, UV0, Tangents);

		// If section 0 already exists (the level was reloaded), refresh its vertex buffer in place.
		procMesh->UpdateMeshSection(0, Vertices, Normals, UV0, UpVertexColors, Tangents);

		CreateMesh();
	}
}

void AProcMeshFromStatic::CreateMesh()
{
	if (baseMesh)
	{
		// Build section 0 from the copied data: new GPU vertex/index buffers plus collision
		// generated from the same triangles.
		procMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, UpVertexColors, Tangents, true);
	}
}
