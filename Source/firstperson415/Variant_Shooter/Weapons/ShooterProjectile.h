// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class ACharacter;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 *  Simple projectile class for a first person shooter game
 */
UCLASS(abstract)
class FIRSTPERSON415_API AShooterProjectile : public AActor
{
	GENERATED_BODY()
	
	/** Provides collision detection for the projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionComponent;

	/** Handles movement for the projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

public:

	// ---- GAM 415 Stepping Stone One: colored projectile mesh + matching splat decal ----
	//
	// Rendering summary:
	//  * ballMesh is a normal static mesh draw. Its material (Projectile_Color) exposes one
	//    vector parameter, "ProjColor", wired to Base Color.
	//  * At BeginPlay we create a Dynamic Material Instance (DMI) of that material for THIS
	//    projectile only and write randColor into "ProjColor". A DMI shares the compiled
	//    shaders of its parent and only owns a private parameter block, so hundreds of
	//    projectiles with different colors cost no extra shader compiles.
	//  * On hit we spawn a deferred decal (Splat_Mat). A decal is a box volume that projects
	//    its material onto whatever geometry is inside it, using the scene depth buffer to
	//    find the surface, and writes into the deferred shading buffers of that surface.
	//    Its DMI gets the same randColor plus a random SubUV "Frame", so mesh and splat match.

	/**
	 *  Visible projectile mesh (set to FirstPersonProjectileMesh, scale 0.125, in the Blueprint).
	 *  Created in C++ rather than in the Blueprint so this class holds a direct reference and
	 *  can swap its material at runtime.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|GAM415")
	UStaticMeshComponent* ballMesh;

	/**
	 *  Decal material spawned on hit (Splat_Mat). Material Domain = Deferred Decal,
	 *  Blend Mode = Translucent, parameters: vector "Color", scalar "Frame".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|GAM415")
	UMaterialInterface* baseMat;

	/** Material used for the projectile mesh (Projectile_Color, vector parameter "ProjColor"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|GAM415")
	UMaterialInterface* projMat;

	/**
	 *  Dynamic instance of projMat applied to ballMesh element 0.
	 *  Runtime only; a UPROPERTY so the garbage collector keeps it alive with the projectile.
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* dmiMat;

	/**
	 *  Random color chosen once at BeginPlay and shared by the mesh and the decal.
	 *  Stored as a member (not a local) because BeginPlay and NotifyHit run at different times.
	 *  FLinearColor is linear-space RGBA floats, the format material vector parameters expect.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Projectile|GAM415")
	FLinearColor randColor;

protected:

	/** Loudness of the AI perception noise done by this projectile on hit */
	UPROPERTY(EditAnywhere, Category="Projectile|Noise", meta = (ClampMin = 0, ClampMax = 100))
	float NoiseLoudness = 3.0f;

	/** Range of the AI perception noise done by this projectile on hit */
	UPROPERTY(EditAnywhere, Category="Projectile|Noise", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float NoiseRange = 1000.0f;

	/** Tag of the AI perception noise done by this projectile on hit */
	UPROPERTY(EditAnywhere, Category="Noise")
	FName NoiseTag = FName("Projectile");

	/** Physics force to apply on hit */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit", meta = (ClampMin = 0, ClampMax = 50000))
	float PhysicsForce = 100.0f;

	/** Damage to apply on hit */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit", meta = (ClampMin = 0, ClampMax = 100))
	float HitDamage = 25.0f;

	/** Type of damage to apply. Can be used to represent specific types of damage such as fire, explosion, etc. */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit")
	TSubclassOf<UDamageType> HitDamageType;

	/** If true, the projectile can damage the character that shot it */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit")
	bool bDamageOwner = false;

	/** If true, the projectile will explode and apply radial damage to all actors in range */
	UPROPERTY(EditAnywhere, Category="Projectile|Explosion")
	bool bExplodeOnHit = false;

	/** Max distance for actors to be affected by explosion damage */
	UPROPERTY(EditAnywhere, Category="Projectile|Explosion", meta = (ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float ExplosionRadius = 500.0f;	

	/** If true, this projectile has already hit another surface */
	bool bHit = false;

	/** How long to wait after a hit before destroying this projectile */
	UPROPERTY(EditAnywhere, Category="Projectile|Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float DeferredDestructionTime = 5.0f;

	/** Timer to handle deferred destruction of this projectile */
	FTimerHandle DestructionTimer;

public:	

	/** Constructor */
	AShooterProjectile();

protected:
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Handles collision */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

protected:

	/** Looks up actors within the explosion radius and damages them */
	void ExplosionCheck(const FVector& ExplosionCenter);

	/** Processes a projectile hit for the given actor */
	void ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection);

	/** Passes control to Blueprint to implement any effects on hit. */
	UFUNCTION(BlueprintImplementableEvent, Category="Projectile", meta = (DisplayName = "On Projectile Hit"))
	void BP_OnProjectileHit(const FHitResult& Hit);

	/** Called from the destruction timer to destroy this projectile */
	void OnDeferredDestruction();

public:

	/** Sets the noise tag to use when generating AI perception noise on impact */
	void SetNoiseTag(const FName& Tag);

};
