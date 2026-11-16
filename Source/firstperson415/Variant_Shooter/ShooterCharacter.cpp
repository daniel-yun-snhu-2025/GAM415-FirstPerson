// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);

	// GAM 495 refinement: the hero's primary spell uses the GAM 415 projectile (Projectile_Color
	// material, Splat_Mat decal, Splat_P Niagara burst) fired by the template rifle weapon.
	StartingSpellWeaponClass = TSoftClassPtr<AShooterWeapon>(FSoftObjectPath(
		TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Rifle.BP_ShooterWeapon_Rifle_C")));
}

void AShooterCharacter::GrantStartingSpell()
{
	if (IsDead() || StartingSpellWeaponClass.IsNull())
	{
		return;
	}
	if (UClass* SpellClass = StartingSpellWeaponClass.LoadSynchronous())
	{
		AddWeaponClass(SpellClass);
		UE_LOG(LogTemp, Log, TEXT("HeroSpell: equipped primary spell %s"), *SpellClass->GetName());
	}
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// update the HUD and reset the hero ability for this life.
	OnDamaged.Broadcast(1.0f);
	ResetPortalAbility();

	// The level's first pawn is possessed before BeginPlay (HP is still zero at that point), so the
	// starting spell is granted here as well as on respawn possession. AddWeaponClass ignores duplicates.
	if (IsPlayerControlled())
	{
		GrantStartingSpell();
	}
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear lifecycle timers
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
	GetWorld()->GetTimerManager().ClearTimer(PortalCooldownTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);

		// Portal mobility. The null check lets template-derived Blueprints opt in by assigning
		// an input action without breaking existing levels.
		if (PortalAbilityAction)
		{
			EnhancedInputComponent->BindAction(PortalAbilityAction, ETriggerEvent::Started, this, &AShooterCharacter::ActivatePortalAbilityInput);
		}
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

#if !UE_BUILD_SHIPPING
	// Development-only capture aid: launching with -GAM495DemoInvulnerable keeps the scripted
	// demonstration camera alive while it records. Never compiled into shipping builds.
	if (FParse::Param(FCommandLine::Get(), TEXT("GAM495DemoInvulnerable")))
	{
		UE_LOG(LogTemp, Verbose, TEXT("Player: ignored %.0f damage (-GAM495DemoInvulnerable)"), Damage);
		return 0.0f;
	}
#endif

	// Reduce HP
	CurrentHP -= Damage;
	UE_LOG(LogTemp, Log, TEXT("Player: took %.0f damage from %s (HP %.0f)"), Damage,
		DamageCauser ? *DamageCauser->GetName() : TEXT("none"), CurrentHP);

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void AShooterCharacter::DoStartFiring()
{
	// fire the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// stop firing the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1 && !IsDead())
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon(PlayerTag);
	}
}

void AShooterCharacter::ActivatePortalAbilityInput()
{
	TryActivatePortalAbility();
}

bool AShooterCharacter::TryActivatePortalAbility()
{
	if (IsDead() || !bPortalAbilityReady)
	{
		UE_LOG(LogTemp, Log, TEXT("PortalAbility: rejected (%s), cooldown remaining %.1fs"),
			IsDead() ? TEXT("dead") : TEXT("cooling down"), GetPortalCooldownRemaining());
		bLastPortalActivationValid = false;
		BroadcastPortalAbilityState();
		return false;
	}

	const FVector Start = GetActorLocation();
	const FVector Forward = GetControlRotation().Vector();
	const FVector DesiredEnd = Start + Forward * PortalAbilityDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PortalAbility), false, this);
	FHitResult Hit;
	const bool bBlocked = GetWorld()->SweepSingleByChannel(Hit, Start, DesiredEnd, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(),
			GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), QueryParams);
	const FVector HorizontalDestination = bBlocked ? Hit.Location - Forward * 10.0f : DesiredEnd;

	// Require meaningful clear travel and walkable ground. Without this probe, looking slightly
	// upward could place the capsule beyond the level and let gravity drop the player forever.
	FHitResult GroundHit;
	const FVector GroundProbeStart = HorizontalDestination + FVector::UpVector * 100.0f;
	const FVector GroundProbeEnd = GroundProbeStart - FVector::UpVector * PortalGroundProbeDistance;
	const bool bHasGround = GetWorld()->LineTraceSingleByChannel(GroundHit, GroundProbeStart,
		GroundProbeEnd, ECC_Visibility, QueryParams);
	const bool bWalkableGround = bHasGround && GroundHit.ImpactNormal.Z >= GetCharacterMovement()->GetWalkableFloorZ();
	if (FVector::DistSquared(Start, HorizontalDestination) < FMath::Square(150.0f) || !bWalkableGround)
	{
		UE_LOG(LogTemp, Log, TEXT("PortalAbility: blocked (%s); cooldown not consumed"),
			bWalkableGround ? TEXT("not enough clear distance") : TEXT("no walkable ground at destination"));
		bLastPortalActivationValid = false;
		BroadcastPortalAbilityState();
		return false;
	}

	const FVector Destination = GroundHit.Location + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	UE_LOG(LogTemp, Log, TEXT("PortalAbility: blink %.0f cm to walkable ground, cooldown %.1fs started"),
		FVector::Dist(Start, Destination), PortalAbilityCooldown);
	SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
	bPortalAbilityReady = false;
	bLastPortalActivationValid = true;
	PortalCooldownEndTime = GetWorld()->GetTimeSeconds() + PortalAbilityCooldown;
	GetWorld()->GetTimerManager().SetTimer(PortalCooldownTimer, this,
		&AShooterCharacter::CompletePortalCooldown, PortalAbilityCooldown, false);
	BroadcastPortalAbilityState();
	return true;
}

