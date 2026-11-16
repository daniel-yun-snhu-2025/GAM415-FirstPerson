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
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

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

				// Preserve the player's pose and momentum relative to the entrance, even when
				// the two portals have different rotations. The destination arrow remains the
				// designer-controlled anchor, while the capsule sweep below keeps the player out
				// of nearby walls and props.
				const FTransform ArrivalTransform = TransformThroughPortal(playerChar->GetActorTransform());
				const FVector ArrivalLocation = FindSafeArrivalLocation(playerChar);
				const FRotator ArrivalRotation = ArrivalTransform.GetRotation().Rotator();
				playerChar->SetActorLocationAndRotation(ArrivalLocation, ArrivalRotation, false, nullptr, ETeleportType::TeleportPhysics);

				if (UCharacterMovementComponent* Movement = playerChar->GetCharacterMovement())
				{
					const FVector EntryVelocity = Movement->Velocity;
					Movement->Velocity = TransformDirectionThroughPortal(EntryVelocity);
					UE_LOG(LogTemp, Log, TEXT("Portal: %s -> %s, portal yaw %.0f -> %.0f, velocity %s -> %s"),
						*GetName(), *OtherPortal->GetName(), GetActorRotation().Yaw,
						OtherPortal->rootArrow->GetComponentRotation().Yaw,
						*EntryVelocity.ToCompactString(), *Movement->Velocity.ToCompactString());
				}

				// A first-person camera follows the controller rather than the pawn rotation, so
				// transform the control rotation through the same portal pair.
				if (AController* PlayerController = playerChar->GetController())
				{
					FTransform ViewTransform(FQuat(PlayerController->GetControlRotation()), playerChar->GetActorLocation());
					PlayerController->SetControlRotation(TransformThroughPortal(ViewTransform).GetRotation().Rotator());
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

FTransform APortal::TransformThroughPortal(const FTransform& WorldTransform) const
{
	if (!OtherPortal)
	{
		return WorldTransform;
	}

	const FTransform SourceTransform = GetActorTransform();
	const FTransform DestinationTransform = OtherPortal->rootArrow->GetComponentTransform();
	const FTransform RelativeTransform = WorldTransform.GetRelativeTransform(SourceTransform);
	return RelativeTransform * DestinationTransform;
}

FVector APortal::TransformDirectionThroughPortal(const FVector& WorldDirection) const
{
	if (!OtherPortal)
	{
		return WorldDirection;
	}

	const FVector LocalDirection = GetActorTransform().InverseTransformVectorNoScale(WorldDirection);
	return OtherPortal->rootArrow->GetComponentTransform().TransformVectorNoScale(LocalDirection);
}

FVector APortal::FindSafeArrivalLocation(const Afirstperson415Character* PlayerCharacter) const
{
	const FVector DesiredLocation = OtherPortal->rootArrow->GetComponentLocation();
	if (!PlayerCharacter || !GetWorld())
	{
		return DesiredLocation;
	}

	const UCapsuleComponent* Capsule = PlayerCharacter->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Forward = OtherPortal->rootArrow->GetForwardVector();
	const FVector TraceStart = DesiredLocation - Forward * Radius;
	const FVector TraceEnd = DesiredLocation + Forward * (Radius * 3.0f);

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PortalArrivalSweep), false, PlayerCharacter);
	if (GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), QueryParams))
	{
		return Hit.Location - Forward * 2.0f;
	}

	return TraceEnd;
}

void APortal::UpdatePortals()
{
	if (!OtherPortal)
	{
		return;
	}

	APlayerCameraManager* CamManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (!CamManager || !CamManager->GetTransformComponent())
	{
		return;
	}

	// The render-target view now uses the same full relative position and orientation mapping
	// as teleportation, so differently oriented portal pairs remain visually coherent.
	const FTransform CameraTransform = CamManager->GetTransformComponent()->GetComponentTransform();
	const FTransform CaptureTransform = OtherPortal->TransformThroughPortal(CameraTransform);
	sceneCapture->SetWorldLocationAndRotation(CaptureTransform.GetLocation(), CaptureTransform.GetRotation().Rotator());
}
