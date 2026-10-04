// GAM 415 - Module 5: Portal built from a Scene Capture Component and a Render Target
//
// Graphics background for this class:
//  - A USceneCaptureComponent2D is a second camera. Every frame it renders the scene from its
//    own position into a UTextureRenderTarget2D instead of to the screen. That is a full extra
//    scene render (an extra view through the renderer), so each portal roughly costs as much as
//    drawing the level again at the render target's resolution.
//  - A render target is a GPU texture that can be written by the renderer and then sampled by
//    a material like any other texture, in the same frame.
//  - Each portal's mesh uses a material that samples the OTHER portal's render target with
//    ScreenAlignedUVs: the texture is looked up by the pixel's position on the screen instead
//    of the mesh UVs. Because the capture camera copies the player camera's rotation, the
//    captured image lines up with the screen, so the portal surface looks like a window.
//  - The material is Unlit with the render target in Emissive. The captured image already
//    contains the other scene's lighting, so lighting it again would darken it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"               // trigger volume that starts the teleport
#include "Components/SceneCaptureComponent2D.h"    // the capture camera
#include "Components/ArrowComponent.h"             // spawn point the player is moved to
#include "Portal.generated.h"

class Afirstperson415Character;      // forward declaration: only used as a pointer parameter
class UTextureRenderTarget2D;
class UMaterialInterface;

/**
 *  One side of a portal pair. Two instances are placed in the level and point at each other
 *  through OtherPortal.
 *
 *  Setup for a pair (Portal_A, Portal_B):
 *    Portal_A: renderTarget = RT_PortalA, mat = M_PortalB (samples RT_PortalB), OtherPortal = Portal_B
 *    Portal_B: renderTarget = RT_PortalB, mat = M_PortalA (samples RT_PortalA), OtherPortal = Portal_A
 *  Portal_A's capture camera sits near Portal_A and draws into RT_PortalA, and that image is
 *  shown on Portal_B's surface, so looking into B shows what is around A.
 */
UCLASS()
class FIRSTPERSON415_API APortal : public AActor
{
	GENERATED_BODY()

public:
	APortal();

protected:
	virtual void BeginPlay() override;

public:
	/** Moves the capture camera every frame so it follows the player camera. */
	virtual void Tick(float DeltaTime) override;

	/**
	 *  The visible portal surface: a thin box (a flat plane with a one sided material would
	 *  only be visible from the front; a two sided material renders the back face mirrored).
	 *  Collision is off so the player can walk into the trigger box behind it.
	 */
	UPROPERTY(EditAnywhere, Category = "Portal")
	UStaticMeshComponent* mesh;

	/** Capture camera. Renders into renderTarget every frame. */
	UPROPERTY(EditAnywhere, Category = "Portal")
	USceneCaptureComponent2D* sceneCapture;

	/**
	 *  Texture this portal's capture camera draws into. The OTHER portal's material samples it.
	 *  RT1/RT2 are 1920x1080 (a new render target defaults to 256x256). The resolution is the
	 *  size of the GPU texture the extra scene render fills, so it sets how sharp the portal
	 *  looks up close and also how much GPU memory and fill cost each portal adds.
	 */
	UPROPERTY(EditAnywhere, Category = "Portal")
	UTextureRenderTarget2D* renderTarget;

	/** Trigger volume, used as the root. Overlapping it teleports the player. */
	UPROPERTY(EditAnywhere, Category = "Portal")
	UBoxComponent* boxComp;

	/** The portal this one leads to. Picked in the level with the eyedropper. */
	UPROPERTY(EditAnywhere, Category = "Portal")
	APortal* OtherPortal;

	/** Material for the surface: samples the OTHER portal's render target. */
	UPROPERTY(EditAnywhere, Category = "Portal")
	UMaterialInterface* mat;

	/**
	 *  Where a player arriving through the other portal is placed, and which way they face:
	 *  the arrow's forward (X) direction becomes the player's view direction. Movable and
	 *  rotatable in the Blueprint.
	 */
	UPROPERTY(EditAnywhere, Category = "Portal")
	UArrowComponent* rootArrow;

	/** Overlap on boxComp. Must be a UFUNCTION so AddDynamic can bind it through reflection. */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/**
	 *  Clears isTeleporting on the player one second after a teleport.
	 *  A UFUNCTION so a timer delegate can call it by name and pass the player as an argument.
	 */
	UFUNCTION()
	void SetBool(Afirstperson415Character* playerChar);

	/** Places the capture camera at the player camera's position, offset between the two portals. */
	void UpdatePortals();
};
