// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

class UShooterUI;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FArcaneTrialUpdatedDelegate, int32, EnemiesDefeated, int32, Goal, bool, bComplete);

/**
 *  Simple GameMode for a first person shooter game
 *  Manages game UI
 *  Keeps track of team scores
 */
UCLASS(abstract)
class FIRSTPERSON415_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:

	/** Type of UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Shooter")
	TSubclassOf<UShooterUI> ShooterUIClass;

	/** Pointer to the UI widget */
	TObjectPtr<UShooterUI> ShooterUI;

	/** Map of scores by team ID */
	TMap<uint8, int32> TeamScores;

	/**
	 * Maximum number of enemy NPCs the Arcane Trial asks for. At BeginPlay the goal is reduced to the
	 * number of enemies the level's spawners will actually produce, so the objective is always completable.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Arcane Trial", meta=(ClampMin=1, ClampMax=20))
	int32 ArcaneTrialGoal = 3;

	/** Enemy NPCs defeated by the player team during this match. */
	int32 ArcaneTrialEnemiesDefeated = 0;

protected:

	/** Determines how many local players should be spawned on game start */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer", meta = (ClampMin = 1, ClampMax = 4))
	int32 NumberOfLocalPlayers = 1;

	/** Used to assign players to different PlayerStarts in the level */
	int32 CurrentPlayerStartAssignment = 0;

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Assigns a PlayerStart to a specific player */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

public:

	/** Increases the score for the given team */
	void IncrementTeamScore(uint8 TeamByte);

	/** Returns true if enemy NPCs should be used */
	bool ShouldSpawnEnemyNPCs() const;

	/** Records one defeated enemy NPC for the Arcane Trial objective and notifies the HUD. */
	void RegisterEnemyDefeated();

	/** Objective state for HUDs created after the match has started. */
	int32 GetArcaneTrialProgress() const { return ArcaneTrialEnemiesDefeated; }
	int32 GetArcaneTrialGoal() const { return ArcaneTrialGoal; }

	/** Broadcast whenever the Arcane Trial progress changes. */
	UPROPERTY(BlueprintAssignable, Category="Arcane Trial")
	FArcaneTrialUpdatedDelegate OnArcaneTrialUpdated;
};
