// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ShooterCharacter.h"
#include "ShooterBulletCounterUI.h"
#include "firstperson415.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "ShooterNativeHUD.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"
#include "ShooterGameMode.h"
#include "EngineUtils.h"
#include "ShooterNPC.h"
#include "Portal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	CreateNativeHUD();

	// GAM 495 refinement: show the Arcane Trial objective, driven by enemy NPC deaths.
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->OnArcaneTrialUpdated.AddDynamic(this, &AShooterPlayerController::OnArcaneTrialUpdated);
		OnArcaneTrialUpdated(GM->GetArcaneTrialProgress(), GM->GetArcaneTrialGoal(),
			GM->GetArcaneTrialProgress() >= GM->GetArcaneTrialGoal());
	}
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Q is a native fallback so the ability works even before a Blueprint input asset is assigned.
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AShooterPlayerController::ActivatePortalAbilityFallback);

#if !UE_BUILD_SHIPPING
	// Development-only helpers used to record a repeatable demonstration video. They only rotate
	// the camera; they do not move the player, damage enemies, or change any gameplay rules.
	InputComponent->BindKey(EKeys::F8, IE_Pressed, this, &AShooterPlayerController::DebugFaceNearestEnemy);
	InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AShooterPlayerController::DebugFaceNearestPortal);
	InputComponent->BindKey(FKey(TEXT("F10")), IE_Pressed, this, &AShooterPlayerController::DebugLogPortals);
	// Use P for the portal-entry capture helper; F11 is not consistently delivered by the game window.
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AShooterPlayerController::DebugEnterNearestPortal);
#endif

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		if (ShouldUseTouchControls())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				// add the controls to the player screen
				MobileControlsWidget->AddToPlayerScreen(0);

			} else {

				UE_LOG(Logfirstperson415, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}

		// create the bullet counter widget and add it to the screen
		BulletCounterUI = CreateWidget<UShooterBulletCounterUI>(this, BulletCounterUIClass);

		if (BulletCounterUI)
		{
			BulletCounterUI->AddToPlayerScreen(0);

		} else {

			UE_LOG(Logfirstperson415, Error, TEXT("Could not spawn bullet counter widget."));

		}
	}
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &AShooterPlayerController::OnPawnDestroyed);

	// is this a shooter character?
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn))
	{
		// add the player tag
		ShooterCharacter->Tags.Add(PlayerPawnTag);

		// set the team
		ShooterCharacter->SetTeam(TeamByte);

		// subscribe to the pawn's delegates
		ShooterCharacter->OnBulletCountUpdated.AddDynamic(this, &AShooterPlayerController::OnBulletCountUpdated);
		ShooterCharacter->OnDamaged.AddDynamic(this, &AShooterPlayerController::OnPawnDamaged);
		ShooterCharacter->OnPortalAbilityUpdated.AddDynamic(this, &AShooterPlayerController::OnPortalAbilityUpdated);

		// force update the life bar and ability HUD
		ShooterCharacter->OnDamaged.Broadcast(1.0f);
		ShooterCharacter->ResetPortalAbility();

		// GAM 495 refinement: the hero spawns (and respawns) with the primary spell equipped.
		ShooterCharacter->GrantStartingSpell();
	}
}

void AShooterPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// reset the bullet counter HUD
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->BP_UpdateBulletCounter(0, 0);
	}

	if (TeamByte < TeamTags.Num())
	{
		// find the player start
		TArray<AActor*> ActorList;
		UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), TeamTags[TeamByte], ActorList);

		if (ActorList.Num() > 0)
		{
			// select a random player start
			AActor* RandomPlayerStart = ActorList[FMath::RandRange(0, ActorList.Num() - 1)];

			// spawn a character at the player start
			const FTransform SpawnTransform = RandomPlayerStart->GetActorTransform();

			if (AShooterCharacter* RespawnedCharacter = GetWorld()->SpawnActor<AShooterCharacter>(CharacterClass, SpawnTransform))
			{
				// possess the character
				Possess(RespawnedCharacter);
			}
		}
	}
}

void AShooterPlayerController::OnBulletCountUpdated(int32 MagazineSize, int32 Bullets)
{
	// update the UI
	if (BulletCounterUI)
	{
		BulletCounterUI->BP_UpdateBulletCounter(MagazineSize, Bullets);
	}
}

void AShooterPlayerController::OnPawnDamaged(float LifePercent)
{
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->BP_Damaged(LifePercent);
	}
}

void AShooterPlayerController::OnPortalAbilityUpdated(bool bReady, float CooldownRemaining, bool bLastActivationValid)
{
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->BP_UpdatePortalAbility(bReady, CooldownRemaining, bLastActivationValid);
	}

	if (PortalAbilityHUD)
	{
		PortalAbilityHUD->SetPortalState(bReady, CooldownRemaining, bLastActivationValid);
	}

	if (!bReady)
	{
		GetWorld()->GetTimerManager().SetTimer(PortalHUDTimer, this,
			&AShooterPlayerController::RefreshPortalHUD, 0.1f, false);
	}
}

void AShooterPlayerController::ActivatePortalAbilityFallback()
{
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn()))
	{
		// TryActivatePortalAbility broadcasts the exact success/blocked/cooldown state. Calling
		// RefreshPortalHUD immediately here would overwrite a blocked message with READY.
		ShooterCharacter->TryActivatePortalAbility();
	}
}

