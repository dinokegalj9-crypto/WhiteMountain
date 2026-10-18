#include "EscapeScoreComponent.h"
#include "WhiteMountain.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	constexpr int32 MaxRecentEvents = 6;

	const TCHAR* MoveName(EShinobiMove Move)
	{
		switch (Move)
		{
		case EShinobiMove::Climb:      return TEXT("Climbing");
		case EShinobiMove::LedgeGrab:  return TEXT("Ledge grab");
		case EShinobiMove::Mantle:     return TEXT("Climbing over");
		case EShinobiMove::WallJump:   return TEXT("Wall jump");
		case EShinobiMove::Rope:       return TEXT("Rope");
		case EShinobiMove::RopeLadder: return TEXT("Rope ladder");
		case EShinobiMove::Kaginawa:   return TEXT("Kaginawa");
		case EShinobiMove::Tightrope:  return TEXT("Tightrope");
		}
		return TEXT("?");
	}
}

UEscapeScoreComponent::UEscapeScoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	for (float& Time : LastMoveTime)
	{
		Time = -1000.f;
	}

	auto AddTeachable = [this](EShinobiMove Move, int32 Cost)
	{
		FShinobiMoveCost Entry;
		Entry.Move = Move;
		Entry.Cost = Cost;
		TeachableMoves.Add(Entry);
	};
	AddTeachable(EShinobiMove::WallJump, 150);
	AddTeachable(EShinobiMove::RopeLadder, 250);
}

void UEscapeScoreComponent::BeginPlay()
{
	Super::BeginPlay();

	for (uint8 Index = 0; Index < NumShinobiMoves; ++Index)
	{
		const EShinobiMove Move = static_cast<EShinobiMove>(Index);
		const bool bTeachable = TeachableMoves.ContainsByPredicate([Move](const FShinobiMoveCost& Entry) { return Entry.Move == Move; });
		if (!bTeachable || bUnlockAllMovesForTesting)
		{
			UnlockedMoves.Add(Index);
		}
	}
}

void UEscapeScoreComponent::NotifyTraversal(EShinobiMove Move)
{
	LastMoveTime[static_cast<uint8>(Move)] = GetWorld()->GetTimeSeconds();
}

void UEscapeScoreComponent::ReportPursuitStarted(AActor* Pursuer)
{
	Pursuers.Add(Pursuer);
}

void UEscapeScoreComponent::ReportPursuerGone(AActor* Pursuer)
{
	Pursuers.Remove(Pursuer);
}

int32 UEscapeScoreComponent::GetPursuerCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Pursuer : Pursuers)
	{
		Count += Pursuer.IsValid() ? 1 : 0;
	}
	return Count;
}

void UEscapeScoreComponent::ReportEscape(AActor* Pursuer, EEscapeReason Reason, float PursuerMultiplier)
{
	Pursuers.Remove(Pursuer);

	const float Now = GetWorld()->GetTimeSeconds();

	int32 Base = 0;
	const TCHAR* Label = TEXT("");
	switch (Reason)
	{
	case EEscapeReason::Unreachable: Base = UnreachablePoints; Label = TEXT("Cannot follow you"); break;
	case EEscapeReason::Detour:      Base = DetourPoints;      Label = TEXT("Detour too long"); break;
	case EEscapeReason::LostTrail:   Base = LostTrailPoints;   Label = TEXT("Lost your trail"); break;
	}

	int32 HeightBonus = 0;
	if (Pursuer && GetOwner())
	{
		const float HeightAbove = GetOwner()->GetActorLocation().Z - Pursuer->GetActorLocation().Z;
		HeightBonus = FMath::Max(0, FMath::FloorToInt(HeightAbove / 100.f)) * PointsPerMeterAbove;
	}

	int32 MovesUsed = 0;
	for (const float Time : LastMoveTime)
	{
		MovesUsed += (Now - Time) <= TraversalMemorySeconds ? 1 : 0;
	}
	const int32 MoveBonus = MovesUsed * PointsPerMoveType;

	Chain = (Now - LastEscapeTime) <= ChainWindowSeconds ? Chain + 1 : 0;
	LastEscapeTime = Now;
	const float ChainMultiplier = FMath::Min(1.f + Chain * ChainStep, MaxChainMultiplier);

	const int32 Earned = FMath::RoundToInt((Base + HeightBonus + MoveBonus) * PursuerMultiplier * ChainMultiplier);
	Points += Earned;
	LifetimePoints += Earned;

	AddEvent(Earned, FString::Printf(TEXT("%s  (%d + height %d + moves %d) x%.1f x%.1f"),
		Label, Base, HeightBonus, MoveBonus, PursuerMultiplier, ChainMultiplier));
	UE_LOG(LogWhiteMountain, Log, TEXT("Escape from %s: +%d"), Pursuer ? *Pursuer->GetName() : TEXT("?"), Earned);
}

void UEscapeScoreComponent::ReportCaught(AActor* Pursuer)
{
	Chain = 0;
	LastEscapeTime = -1000.f;
	const int32 Lost = FMath::Min(Points, CaughtPenalty);
	Points -= Lost;
	AddEvent(-Lost, TEXT("Caught!"));
}

bool UEscapeScoreComponent::IsMoveUnlocked(EShinobiMove Move) const
{
	return bUnlockAllMovesForTesting || UnlockedMoves.Contains(static_cast<uint8>(Move));
}

bool UEscapeScoreComponent::TryBuyNextMove(FString& OutMessage)
{
	const FShinobiMoveCost* Cheapest = nullptr;
	for (const FShinobiMoveCost& Entry : TeachableMoves)
	{
		if (!IsMoveUnlocked(Entry.Move) && (!Cheapest || Entry.Cost < Cheapest->Cost))
		{
			Cheapest = &Entry;
		}
	}

	if (!Cheapest)
	{
		OutMessage = TEXT("Master: You have learned everything I can show you for now.");
		return false;
	}
	if (Points < Cheapest->Cost)
	{
		OutMessage = FString::Printf(TEXT("Master: %s costs %d escape points. You have %d."), MoveName(Cheapest->Move), Cheapest->Cost, Points);
		return false;
	}

	Points -= Cheapest->Cost;
	UnlockedMoves.Add(static_cast<uint8>(Cheapest->Move));
	OutMessage = FString::Printf(TEXT("A village master taught you: %s"), MoveName(Cheapest->Move));
	AddEvent(-Cheapest->Cost, OutMessage);
	return true;
}

void UEscapeScoreComponent::PushMessage(const FString& Text)
{
	AddEvent(0, Text);
}

void UEscapeScoreComponent::AddEvent(int32 InPoints, const FString& Text)
{
	FEscapeScoreEvent Event;
	Event.Points = InPoints;
	Event.Text = Text;
	Event.Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	RecentEvents.Add(Event);
	if (RecentEvents.Num() > MaxRecentEvents)
	{
		RecentEvents.RemoveAt(0);
	}
	OnScoreEvent.Broadcast(Event);
}
