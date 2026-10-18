#include "ShinobiHUD.h"
#include "ShinobiCharacter.h"
#include "ShinobiEnemy.h"
#include "ShinobiTraversalComponent.h"
#include "EscapeScoreComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"

namespace
{
	const TCHAR* ControlsText[] =
	{
		TEXT("WASD move   Mouse camera   Shift sprint   C crouch / let go / ukemi"),
		TEXT("Space jump / climb a marked wall / vault / wall jump"),
		TEXT("E rope, tightrope, master, oil jar, push log, steal   Q kaginawa (look at an anchor)"),
		TEXT("R rope ladder at an edge (R again at the top = untie)"),
		TEXT("Behind a calm guard: E Silent Sleep (slow, silent)   F Pressure Strike (fast, he groans)"),
	};
}

void AShinobiHUD::DrawHUD()
{
	Super::DrawHUD();

	const AShinobiCharacter* Shinobi = Cast<AShinobiCharacter>(GetOwningPawn());
	if (!Shinobi || !Canvas)
	{
		return;
	}
	const UShinobiTraversalComponent* Traversal = Shinobi->GetTraversal();
	const UEscapeScoreComponent* Score = Shinobi->GetEscapeScore();
	UFont* TitleFont = GEngine->GetMediumFont();
	UFont* SmallFont = GEngine->GetSmallFont();

	float Y = 30.f;
	const float X = 30.f;

	DrawText(FString::Printf(TEXT("ESCAPE POINTS: %d"), Score->GetPoints()), FLinearColor(1.f, 0.85f, 0.4f), X, Y, TitleFont, 1.3f);
	Y += 32.f;
	DrawText(FString::Printf(TEXT("Total earned: %d   Chain: x%d   Pursuers: %d"), Score->GetLifetimePoints(), Score->GetChain() + 1, Score->GetPursuerCount()),
		FLinearColor::White, X, Y, SmallFont);
	Y += 22.f;
	DrawText(FString::Printf(TEXT("State: %s   Rope ladders: %d"), *Traversal->GetStateDisplayName(), Traversal->GetRopeLadderCount()),
		FLinearColor::White, X, Y, SmallFont);
	Y += 24.f;

	DrawText(TEXT("Stamina"), FLinearColor::White, X, Y, SmallFont);
	DrawBar(X + 100.f, Y + 2.f, 200.f, 12.f, Traversal->GetStamina() / FMath::Max(1.f, Traversal->GetMaxStamina()), FLinearColor(0.3f, 0.8f, 0.3f));
	Y += 22.f;

	if (Traversal->GetState() == EShinobiTraversalState::Balancing)
	{
		// Balance meter: centre is safe, edges mean falling.
		const float W = 300.f;
		const float BX = Canvas->ClipX * 0.5f - W * 0.5f;
		const float BY = Canvas->ClipY * 0.7f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), BX, BY, W, 14.f);
		DrawRect(FLinearColor(0.2f, 0.8f, 0.2f, 0.6f), BX + W * 0.35f, BY, W * 0.3f, 14.f);
		const float Marker = BX + W * 0.5f * (1.f + FMath::Clamp(Traversal->GetBalance(), -1.f, 1.f));
		DrawRect(FLinearColor::White, Marker - 3.f, BY - 4.f, 6.f, 22.f);
		DrawText(TEXT("BALANCE (A / D)"), FLinearColor::White, BX + 90.f, BY - 24.f, SmallFont);
	}

	// Takedown: progress bar while holding, otherwise a hint when one is possible.
	{
		const float W = 300.f;
		const float BX = Canvas->ClipX * 0.5f - W * 0.5f;
		const float BY = Canvas->ClipY * 0.62f;
		if (Shinobi->IsInTakedown())
		{
			DrawText(Shinobi->GetTakedownName(), FLinearColor::White, BX, BY - 24.f, SmallFont);
			DrawBar(BX, BY, W, 14.f, Shinobi->GetTakedownProgress(), FLinearColor(0.6f, 0.75f, 1.f));
		}
		else if (Shinobi->FindTakedownTarget())
		{
			DrawText(TEXT("[E] Silent Sleep    [F] Pressure Strike"), FLinearColor(0.75f, 0.85f, 1.f), BX, BY, SmallFont);
		}
	}

	// Event feed, newest last, fading out.
	const float Now = GetWorld()->GetTimeSeconds();
	Y += 10.f;
	for (const FEscapeScoreEvent& Event : Score->GetRecentEvents())
	{
		const float Age = Now - Event.Time;
		if (Age > 6.f)
		{
			continue;
		}
		const float Alpha = FMath::Clamp(1.f - (Age - 4.f) / 2.f, 0.f, 1.f);
		const FLinearColor Color = Event.Points > 0 ? FLinearColor(1.f, 0.85f, 0.3f, Alpha)
			: Event.Points < 0 ? FLinearColor(1.f, 0.35f, 0.3f, Alpha)
			: FLinearColor(0.85f, 0.85f, 0.85f, Alpha);
		const FString Line = Event.Points != 0 ? FString::Printf(TEXT("%+d  %s"), Event.Points, *Event.Text) : Event.Text;
		DrawText(Line, Color, X, Y, SmallFont);
		Y += 20.f;
	}

	if (bShowControls)
	{
		float CY = Canvas->ClipY - 30.f - UE_ARRAY_COUNT(ControlsText) * 18.f;
		for (const TCHAR* Line : ControlsText)
		{
			DrawText(Line, FLinearColor(0.8f, 0.8f, 0.8f, 0.8f), X, CY, SmallFont);
			CY += 18.f;
		}
	}

	DrawGuardMarkers();
}

void AShinobiHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X, Y, Width, Height);
	DrawRect(Color, X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height);
}

void AShinobiHUD::DrawGuardMarkers()
{
	UFont* MarkerFont = GEngine->GetLargeFont();
	for (TActorIterator<AShinobiEnemy> It(GetWorld()); It; ++It)
	{
		const FVector Screen = Project(It->GetActorLocation() + FVector(0.f, 0.f, 130.f));
		if (Screen.Z <= 0.f)
		{
			continue;	// behind the camera
		}

		const TCHAR* Symbol = nullptr;
		FLinearColor Color = FLinearColor::White;
		switch (It->GetAlert())
		{
		case EEnemyAlert::Chase:      Symbol = TEXT("!");   Color = FLinearColor(1.f, 0.2f, 0.1f); break;
		case EEnemyAlert::Search:     Symbol = TEXT("?");   Color = FLinearColor(1.f, 0.8f, 0.1f); break;
		case EEnemyAlert::Frustrated: Symbol = TEXT("...");  Color = FLinearColor(0.6f, 0.6f, 1.f); break;
		default: break;
		}
		if (It->IsUnconscious())
		{
			Symbol = TEXT("zzz");
			Color = It->bBodyFound ? FLinearColor(1.f, 0.8f, 0.1f) : FLinearColor(0.6f, 0.6f, 0.6f);
		}
		if (It->IsAiming())
		{
			Symbol = TEXT("AIM");
			Color = FLinearColor(1.f, 0.45f, 0.f);
		}
		if (Symbol)
		{
			DrawText(Symbol, Color, Screen.X - 6.f, Screen.Y, MarkerFont, 1.5f);
		}
	}
}