void AShooterCharacter::ResetPortalAbility()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PortalCooldownTimer);
	}
	bPortalAbilityReady = true;
	bLastPortalActivationValid = true;
	PortalCooldownEndTime = 0.0f;
	BroadcastPortalAbilityState();
}

float AShooterCharacter::GetPortalCooldownRemaining() const
{
	return (!bPortalAbilityReady && GetWorld())
		? FMath::Max(0.0f, PortalCooldownEndTime - GetWorld()->GetTimeSeconds())
		: 0.0f;
}

void AShooterCharacter::CompletePortalCooldown()
{
	UE_LOG(LogTemp, Log, TEXT("PortalAbility: cooldown complete, ready"));
	bPortalAbilityReady = true;
	bLastPortalActivationValid = true;
	PortalCooldownEndTime = 0.0f;
	BroadcastPortalAbilityState();
}

void AShooterCharacter::BroadcastPortalAbilityState()
{
	OnPortalAbilityUpdated.Broadcast(bPortalAbilityReady, GetPortalCooldownRemaining(), bLastPortalActivationValid);
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	// stub
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// GAM 415 Stepping Stone Three: this is the player's raytrace that drives terrain digging.
	// A line trace (ray cast) runs from the first person camera along the view direction.
	// It is a physics-scene query against collision shapes, not a render operation: the
	// procedural terrain answers it because CreateMeshSection was called with collision
	// enabled, which builds a triangle mesh collider from the same vertices the GPU draws.
	// The weapon aims the projectile at the returned impact point, and when the projectile
	// hits APerlinProcTerrain its NotifyHit calls AlterMesh with that point, so the hole
	// appears exactly where the player's crosshair ray met the surface.

	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon(PlayerTag);
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}

	// grant the death tag to the character
	Tags.Add(DeathTag);
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// disable controls
	DisableInput(nullptr);

	// reset HUD state and stop the ability cooldown for this life.
	OnBulletCountUpdated.Broadcast(0, 0);
	GetWorld()->GetTimerManager().ClearTimer(PortalCooldownTimer);
	bPortalAbilityReady = false;
	bLastPortalActivationValid = false;
	PortalCooldownEndTime = 0.0f;
	BroadcastPortalAbilityState();

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return CurrentHP <= 0.0f;
}

void AShooterCharacter::SetTeam(uint8 Team)
{
	TeamByte = Team;
}
