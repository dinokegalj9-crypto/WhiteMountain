#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShinobiTypes.h"
#include "ShinobiEnemyController.generated.h"

class AShinobiEnemy;
class UEscapeScoreComponent;

/** Result of asking the navmesh whether a guard can reach the shinobi. */
struct FShinobiRouteCheck
{
	bool bReachable = false;
	bool bDetour = false;
	FVector End = FVector::ZeroVector;
};

/**
 * Guard brain: Patrol -> Chase -> (Frustrated | Search) -> Return.
 *
 * Every RepathInterval the guard asks the navmesh for a route to the shinobi. If there is none
 * (shinobi on a roof, up a rope, across a tightrope) or it is absurdly long, the guard ends up
 * Frustrated below him and gives up: that is an escape and the shinobi gets points.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AShinobiEnemyController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	EEnemyAlert GetAlert() const { return Alert; }

	/** Someone shouted: run to Location and start chasing, even without seeing the shinobi. */
	void RaiseAlarm(const FVector& Location);

	/**
	 * Something odd here (a groan, a body, waking up on the ground): walk over and search.
	 * Ignored while chasing, unless bEvenIfChasing (he woke up and has no idea where you went).
	 * Ending this search gives no escape points: nobody was chasing you.
	 */
	void Investigate(const FVector& Location, bool bEvenIfChasing = false);

	bool IsAiming() const { return bAiming; }

	/**
	 * How likely an arrow from this guard hits the shinobi right now (0..1).
	 * Far, sprinting or crouched is harder; climbing or hanging is easier.
	 * On an escape route: unrehearsed is easier to hit, each rehearsal makes it harder.
	 */
	float GetHitChance() const;

	UPROPERTY(EditAnywhere, Category = "Guard|Pursuit")
	float RepathInterval = 0.3f;

	/** Seconds with no route to the shinobi before the guard gives up. */
	UPROPERTY(EditAnywhere, Category = "Guard|Pursuit")
	float UnreachableGiveUpTime = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Guard|Pursuit")
	float FrustratedDuration = 2.f;

	/** A route longer than this many times the straight distance counts as "cannot follow". */
	UPROPERTY(EditAnywhere, Category = "Guard|Pursuit")
	float DetourRatio = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Guard|Pursuit")
	float DetourMinLength = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Guard|Pursuit")
	float CatchRadius = 130.f;

	UPROPERTY(EditAnywhere, Category = "Guard|Pursuit")
	float CatchCooldown = 2.f;

	UPROPERTY(EditAnywhere, Category = "Guard|Patrol")
	float PatrolRadius = 700.f;

private:
	void EnterState(EEnemyAlert NewState);
	void TickPatrol(float DeltaSeconds);
	void TickChase(float DeltaSeconds);
	void TickSearch(float DeltaSeconds);
	void TickFrustrated(float DeltaSeconds);
	void TickReturn(float DeltaSeconds);

	bool CanSeePlayer() const;
	/** A calm guard who sees a comrade lying unconscious goes to look. Returns true if he found one. */
	bool CheckForBodies();
	FShinobiRouteCheck CheckRoute(const APawn* Target) const;
	bool IsIdle() const;
	void MoveToRandomPointNear(const FVector& Center, float Radius);
	void FaceTowards(const FVector& Location, float DeltaSeconds);
	void TryCatch();

	/** Archer aiming and shooting. Returns true while he stands still to aim. */
	bool TickBow(float DeltaSeconds, bool bSees);
	void Shoot();

	AShinobiEnemy* GetEnemy() const;
	APawn* GetPlayer() const;
	UEscapeScoreComponent* GetPlayerScore() const;

	EEnemyAlert Alert = EEnemyAlert::Patrol;
	EEscapeReason PendingReason = EEscapeReason::Unreachable;
	FVector HomeLocation = FVector::ZeroVector;
	FVector LastSeenLocation = FVector::ZeroVector;
	FShinobiRouteCheck LastRoute;
	float StateTime = 0.f;
	float UnreachableTime = 0.f;
	float LostSightTime = 0.f;
	float RepathTimer = 0.f;
	float WaitTimer = 0.f;
	float CatchTimer = 0.f;
	bool bInvestigating = false;	// searching because of a noise or a body, not a lost chase
	float BodyCheckTimer = 0.f;
	bool bAiming = false;
	float AimTimer = 0.f;
	float ShotTimer = 0.f;
};
