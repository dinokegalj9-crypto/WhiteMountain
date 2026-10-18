#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShinobiTypes.h"
#include "ShinobiEnemy.generated.h"

class UStaticMeshComponent;

/**
 * A guard. Walks only where the navmesh goes: no climbing, no jumping, no ropes.
 * That limit is what makes the shinobi's routes worth escape points.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AShinobiEnemy();

	virtual void BeginPlay() override;

	/** Applies the per-type defaults below. Called in BeginPlay; call again after changing GuardType at runtime. */
	void ApplyGuardType();

	EEnemyAlert GetAlert() const;

	/** Knocked off his feet (oil, a falling log). Does nothing if he is already down. Returns true if he fell. */
	bool Slip(float Duration);

	bool IsStunned() const { return bStunned; }

	// --- Takedowns (the shinobi's quiet moves) ---
	/** Out cold after a takedown. Lies still until he wakes up, then searches around him. */
	bool IsUnconscious() const { return bUnconscious; }

	/** Held in a takedown right now. */
	bool IsHeld() const { return bHeld; }

	/**
	 * Can the shinobi standing at ShinobiLocation take him down?
	 * From behind while he is calm, or from any side while he lies slipped on the ground.
	 */
	bool CanBeTakenDown(const FVector& ShinobiLocation) const;

	/** The shinobi grabs him: he stops dead and cannot see or shout. */
	void BeginTakedown();

	/** The takedown worked: he drops and stays out for Seconds. */
	void FinishTakedown(float Seconds);

	/** The shinobi had to let go (caught, shot): he tears free and chases at once. */
	void BreakTakedown(const FVector& ShinobiLocation);

	/** Another guard already found him lying here (so it does not raise the alarm twice). */
	bool bBodyFound = false;

	/** The shinobi can take him down from this close. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Takedown")
	float TakedownRange = 160.f;

	/** "Behind" means within this many degrees of straight behind his back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Takedown")
	float TakedownBehindAngle = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	EGuardType GuardType = EGuardType::Spearman;

	/** When true, BeginPlay overwrites the values below from GuardType. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	bool bUseGuardTypeDefaults = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float PatrolSpeed = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float ChaseSpeed = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float SightRange = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float SightHalfAngle = 60.f;

	/** While alert, the guard notices the shinobi this close in any direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float CloseAwarenessRadius = 500.f;

	/** A crouching shinobi is not noticed beyond this distance by a calm guard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float CrouchHiddenDistance = 900.f;

	/** Seconds without sight before the guard starts searching. Dogs follow scent much longer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float LostSightGiveUpTime = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float SearchDuration = 6.f;

	// --- Bow (archers) ---
	/** Set by GuardType: only archers shoot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Bow")
	bool bCanShoot = false;

	/** Seconds standing still and aiming before the arrow flies. Watch for "AIM" over his head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Bow")
	float AimTime = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Bow")
	float ShotCooldown = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Bow")
	float MinShootDistance = 400.f;

	/** Chance to hit before distance, movement, cover and route rehearsal are taken into account. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Bow")
	float BaseHitChance = 0.55f;

	bool IsAiming() const;

	/** Escape points from this guard are multiplied by this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float EscapePointMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	FLinearColor BodyColor = FLinearColor(0.55f, 0.08f, 0.06f);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Guard")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, Category = "Guard")
	TObjectPtr<UStaticMeshComponent> PlaceholderFace;

private:
	void Recover();
	/** Grey-box fall: tip the visible body over by Roll degrees (the capsule stays upright). */
	void FallOver(float Roll);
	/** Pawns bump into him again once nobody stands where he lies. */
	void RestorePawnBlock();

	bool bStunned = false;
	bool bFallen = false;
	bool bUnconscious = false;
	bool bHeld = false;
	FRotator BodyRotationBeforeSlip = FRotator::ZeroRotator;
	FRotator MeshRotationBeforeSlip = FRotator::ZeroRotator;
	FTimerHandle RecoverTimer;
	FTimerHandle BlockTimer;
};
