#include "ShooterNativeHUD.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"
#include "TimerManager.h"

TSharedRef<SWidget> UShooterNativeHUD::RebuildWidget()
{
	// Initialize() creates the (empty) WidgetTree for a native class; build our tree before the
	// base class turns RootWidget into Slate widgets.
	if (!IsDesignTime())
	{
		Initialize();
		if (WidgetTree && !WidgetTree->RootWidget)
		{
			BuildNativeTree();
		}
	}
	return Super::RebuildWidget();
}

UTextBlock* UShooterNativeHUD::AddPanelText(UCanvasPanel* Root, FName Name, const FAnchors& Anchors,
	const FVector2D& Alignment, const FVector2D& Position, int32 FontSize)
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*(Name.ToString() + TEXT("Panel"))));
	Panel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	Panel->SetPadding(FMargin(14.0f, 6.0f));

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);
	Panel->SetContent(Text);

	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(Anchors);
	PanelSlot->SetAlignment(Alignment);
	PanelSlot->SetPosition(Position);
	PanelSlot->SetAutoSize(true);
	return Text;
}

void UShooterNativeHUD::BuildNativeTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NativeHUDRoot"));
	WidgetTree->RootWidget = Root;

	// Full-screen hit flash sits underneath the text elements.
	HitBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HitFeedback"));
	HitBorder->SetBrushColor(FLinearColor(0.15f, 0.55f, 1.0f, 0.30f));
	HitBorder->SetVisibility(ESlateVisibility::Hidden);
	UCanvasPanelSlot* HitSlot = Root->AddChildToCanvas(HitBorder);
	HitSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	HitSlot->SetOffsets(FMargin(0.0f));

	CrosshairText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Crosshair"));
	FSlateFontInfo CrosshairFont = CrosshairText->GetFont();
	CrosshairFont.Size = 26;
	CrosshairText->SetFont(CrosshairFont);
	CrosshairText->SetText(FText::FromString(TEXT("+")));
	CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CrosshairText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	CrosshairText->SetShadowColorAndOpacity(FLinearColor::Black);
	UCanvasPanelSlot* CrosshairSlot = Root->AddChildToCanvas(CrosshairText);
	CrosshairSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	CrosshairSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CrosshairSlot->SetAutoSize(true);

	// Arcane Trial objective (defeat AI enemies with the spell projectile), top center.
	ObjectiveText = AddPanelText(Root, TEXT("ObjectiveStatus"), FAnchors(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.0f, 24.0f), 20);

	// Portal mobility state, bottom center.
	PortalText = AddPanelText(Root, TEXT("PortalStatus"), FAnchors(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(0.0f, -110.0f), 20);

	SetPortalState(true, 0.0f, true);
	SetObjectiveProgress(0, 0, false);
}

void UShooterNativeHUD::SetObjectiveProgress(int32 EnemiesDefeated, int32 Goal, bool bComplete)
{
	if (!ObjectiveText)
	{
		return;
	}
	if (Goal <= 0)
	{
		ObjectiveText->SetText(FText::FromString(TEXT("ARCANE TRIAL  DEFEAT AI ENEMIES")));
		ObjectiveText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.35f)));
		return;
	}
	const FString Status = bComplete
		? FString::Printf(TEXT("ARCANE TRIAL COMPLETE  %d / %d ENEMIES DEFEATED"), EnemiesDefeated, Goal)
		: FString::Printf(TEXT("ARCANE TRIAL  DEFEAT AI ENEMIES  %d / %d"), EnemiesDefeated, Goal);
	ObjectiveText->SetText(FText::FromString(Status));
	ObjectiveText->SetColorAndOpacity(FSlateColor(bComplete ? FLinearColor(0.3f, 1.0f, 0.4f) : FLinearColor(1.0f, 0.85f, 0.35f)));
	UE_LOG(LogTemp, Log, TEXT("NativeHUD: objective text '%s'"), *Status);
}

void UShooterNativeHUD::SetPortalState(bool bReady, float CooldownRemaining, bool bLastActivationValid)
{
	if (!PortalText)
	{
		return;
	}
	const FString Status = !bLastActivationValid ? TEXT("PORTAL BLOCKED")
		: bReady ? TEXT("Q  PORTAL READY")
		: FString::Printf(TEXT("Q  PORTAL  %.1fs"), CooldownRemaining);
	PortalText->SetText(FText::FromString(Status));
	PortalText->SetColorAndOpacity(FSlateColor(!bLastActivationValid ? FLinearColor(1.0f, 0.25f, 0.25f)
		: bReady ? FLinearColor(0.2f, 0.75f, 1.0f) : FLinearColor::Yellow));
	if (Status != LastPortalStatus)
	{
		LastPortalStatus = Status;
		UE_LOG(LogTemp, Log, TEXT("NativeHUD: portal text '%s'"), *Status);
	}
}

void UShooterNativeHUD::ShowHitFeedback()
{
	if (HitBorder)
	{
		HitBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (CrosshairText)
		{
			// Hit marker: the crosshair briefly turns into a blue X when a spell damages an enemy.
			CrosshairText->SetText(FText::FromString(TEXT("X")));
			CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.75f, 1.0f)));
		}
		UE_LOG(LogTemp, Log, TEXT("NativeHUD: hit marker shown"));
		GetWorld()->GetTimerManager().SetTimer(HitTimer, this, &UShooterNativeHUD::ClearHitFeedback, 0.15f, false);
	}
}

void UShooterNativeHUD::ClearHitFeedback()
{
	if (HitBorder)
	{
		HitBorder->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CrosshairText)
	{
		CrosshairText->SetText(FText::FromString(TEXT("+")));
		CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
}
