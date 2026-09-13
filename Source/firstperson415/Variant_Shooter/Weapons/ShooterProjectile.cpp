// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetMathLibrary.h"

AShooterProjectile::AShooterProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// create the collision component and assign it as the root
	RootComponent = CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));

	CollisionComponent->SetSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	// GAM 415: create the visible ball mesh and attach it to the collision component.
	// The sphere component stays the root (it drives physics and movement); the mesh only
	// renders. Attaching it means the mesh inherits the sphere's transform every frame, which
	// is the object-to-world matrix the vertex shader uses when this projectile is drawn.
	ballMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball Mesh"));
	ballMesh->SetupAttachment(CollisionComponent);
	// Purely visual: no collision so the mesh never interferes with the sphere's hit detection.
	ballMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// create the projectile movement component. No need to attach it because it's not a Scene Component
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));

	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bShouldBounce = true;

	// set the default damage type
	HitDamageType = UDamageType::StaticClass();
}

void AShooterProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// ignore the pawn that shot this projectile
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);

	// GAM 415: pick the color once, as soon as the projectile spawns, so the mesh is colored
	// from its first rendered frame and the decal can reuse the exact same value later.
	// Linear RGB in [0,1] plus alpha 1: this is uploaded to the GPU as a float4 constant.
	randColor = FLinearColor(
		UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f),
		UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f),
		UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f),
		1.0f);

	if (projMat && ballMesh)
	{
		// Create a per-projectile Dynamic Material Instance. No shader work happens here:
		// the instance points at Projectile_Color's compiled shaders and owns only its
		// own parameter values (its own uniform buffer contents).
		dmiMat = UMaterialInstanceDynamic::Create(projMat, this);

		// Element index 0 = the mesh's first material slot. Meshes with several material
		// sections would use 1, 2, ... for the other sections.
		ballMesh->SetMaterial(0, dmiMat);

		// Write the random color into the "ProjColor" vector parameter that feeds Base Color.
		// The value is copied to the render thread and lands in the constant buffer bound
		// when the ball's draw call executes.
		dmiMat->SetVectorParameterValue(TEXT("ProjColor"), randColor);
	}
}

void AShooterProjectile::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the destruction timer
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
}

void AShooterProjectile::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// ignore if we've already hit something else
	if (bHit)
	{
		return;
	}

	bHit = true;

	// disable collision on the projectile
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// make AI perception noise
	MakeNoise(NoiseLoudness, GetInstigator(), GetActorLocation(), NoiseRange, NoiseTag);

	if (bExplodeOnHit)
	{
		
		// apply explosion damage centered on the projectile
		ExplosionCheck(GetActorLocation());

	} else {

		// single hit projectile. Process the collided actor
		ProcessHit(Other, OtherComp, Hit.ImpactPoint, -Hit.ImpactNormal);

	}

	// GAM 415: spawn a paint splat decal on the surface we hit, matching the projectile color.
	// Only when we actually hit something and a decal material was assigned in the Blueprint.
	if (Other != nullptr && baseMat != nullptr)
	{
		// Splat_Mat samples a 2x2 texture atlas through the SubUV_Function node. "Frame" picks
		// which of the four splat images is shown: the function scales and offsets the UVs
		// into that cell before the texture is sampled, so one texture gives four looks.
		// Range 0-3 (the material floors it to a cell index).
		float frameNum = UKismetMathLibrary::RandomFloatInRange(0.0f, 3.0f);

		// Decal size is the half-extent box the decal projects through (X = projection depth,
		// Y/Z = footprint). Random so hits look less uniform.
		FVector decalSize = FVector(UKismetMathLibrary::RandomFloatInRange(20.0f, 40.0f));

		// SpawnDecalAtLocation creates a standalone decal component in the world.
		//  - Hit.Location: the impact point, center of the decal box.
		//  - Hit.Normal.Rotation(): a rotator whose X axis is the surface normal. Decals
		//    project along their local X axis, so this makes the splat lie flat on the surface
		//    and face the right way (a wall gets a vertical splat, the floor a horizontal one).
		//  - Life span 0 = never auto-destroyed.
		// Being a *deferred* decal, its material does not draw geometry; during the deferred
		// pass the GPU reconstructs the surface position from the depth buffer for every pixel
		// inside the box and blends the decal's Base Color/Opacity into that surface's shading.
		auto Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), baseMat, decalSize, Hit.Location, Hit.Normal.Rotation(), 0.0f);
		if (Decal)
		{
			// Give this decal its own Dynamic Material Instance (same idea as the ball mesh:
			// shared shaders, private parameters), then feed it the SAME randColor as the mesh.
			auto MatInstance = Decal->CreateDynamicMaterialInstance();
			if (MatInstance)
			{
				MatInstance->SetVectorParameterValue(TEXT("Color"), randColor);   // vector param -> Base Color
				MatInstance->SetScalarParameterValue(TEXT("Frame"), frameNum);    // scalar param -> SubUV frame
			}
		}
	}

	// pass control to BP for any extra effects
	BP_OnProjectileHit(Hit);

	// check if we should schedule deferred destruction of the projectile
	if (DeferredDestructionTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestructionTimer, this, &AShooterProjectile::OnDeferredDestruction, DeferredDestructionTime, false);

	} else {

		// destroy the projectile right away
		Destroy();
	}
}

void AShooterProjectile::ExplosionCheck(const FVector& ExplosionCenter)
{
	// do a sphere overlap check look for nearby actors to damage
	TArray<FOverlapResult> Overlaps;

	FCollisionShape OverlapShape;
	OverlapShape.SetSphere(ExplosionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (!bDamageOwner)
	{
		QueryParams.AddIgnoredActor(GetInstigator());
	}

	GetWorld()->OverlapMultiByObjectType(Overlaps, ExplosionCenter, FQuat::Identity, ObjectParams, OverlapShape, QueryParams);

	TArray<AActor*> DamagedActors;

	// process the overlap results
	for (const FOverlapResult& CurrentOverlap : Overlaps)
	{
		// overlaps may return the same actor multiple times per each component overlapped
		// ensure we only damage each actor once by adding it to a damaged list
		if (DamagedActors.Find(CurrentOverlap.GetActor()) == INDEX_NONE)
		{
			DamagedActors.Add(CurrentOverlap.GetActor());

			// apply physics force away from the explosion
			const FVector& ExplosionDir = CurrentOverlap.GetActor()->GetActorLocation() - GetActorLocation();

			// push and/or damage the overlapped actor
			ProcessHit(CurrentOverlap.GetActor(), CurrentOverlap.GetComponent(), GetActorLocation(), ExplosionDir.GetSafeNormal());
		}
			
	}
}

void AShooterProjectile::ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection)
{
	// have we hit a character?
	if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
	{
		// ignore the owner of this projectile
		if (HitCharacter != GetOwner() || bDamageOwner)
		{
			// apply damage to the character
			UGameplayStatics::ApplyDamage(HitCharacter, HitDamage, GetInstigator()->GetController(), this, HitDamageType);
		}
	}

	// have we hit a physics object?
	if (HitComp->IsSimulatingPhysics())
	{
		// give some physics impulse to the object
		HitComp->AddImpulseAtLocation(HitDirection * PhysicsForce, HitLocation);
	}
}

void AShooterProjectile::OnDeferredDestruction()
{
	// destroy this actor
	Destroy();
}

void AShooterProjectile::SetNoiseTag(const FName& Tag)
{
	NoiseTag = Tag;
}
