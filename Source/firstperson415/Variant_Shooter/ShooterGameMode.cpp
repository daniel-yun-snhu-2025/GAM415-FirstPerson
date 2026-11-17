// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"
#include "ShooterUI.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ShooterPlayerController.h"
#include "ShooterNPCSpawner.h"
#include "EngineUtils.h"

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// create the UI
	if ((ShooterUI = CreateWidget<UShooterUI>(UGameplayStatics::GetPlayerController(GetWorld(), 0), ShooterUIClass)))
	{
		ShooterUI->AddToViewport(0);
	}

	// GAM 495 refinement: size the Arcane Trial to the enemies this level will spawn.
	if (ShouldSpawnEnemyNPCs())
	{
		int32 SpawnableEnemies = 0;
		for (TActorIterator<AShooterNPCSpawner> It(GetWorld()); It; ++It)
		{
			SpawnableEnemies += FMath::Max(0, It->GetRemainingSpawnCount());
		}
		if (SpawnableEnemies > 0)
		{
			ArcaneTrialGoal = FMath::Min(ArcaneTrialGoal, SpawnableEnemies);
		}
		UE_LOG(LogTemp, Log, TEXT("ArcaneTrial: level spawns %d enemies, goal %d"), SpawnableEnemies, ArcaneTrialGoal);
		OnArcaneTrialUpdated.Broadcast(ArcaneTrialEnemiesDefeated, ArcaneTrialGoal, false);
	}

	// create each additional local player.
	// Player 0 will be created automatically as part of regular game init
	for (int32 i = 2; i <= NumberOfLocalPlayers; ++i)
	{
		if (AShooterPlayerController* NewPlayer = Cast<AShooterPlayerController>(UGameplayStatics::CreatePlayer(GetWorld(), -1, true)))
		{
			NewPlayer->SetTeam(1 - (i % 2));
		}
	}
}

AActor* AShooterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// build the current player tag
	FName PlayerTag = FName(*FString::Printf(TEXT("Player%d"), CurrentPlayerStartAssignment));

	// find all player starts with the matching player tag
	TArray<AActor*> PlayerStarts;

	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), PlayerTag, PlayerStarts);

	// increment the player start assignment index
	++CurrentPlayerStartAssignment;

	// if no PlayerStarts were found, default to all PlayerStarts instead
	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	// have we found at least one PlayerStart?
	if (!PlayerStarts.IsEmpty())
	{
		return PlayerStarts[ FMath::RandRange(0, PlayerStarts.Num() - 1) ];
	}

	// no PlayerStarts in the level
	return nullptr;
}

void AShooterGameMode::IncrementTeamScore(uint8 TeamByte)
{
	// retrieve the team score if any
	int32 Score = 0;
	if (int32* FoundScore = TeamScores.Find(TeamByte))
	{
		Score = *FoundScore;
	}

	// increment the score for the given team
	++Score;
	TeamScores.Add(TeamByte, Score);

	// update the UI
	if (ShooterUI)
	{
		ShooterUI->BP_UpdateScore(TeamByte, Score);
	}
}

bool AShooterGameMode::ShouldSpawnEnemyNPCs() const
{
	// only spawn enemy NPCs in single player mode
	return NumberOfLocalPlayers < 2;
}

void AShooterGameMode::RegisterEnemyDefeated()
{
	// GAM 495 refinement: the template's AI enemies become a short objective. The count stops at
	// the level-derived goal so late kills cannot push the HUD past the displayed total.
	if (ArcaneTrialEnemiesDefeated >= ArcaneTrialGoal)
	{
		return;
	}

	++ArcaneTrialEnemiesDefeated;
	const bool bComplete = ArcaneTrialEnemiesDefeated >= ArcaneTrialGoal;
	UE_LOG(LogTemp, Log, TEXT("ArcaneTrial: enemies defeated %d / %d%s"), ArcaneTrialEnemiesDefeated, ArcaneTrialGoal,
		bComplete ? TEXT(" - COMPLETE") : TEXT(""));
	OnArcaneTrialUpdated.Broadcast(ArcaneTrialEnemiesDefeated, ArcaneTrialGoal, bComplete);
}
