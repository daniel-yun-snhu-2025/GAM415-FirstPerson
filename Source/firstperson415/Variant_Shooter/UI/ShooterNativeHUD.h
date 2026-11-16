#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterNativeHUD.generated.h"

class UTextBlock;
class UBorder;

/** Native fallback HUD for crosshair, Arcane Trial objective, portal cooldown/invalid state, and hit feedback. */
UCLASS()
class FIRSTPERSON415_API UShooterNativeHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	/**
	 * The widget tree must exist before UUserWidget::RebuildWidget converts WidgetTree->RootWidget
	 * into Slate. NativeConstruct runs afterwards (from OnWidgetRebuilt), which is too late.
	 */
	virtual TSharedRef<SWidget> RebuildWidget() override;

public:
	void SetPortalState(bool bReady, float CooldownRemaining, bool bLastActivationValid);
	void SetObjectiveProgress(int32 EnemiesDefeated, int32 Goal, bool bComplete);
	void ShowHitFeedback();

private:
	void BuildNativeTree();

	/** Creates a text block on a dark translucent panel so it stays readable on bright scenes. */
	UTextBlock* AddPanelText(class UCanvasPanel* Root, FName Name, const FAnchors& Anchors,
		const FVector2D& Alignment, const FVector2D& Position, int32 FontSize);

	UPROPERTY()
	TObjectPtr<UTextBlock> PortalText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY()
	TObjectPtr<UTextBlock> CrosshairText;

	UPROPERTY()
	TObjectPtr<UBorder> HitBorder;

	/** Last portal text, used to log state changes once instead of every refresh. */
	FString LastPortalStatus;

	FTimerHandle HitTimer;
	void ClearHitFeedback();
};
