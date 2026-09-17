// GAM 415 - Module 2: Dynamic Material Instance cube
// See CubeDMIMod.h for the graphics background (materials, instances, uniform buffers).

#include "CubeDMIMod.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"   // UNiagaraFunctionLibrary::SpawnSystemAttached
#include "NiagaraComponent.h"         // UNiagaraComponent: the spawned instance we set variables on
#include "firstperson415Character.h"

ACubeDMIMod::ACubeDMIMod()
{
	// Nothing changes per frame in C++; all visual change is driven by material parameters,
	// so the actor does not need to tick.
	PrimaryActorTick.bCanEverTick = false;

	// Create the sub objects. CreateDefaultSubobject builds the components as part of the
	// class default object, so every instance (and the Blueprint child class) starts with them.
	boxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Component"));
	cubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube Mesh"));

	// Set up attachments: the box is the root, the cube mesh hangs off it.
	// The attachment hierarchy is what gives the mesh its world transform; that transform
	// becomes the per-object matrix the vertex shader uses to place the cube in the scene.
	RootComponent = boxComp;
	cubeMesh->SetupAttachment(boxComp);

	// 100 cm half extents -> a 2 m trigger volume around the 1 m cube.
	boxComp->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	// Overlap (not block) with everything dynamic so the player can walk into the volume.
	boxComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	boxComp->SetGenerateOverlapEvents(true);
}

void ACubeDMIMod::BeginPlay()
{
	Super::BeginPlay();

	// Listen for overlaps on the box. This is a physics/gameplay event, delivered on the
	// game thread, which is also the only thread allowed to change material parameters.
	boxComp->OnComponentBeginOverlap.AddDynamic(this, &ACubeDMIMod::OnOverlapBegin);

	// Create the dynamic material instance from the base material and apply it to the cube.
	if (baseMat)
	{
		// UMaterialInstanceDynamic::Create does NOT compile any shaders: it wraps the already
		// compiled base material and gives this actor a private set of parameter values.
		// "this" is the outer, so the instance is owned by (and garbage collected with) the cube.
		dmiMat = UMaterialInstanceDynamic::Create(baseMat, this);

		if (cubeMesh)
		{
			// Material slot 0 = first material element of the static mesh. From now on the
			// cube's draw call binds this instance's uniform buffer instead of the base material's.
			cubeMesh->SetMaterial(0, dmiMat);
		}
	}
}

void ACubeDMIMod::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only react to the player character. OtherActor is the actor that entered the box;
	// the Cast returns nullptr for anything that is not (derived from) our character class.
	Afirstperson415Character* overlappedActor = Cast<Afirstperson415Character>(OtherActor);

	if (overlappedActor && dmiMat)
	{
		// Random RGB in [0,1]. Material colors are linear-space floats (FLinearColor), not
		// 0-255 sRGB bytes; the GPU receives them as a float4 in the material uniform buffer.
		float ranNumX = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
		float ranNumY = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
		float ranNumZ = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);

		// Alpha stays 1: the fourth component of the vector parameter is not what drives
		// translucency here, the separate "Opacity" scalar parameter is.
		FLinearColor randColor = FLinearColor(ranNumX, ranNumY, ranNumZ, 1.0f);

		// Each Set*ParameterValue call updates one entry in the instance's parameter list and
		// marks the render-thread proxy dirty; the new uniform buffer contents are uploaded
		// before the next frame draws the cube. No mesh data or shaders are touched.
		dmiMat->SetVectorParameterValue(TEXT("Color"), randColor);      // feeds Base Color (multiplied by Darkness)
		dmiMat->SetScalarParameterValue(TEXT("Darkness"), ranNumX);     // scales Base Color; reusing X as in the tutorial

		// Assignment requirement: also randomize the opacity. Kept in a visible range so the
		// cube never becomes fully invisible. Because the material is Translucent, this value
		// is the blend factor the output-merger uses against the already rendered scene.
		float ranOpacity = UKismetMathLibrary::RandomFloatInRange(0.2f, 1.0f);
		dmiMat->SetScalarParameterValue(TEXT("Opacity"), ranOpacity);

		// Module 3: burst of particles in the same random color.
		// colorP must be checked: calling the spawn function with a null system asserts/crashes,
		// which is exactly what happens if the Blueprint property was left empty.
		if (colorP)
		{
			// SpawnSystemAttached creates a UNiagaraComponent (one running instance of the
			// system) and parents it to the component that overlapped us, at zero offset.
			// Arguments: template, attach parent, socket/bone (none), relative location,
			// relative rotation, how to interpret that transform, auto-destroy when finished.
			UNiagaraComponent* particleComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				colorP, OtherComp, NAME_None, FVector(0.0f), FRotator(0.0f),
				EAttachLocation::KeepRelativeOffset, true);

			if (particleComp)
			{
				// Same idea as a material parameter: the emitters are already compiled, we only
				// override the value of the exposed user parameter for THIS instance. Initialize
				// Particle copies it into each particle's Color attribute at spawn, and the sprite
				// renderer passes that attribute to the material as the Particle Color node.
				particleComp->SetVariableLinearColor(FName("RandColor"), randColor);
			}
		}
	}
}
