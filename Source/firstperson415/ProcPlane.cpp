// GAM 415 - Module 4: Procedural plane
// See ProcPlane.h for the graphics background (vertex/index buffers, winding, UVs).

#include "ProcPlane.h"
#include "ProceduralMeshComponent.h"   // UProceduralMeshComponent, FProcMeshTangent
#include "Materials/MaterialInterface.h"

AProcPlane::AProcPlane()
{
	// The geometry is built once; nothing changes per frame.
	PrimaryActorTick.bCanEverTick = false;

	// The procedural mesh is the root so the actor transform is the mesh transform, which is
	// the object-to-world matrix the vertex shader applies to every generated vertex.
	procMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Proc Mesh"));
	RootComponent = procMesh;
}

void AProcPlane::BeginPlay()
{
	Super::BeginPlay();
}

void AProcPlane::PostActorCreated()
{
	// Super first: skipping it leaves the actor half initialized and crashed the editor in the tutorial.
	Super::PostActorCreated();
	CreateMesh();
}

void AProcPlane::PostLoad()
{
	Super::PostLoad();
	CreateMesh();
}

void AProcPlane::CreateMesh()
{
	if (!procMesh)
	{
		return;
	}

	// Section 0 is one draw call: one vertex buffer, one index buffer, one material.
	// Normals, vertex colors and tangents are passed as empty arrays, so the component fills
	// in defaults (flat normal, white). The last argument creates collision from the same
	// triangles, so the plane can be stood on and hit by traces.
	procMesh->CreateMeshSection(0, Vertices, Triangles, TArray<FVector>(), UV0, TArray<FColor>(), TArray<FProcMeshTangent>(), true);

	// The material is applied after the section exists. In the tutorial it was first set in
	// the constructor, where the Blueprint value is not loaded yet, so it never showed up.
	if (PlaneMat)
	{
		procMesh->SetMaterial(0, PlaneMat);
	}
}