void AShooterPlayerController::OnArcaneTrialUpdated(int32 EnemiesDefeated, int32 Goal, bool bComplete)
{
	if (PortalAbilityHUD)
	{
		PortalAbilityHUD->SetObjectiveProgress(EnemiesDefeated, Goal, bComplete);
	}
}

#if !UE_BUILD_SHIPPING
void AShooterPlayerController::DebugFaceNearestEnemy()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}
	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DebugFaceEnemy), false, MyPawn);
	for (TActorIterator<AShooterNPC> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(FName("Dead")))
		{
			continue;
		}
		// Prefer enemies in direct line of sight; occluded ones are only a fallback.
		FHitResult Hit;
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, It->GetActorLocation(), ECC_Visibility, Params)
			&& Hit.GetActor() != *It;
		const float Score = FVector::DistSquared(It->GetActorLocation(), MyPawn->GetActorLocation()) + (bBlocked ? 1.0e12f : 0.0f);
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = *It;
		}
	}
	if (Best)
	{
		// Aim slightly below the capsule center so the projectile meets the torso.
		DebugFaceLocation(Best->GetActorLocation() - FVector(0.0f, 0.0f, 20.0f));
	}
}

void AShooterPlayerController::DebugFaceNearestPortal()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}
	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<APortal> It(GetWorld()); It; ++It)
	{
		const float DistSq = FVector::DistSquared(It->GetActorLocation(), MyPawn->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}
	if (Best)
	{
		DebugFaceLocation(Best->GetActorLocation());
	}
}

void AShooterPlayerController::DebugLogPortals()
{
	for (TActorIterator<APortal> It(GetWorld()); It; ++It)
	{
		UE_LOG(LogTemp, Log, TEXT("PortalPair: %s at %s yaw %.0f -> %s arrow yaw %.0f"), *It->GetName(),
			*It->GetActorLocation().ToCompactString(), It->GetActorRotation().Yaw,
			It->OtherPortal ? *It->OtherPortal->GetName() : TEXT("none"),
			It->OtherPortal ? It->OtherPortal->rootArrow->GetComponentRotation().Yaw : 0.0f);
	}
}

void AShooterPlayerController::DebugEnterNearestPortal()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}
	APortal* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<APortal> It(GetWorld()); It; ++It)
	{
		const float DistSq = FVector::DistSquared(It->GetActorLocation(), MyPawn->GetActorLocation());
		if (It->OtherPortal && DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}
	if (Best)
	{
		// The evidence-only launch flag rotates the destination arrow by 90 degrees so this level,
		// whose two placed portals both have yaw 0, also exercises the full relative transform.
		if (FParse::Param(FCommandLine::Get(), TEXT("GAM495RotatePortalDemo")) && Best->OtherPortal)
		{
			FRotator Rotated = Best->OtherPortal->rootArrow->GetComponentRotation();
			Rotated.Yaw = FMath::UnwindDegrees(Rotated.Yaw + 90.0f);
			Best->OtherPortal->rootArrow->SetWorldRotation(Rotated);
			UE_LOG(LogTemp, Log, TEXT("PortalDemo: rotated destination arrow to yaw %.0f"), Rotated.Yaw);
		}

		// Start just outside the trigger and walk into it. The overlap, relative transform,
		// momentum mapping, destination sweep, and anti-loop flag are all exercised normally.
		const FVector Forward = Best->GetActorForwardVector();
		const float TriggerDepth = Best->boxComp ? Best->boxComp->GetScaledBoxExtent().X : 100.0f;
		const FVector Start = Best->GetActorLocation() - Forward * (TriggerDepth + 160.0f);
		MyPawn->SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
		DebugFaceLocation(Best->GetActorLocation());
		if (AShooterCharacter* Shooter = Cast<AShooterCharacter>(MyPawn))
		{
			Shooter->isTeleporting = false;
			Shooter->GetCharacterMovement()->Velocity = Forward * 1200.0f;
		}
		UE_LOG(LogTemp, Log, TEXT("PortalDemo: entering %s toward %s"), *Best->GetName(), *Best->OtherPortal->GetName());
	}
}

void AShooterPlayerController::DebugFaceLocation(const FVector& TargetLocation)
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	SetControlRotation((TargetLocation - ViewLocation).Rotation());
}
#endif

void AShooterPlayerController::ShowHitFeedback()
{
	if (PortalAbilityHUD)
	{
		PortalAbilityHUD->ShowHitFeedback();
	}
}

void AShooterPlayerController::CreateNativeHUD()
{
	PortalAbilityHUD = CreateWidget<UShooterNativeHUD>(this, UShooterNativeHUD::StaticClass());
	if (PortalAbilityHUD)
	{
		PortalAbilityHUD->AddToPlayerScreen(5);
	}
}

void AShooterPlayerController::RefreshPortalHUD()
{
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn()))
	{
		const float Remaining = ShooterCharacter->GetPortalCooldownRemaining();
		if (PortalAbilityHUD)
		{
			PortalAbilityHUD->SetPortalState(Remaining <= 0.0f, Remaining, true);
		}
		if (Remaining > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(PortalHUDTimer, this,
				&AShooterPlayerController::RefreshPortalHUD, 0.1f, false);
		}
	}
}

bool AShooterPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AShooterPlayerController::SetTeam(uint8 Team)
{
	TeamByte = Team;

	// if we already have a pawn, set its team
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn()))
	{
		ShooterCharacter->SetTeam(Team);
	}
}
