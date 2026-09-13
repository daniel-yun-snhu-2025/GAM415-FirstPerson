// GAM 415 - Module 2: Dynamic Material Instance cube
//
// Graphics background for this class:
//  - A UMaterial (DMI1_Mat) is compiled by the engine into shader programs that the RHI
//    (Render Hardware Interface) submits to the platform graphics API (Direct3D 12, Vulkan
//    or Metal). Compiling shaders is expensive, so we never do it at runtime.
//  - A Material Instance reuses those compiled shaders and only overrides parameter values.
//    A *Dynamic* Material Instance (UMaterialInstanceDynamic, "DMI") is created in code at
//    runtime, so each actor can own its own copy and change parameters every frame without
//    touching the shared material asset or any other instance.
//  - Parameter values (vector = float4, scalar = float) live in the material's uniform
//    buffer (constant buffer) on the GPU. Setting a parameter only updates that small
//    buffer on the render thread; the draw call, mesh buffers and pipeline state stay the same.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubeDMIMod.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 *  Cube that creates a Dynamic Material Instance from a base material and
 *  randomizes its Color / Darkness / Opacity parameters when the player overlaps it.
 *
 *  DMI1_Mat graph (built in the Material Editor):
 *    VectorParameter "Color" * ScalarParameter "Darkness" -> Base Color
 *    ScalarParameter "Opacity"                            -> Opacity
 *    Blend Mode = Translucent, so the Opacity output is used by the GPU blend stage to mix
 *    the cube with what is already in the frame buffer (rendered after the opaque pass).
 */
UCLASS()
class FIRSTPERSON415_API ACubeDMIMod : public AActor
{
	GENERATED_BODY()

public:
	ACubeDMIMod();

protected:
	/** Called when the level starts; this is where the DMI is created and bound to the mesh. */
	virtual void BeginPlay() override;

public:
	/**
	 *  Overlap volume, used as the root component.
	 *  Collision is a physics-scene concept, not a rendering one: the box never draws anything,
	 *  it only reports when another primitive enters it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UBoxComponent* boxComp;

	/**
	 *  Visible cube mesh that receives the dynamic material.
	 *  A static mesh component is what actually produces a draw call: the mesh supplies the
	 *  vertex/index buffers, and the material assigned to element 0 supplies the shaders and
	 *  render state (blend mode, two-sided, etc.) for that draw.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* cubeMesh;

	/**
	 *  Base material the dynamic instance is created from (DMI1_Mat).
	 *  UMaterialInterface is the common parent of UMaterial and UMaterialInstance, so a
	 *  designer can plug in either the material or a pre-made instance in the Blueprint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	UMaterialInterface* baseMat;

	/**
	 *  Dynamic material instance applied to the cube at BeginPlay.
	 *  Not editable: it is a transient runtime object that only exists while the game runs.
	 *  Kept as a UPROPERTY so the garbage collector does not destroy it while the mesh uses it.
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* dmiMat;

	/**
	 *  Called when something overlaps the box component.
	 *  Must be a UFUNCTION: dynamic multicast delegates (AddDynamic) look the function up by
	 *  name through reflection, so a plain C++ member function could not be bound.
	 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
