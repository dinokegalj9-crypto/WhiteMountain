#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShinobiTypes.h"
#include "EscapeScoreComponent.generated.h"

USTRUCT(BlueprintType)
struct FEscapeScoreEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Points = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	float Time = 0.f;
};

/** A move a village trainer can teach for escape points. */
USTRUCT(BlueprintType)
struct FShinobiMoveCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	EShinobiMove Move = EShinobiMove::WallJump;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 Cost = 150;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEscapeScoreEvent, const FEscapeScoreEvent&, Event);

/**
 * Escape points: awarded when a pursuer gives up because it cannot follow the shinobi's route.
 * Points are a currency spent with a village trainer on new moves (see AShinobiTrainer).
 *
 * Formula per escape:
 *   (base by reason + height above the pursuer + bonus per distinct traversal move used recently)
 *   x pursuer type multiplier x chain multiplier (escapes close together)
 */
UCLASS(ClassGroup = (Shinobi), meta = (BlueprintSpawnableComponent))
class WHITEMOUNTAIN_API UEscapeScoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEscapeScoreComponent();

	virtual void BeginPlay() override;

	// --- Called by traversal and by enemies ---
	void NotifyTraversal(EShinobiMove Move);
	void ReportPursuitStarted(AActor* Pursuer);
	void ReportEscape(AActor* Pursuer, EEscapeReason Reason, float PursuerMultiplier);
	void ReportCaught(AActor* Pursuer);
	void ReportPursuerGone(AActor* Pursuer);

	// --- Village trainer / progression ---
	UFUNCTION(BlueprintPure, Category = "Score")
	bool IsMoveUnlocked(EShinobiMove Move) const;

	/** Buys the cheapest locked move the player can afford. Returns false if nothing was bought. */
	bool TryBuyNextMove(FString& OutMessage);

	/** Short message on the HUD feed (no points). */
	void PushMessage(const FString& Text);

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetPoints() const { return Points; }

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetLifetimePoints() const { return LifetimePoints; }

	int32 GetChain() const { return Chain; }
	int32 GetPursuerCount() const;
	const TArray<FEscapeScoreEvent>& GetRecentEvents() const { return RecentEvents; }
	const TArray<FShinobiMoveCost>& GetTeachableMoves() const { return TeachableMoves; }

	UPROPERTY(BlueprintAssignable, Category = "Score")
	FOnEscapeScoreEvent OnScoreEvent;

	// --- Tuning ---
	UPROPERTY(EditAnywhere, Category = "Score|Points")
	int32 UnreachablePoints = 150;

	UPROPERTY(EditAnywhere, Category = "Score|Points")
	int32 DetourPoints = 100;

	UPROPERTY(EditAnywhere, Category = "Score|Points")
	int32 LostTrailPoints = 50;

	UPROPERTY(EditAnywhere, Category = "Score|Points")
	int32 PointsPerMeterAbove = 15;

	UPROPERTY(EditAnywhere, Category = "Score|Points")
	int32 PointsPerMoveType = 40;

	/** Traversal moves used within this many seconds count towards an escape. */
	UPROPERTY(EditAnywhere, Category = "Score|Points")
	float TraversalMemorySeconds = 12.f;

	UPROPERTY(EditAnywhere, Category = "Score|Chain")
	float ChainWindowSeconds = 10.f;

	UPROPERTY(EditAnywhere, Category = "Score|Chain")
	float ChainStep = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Score|Chain")
	float MaxChainMultiplier = 3.f;

	UPROPERTY(EditAnywhere, Category = "Score|Points")
	int32 CaughtPenalty = 25;

	/** Moves a village trainer can still teach. Everything not listed here is known from the start. */
	UPROPERTY(EditAnywhere, Category = "Score|Trainer")
	TArray<FShinobiMoveCost> TeachableMoves;

	/** Prototype switch: ignore the trainers and allow every move. */
	UPROPERTY(EditAnywhere, Category = "Score|Trainer")
	bool bUnlockAllMovesForTesting = false;

private:
	void AddEvent(int32 InPoints, const FString& Text);

	int32 Points = 0;
	int32 LifetimePoints = 0;
	int32 Chain = 0;
	float LastEscapeTime = -1000.f;
	float LastMoveTime[NumShinobiMoves];
	TSet<uint8> UnlockedMoves;
	TSet<TWeakObjectPtr<AActor>> Pursuers;
	TArray<FEscapeScoreEvent> RecentEvents;
};
