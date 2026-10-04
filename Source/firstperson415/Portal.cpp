// GAM 415 - Module 5: Portal
// See Portal.h for the graphics background (scene capture, render targets, screen aligned UVs).

#include "Portal.h"
#include "firstperson415Character.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "GameFramework/Controller.h"

APortal::APortal()
{
	// The capture camera has to follow the player camera every frame.
	PrimaryActorTick.bCanEverTick = true;

	boxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Comp"));
	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	sceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	rootArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Root Arrow"));

	// The trigger box is the root; the surface hangs off it, and the capture camera hangs off
	// the surface. The capture camera is moved in world space every frame anyway, so its
	// attachment only matters for where it starts.
	RootComponent = boxComp;
	mesh->SetupAttachment(boxComp);
	sceneCapture->SetupAttachment(mesh);
	rootArrow->SetupAttachment(RootComponent);

	// The player walks into the portal, so the surface must not block them.
	mesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	// Do not draw this portal's own surface into its capture. Otherwise the capture camera,
	// which sits just in front of this portal, would mostly see the back of the portal mesh.
	mesh->SetHiddenInSceneCapture(true);

	// The surface is not a real object, so it should not throw a shadow onto the floor.
	// The capture would also pick up that shadow even though the mesh itself is hidden from it.
	mesh->SetCastShadow(false);
}

void APortal::BeginPlay()
{
	Super::BeginPlay();

	// Overlap is a physics/gameplay event; it fires on the game thread when a body enters the box.
	boxComp->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);

	// Apply the surface material (it samples the OTHER portal's render target).
	if (mat)
	{
		mesh->SetMaterial(0, mat);
	}

	// Point the capture camera at this portal's render target. From now on the renderer draws
	// an extra view of the scene from sceneCapture into this texture every frame. The tutorial
	// does this in the Blueprint graph; assigning it here keeps all the setup in one place.
	if (renderTarget)
	{
		sceneCapture->TextureTarget = renderTarget;
	}
}

void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePortals();
}

void APortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only players teleport. The cast returns null for projectiles, bots without the character
	// base class, props, etc.
	Afirstperson415Character* playerChar = Cast<Afirstperson415Character>(OtherActor);

	if (playerChar)
	{
		if (OtherPortal)
		{
			if (!playerChar->isTeleporting)
			{
				// Set the flag BEFORE moving the player. The move puts them inside the other
				// portal's box, whose overlap fires immediately; with the flag already set it
				// does nothing, instead of sending the player straight back (an endless loop).
				playerChar->isTeleporting = true;

				// Arrive at the other portal's arrow component rather than its pivot, so the
				// spawn point can be placed in front of the portal in the Blueprint.
				FVector loc = OtherPortal->rootArrow->GetComponentLocation();
				playerChar->SetActorLocation(loc);

				// Stepping Stone Four: face the player along the other portal's forward direction.
				// The arrow's X axis is the portal's forward vector. Only its yaw is used so the
				// player arrives looking level instead of inheriting any pitch or roll.
				// A first person character's view comes from its controller's control rotation,
				// not the actor rotation (the actor copies the controller yaw every frame), so the
				// control rotation is what has to change for the camera to turn.
				FRotator arriveRot(0.0f, OtherPortal->rootArrow->GetComponentRotation().Yaw, 0.0f);
				playerChar->SetActorRotation(arriveRot);
				if (AController* playerController = playerChar->GetController())
				{
					playerController->SetControlRotation(arriveRot);
				}

				// Clear the flag after one second. SetTimer by function name cannot pass
				// arguments, so a timer delegate binds SetBool together with the player pointer.
				FTimerHandle TimerHandle;
				FTimerDelegate TimerDelegate;
				TimerDelegate.BindUFunction(this, FName("SetBool"), playerChar);
				// 1 second, false = do not loop.
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 1.0f, false);
			}
		}
	}
}

void APortal::SetBool(Afirstperson415Character* playerChar)
{
	if (playerChar)
	{
		playerChar->isTeleporting = false;
	}
}

void APortal::UpdatePortals()
{
	if (!OtherPortal)
	{
		return;
	}

	APlayerCameraManager* camManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (!camManager || !camManager->GetTransformComponent())
	{
		return;
	}

	// Offset from the other portal to this one.
	FVector Location = this->GetActorLocation() - OtherPortal->GetActorLocation();

	// Player camera position and rotation. Index 0 = the first local player (single player).
	FVector camLocation = camManager->GetTransformComponent()->GetComponentLocation();
	FRotator camRotation = camManager->GetTransformComponent()->GetComponentRotation();

	// Put the capture camera where the player camera would be if it stood the same distance
	// from THIS portal as it currently stands from the other one. This portal's render target
	// is shown on the other portal, so when the player looks at the other portal they see the
	// view around this one from the matching position. Copying the rotation is what lets the
	// material use screen aligned UVs: capture and screen share the same view direction.
	// Note: this is a pure translation, so it assumes both portals face the same direction.
	FVector CombinedLocation = camLocation + Location;

	sceneCapture->SetWorldLocationAndRotation(CombinedLocation, camRotation);
}
