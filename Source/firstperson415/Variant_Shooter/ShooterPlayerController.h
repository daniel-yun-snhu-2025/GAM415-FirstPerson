// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UInputMappingContext;
class AShooterCharacter;
class UShooterBulletCounterUI;
class UShooterNativeHUD;

/**
 *  Simple PlayerController for a first person shooter game
 *  Manages input mappings
 *  Respawns the player pawn when it's destroyed
 */
UCLASS(abstract, config="Game")
class FIRSTPERSON415_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input mapping contexts for this player */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Character class to respawn when the possessed pawn is destroyed */
	UPROPERTY(EditAnywhere, Category="Shooter|Respawn")
	TSubclassOf<AShooterCharacter> CharacterClass;

	/** Type of bullet counter UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UShooterBulletCounterUI> BulletCounterUIClass;

	/** Tag to grant the possessed pawn to flag it as the player */
	UPROPERTY(EditAnywhere, Category="Shooter|Player")
	FName PlayerPawnTag = FName("Player");

	/** Pointer to the bullet counter UI widget */
	UPROPERTY()
	TObjectPtr<UShooterBulletCounterUI> BulletCounterUI;

	/** Native fallback HUD, created even when no Blueprint widget is assigned. */
	UPROPERTY()
	TObjectPtr<UShooterNativeHUD> PortalAbilityHUD;

	FTimerHandle PortalHUDTimer;

	/** Team ID for this player */
	uint8 TeamByte = 0;

	/** Tags that identify each team for PlayerStart selection upon respawning */
	UPROPERTY(EditAnywhere, Category="Shooter|Team")
	TArray<FName> TeamTags;

protected:

	/** Gameplay Initialization */
	virtual void BeginPlay() override;

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	/** Pawn initialization */
	virtual void OnPossess(APawn* InPawn) override;

	/** Called if the possessed pawn is destroyed */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** Called when the bullet count on the possessed pawn is updated */
	UFUNCTION()
	void OnBulletCountUpdated(int32 MagazineSize, int32 Bullets);

	/** Called when the possessed pawn is damaged */
	UFUNCTION()
	void OnPawnDamaged(float LifePercent);

	/** Called when the portal ability enters cooldown, becomes ready, or rejects an activation. */
	UFUNCTION()
	void OnPortalAbilityUpdated(bool bReady, float CooldownRemaining, bool bLastActivationValid);

	/** Q-key fallback that invokes the possessed character's portal mobility. */
	void ActivatePortalAbilityFallback();

	/** Called by the game mode when the Arcane Trial (defeat AI enemies) progress changes. */
	UFUNCTION()
	void OnArcaneTrialUpdated(int32 EnemiesDefeated, int32 Goal, bool bComplete);

#if !UE_BUILD_SHIPPING
	/** Development-only capture helper (F8): turns the view toward the nearest living AI enemy. */
	void DebugFaceNearestEnemy();

	/** Development-only capture helper (F9): turns the view toward the nearest portal. */
	void DebugFaceNearestPortal();

	/** Sets the control rotation so the camera looks at a world location. */
	void DebugFaceLocation(const FVector& TargetLocation);

	/** Development-only capture helper (F10): logs every portal pair transform for the evidence report. */
	void DebugLogPortals();

	/** Development-only capture helper (F11): enters the nearest portal through its real overlap path. */
	void DebugEnterNearestPortal();
#endif

public:
	/** Displays a brief native hit-confirmation flash. */
	void ShowHitFeedback();

protected:

	/** Builds crosshair, portal cooldown, and hit feedback without Blueprint dependencies. */
	void CreateNativeHUD();

	/** Refreshes cooldown text while the ability is unavailable. */
	void RefreshPortalHUD();

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

public:

	/** Assigns a team ID to this player */
	void SetTeam(uint8 Team);
};
