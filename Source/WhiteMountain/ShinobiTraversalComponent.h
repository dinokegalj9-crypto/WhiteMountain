#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShinobiTypes.h"
#include "ShinobiTraversalComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UCapsuleComponent;
class UEscapeScoreComponent;
class AShinobiRope;
class AShinobiGrapplePoint;
class AShinobiTightrope;

/**
 * Everything the shinobi does beyond walking and jumping:
 * wall climbing, ledge hang + shimmy, mantling / vaulting, wall jump,
 * ropes, rope ladders tied at edges, kaginawa (hook and rope), tightrope balance, stamina.
 *
 * Implemented with MOVE_Flying + direct positioning so it works without animations.
 * Later this should move into a custom movement mode and use Motion Warping for animations.
 */
UCLASS(ClassGroup = (Shinobi), meta = (BlueprintSpawnableComponent))
class WHITEMOUNTAIN_API UShinobiTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShinobiTraversalComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Input from the character. Each Handle* returns true when traversal used the press. ---
	void SetMoveInput(const FVector2D& Input) { MoveInput = Input; }
	bool HandleJumpPressed();
	bool HandleDropPressed();
	bool HandleInteractPressed();
	bool HandleRopeLadderPressed();
	bool HandleKaginawaPressed();

	/** Drains sprint stamina for this frame. Returns false when the shinobi is out of breath. */
	bool ConsumeSprintStamina(float DeltaTime);

	bool IsTraversing() const { return State != EShinobiTraversalState::None; }
	EShinobiTraversalState GetState() const { return State; }
	FString GetStateDisplayName() const;
	int32 GetRopeLadderCount() const { return RopeLadders; }
	float GetStamina() const { return Stamina; }
	float GetMaxStamina() const { return MaxStamina; }
	/** -1..1 while on a tightrope; falling off at +-1. */
	float GetBalance() const { return Balance; }

	// --- Climbing ---
	/** Design doc: only marked surfaces can be climbed. Tag the actor or component "Climbable". */
	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	bool bRequireClimbableTag = true;

	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	FName ClimbableTag = TEXT("Climbable");

	/** Never climbable, and ledges on it cannot be grabbed either. */
	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	FName NoClimbTag = TEXT("NoClimb");

	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	float ClimbSpeed = 140.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	float ClimbStartDistance = 35.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	float WallProbeDistance = 45.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	float WallOffset = 3.f;

	/** A surface counts as a wall when |normal.Z| is below this. */
	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	float MaxClimbableNormalZ = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Climb")
	float WalkableNormalZ = 0.7f;

	// --- Mantle / vault ---
	UPROPERTY(EditAnywhere, Category = "Traversal|Mantle")
	float LedgeDepth = 25.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Mantle")
	float MantleReachAbove = 80.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Mantle")
	float MantleDuration = 0.45f;

	/** Obstacles up to this height above the feet are vaulted with Jump. */
	UPROPERTY(EditAnywhere, Category = "Traversal|Mantle")
	float VaultMaxHeight = 170.f;

	// --- Ledge hang ---
	UPROPERTY(EditAnywhere, Category = "Traversal|Ledge")
	bool bAutoLedgeGrab = true;

	UPROPERTY(EditAnywhere, Category = "Traversal|Ledge")
	float LedgeGrabReach = 35.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Ledge")
	float LedgeReachAbove = 60.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Ledge")
	float HangCapsuleTopBelowLedge = 10.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Ledge")
	float ShimmySpeed = 110.f;

	// --- Wall jump ---
	UPROPERTY(EditAnywhere, Category = "Traversal|WallJump")
	float WallJumpAway = 450.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|WallJump")
	float WallJumpUp = 520.f;

	// --- Rope ---
	UPROPERTY(EditAnywhere, Category = "Traversal|Rope")
	float RopeGrabRadius = 110.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Rope")
	float RopeClimbSpeed = 150.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Rope")
	float RopeHoldOffset = 8.f;

	/** Degrees per second when circling a free-hanging rope with A/D. */
	UPROPERTY(EditAnywhere, Category = "Traversal|Rope")
	float RopeOrbitSpeed = 90.f;

	// --- Kaginawa ---
	UPROPERTY(EditAnywhere, Category = "Traversal|Kaginawa")
	float KaginawaRange = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Kaginawa")
	float KaginawaAimAngle = 30.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Kaginawa")
	float KaginawaThrowTime = 0.4f;

	/** The shinobi pulls the rope up behind him so nobody can follow. */
	UPROPERTY(EditAnywhere, Category = "Traversal|Kaginawa")
	float KaginawaRetractDelay = 1.f;

	// --- Rope ladder ---
	UPROPERTY(EditAnywhere, Category = "Traversal|RopeLadder")
	int32 StartingRopeLadders = 3;

	UPROPERTY(EditAnywhere, Category = "Traversal|RopeLadder")
	float LadderDeployTime = 1.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|RopeLadder")
	float MinLadderDrop = 200.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|RopeLadder")
	float MaxLadderLength = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|RopeLadder")
	float EdgeSearchDistance = 150.f;

	// --- Tightrope ---
	UPROPERTY(EditAnywhere, Category = "Traversal|Tightrope")
	float TightropeSpeed = 160.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Tightrope")
	float TightropeMountRadius = 130.f;

	/** How hard the balance drifts (grows with speed). */
	UPROPERTY(EditAnywhere, Category = "Traversal|Tightrope")
	float BalanceDrift = 0.9f;

	/** How strongly A/D push the balance back. */
	UPROPERTY(EditAnywhere, Category = "Traversal|Tightrope")
	float BalanceCorrection = 2.2f;

	// --- Stamina ---
	UPROPERTY(EditAnywhere, Category = "Traversal|Stamina")
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Stamina")
	float SprintStaminaPerSecond = 12.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Stamina")
	float ClimbStaminaPerSecond = 7.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Stamina")
	float HangStaminaPerSecond = 4.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Stamina")
	float RopeStaminaPerSecond = 5.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Stamina")
	float StaminaRegenPerSecond = 22.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Stamina")
	float StaminaRegenDelay = 1.f;

	UPROPERTY(EditAnywhere, Category = "Traversal|Debug")
	bool bDebugDraw = false;

private:
	// Ticks per state
	void TickClimbing(float DeltaTime);
	void TickLedgeHang(float DeltaTime);
	void TickMantle(float DeltaTime);
	void TickRope(float DeltaTime);
	void TickBalancing(float DeltaTime);
	void TickStamina(float DeltaTime);

	// Transitions
	void EnterTraversalMovement(EShinobiTraversalState NewState);
	void StopTraversal(float GrabCooldown = 0.f);
	void StartClimbing(const FHitResult& WallHit);
	bool TryGrabLedge();
	void StartLedgeHang(const FVector& WallPoint, const FVector& Normal, float LedgeZ);
	bool TryMantleOver(const FVector& WallNormal2D);
	bool StartMantle(const FVector& LedgeTop);
	void WallJump();
	void AttachToRope(AShinobiRope* Rope);
	void UpdateRopePosition();
	bool TryExitRopeTop();
	void JumpOffRope();
	void SpawnRopeLadder();
	void SpawnKaginawaRope();
	bool TryMountTightrope();
	void FallOffTightrope();

	// Queries
	bool TraceWorld(const FVector& Start, const FVector& End, FHitResult& OutHit) const;
	bool IsWall(const FHitResult& Hit) const;
	bool IsClimbable(const FHitResult& Hit) const;
	bool HasTag(const FHitResult& Hit, FName Tag) const;
	bool FindLedgeTop(const FVector& WallPoint, const FVector& WallNormal2D, float MinZ, float MaxZ, FVector& OutTop) const;
	bool CapsuleFitsAt(const FVector& Location) const;
	bool HasFloorBelow(float MaxDistance) const;
	AShinobiGrapplePoint* FindKaginawaTarget() const;
	bool CheckUnlocked(EShinobiMove Move);
	void Notify(EShinobiMove Move);
	void Message(const FString& Text);
	bool DrainStamina(float Amount);
	float Radius() const;
	float HalfHeight() const;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> Movement;

	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(Transient)
	TObjectPtr<UEscapeScoreComponent> Score;

	UPROPERTY(Transient)
	TObjectPtr<AShinobiRope> CurrentRope;

	UPROPERTY(Transient)
	TObjectPtr<AShinobiTightrope> CurrentTightrope;

	TArray<TWeakObjectPtr<AShinobiRope>> DeployedLadders;
	TWeakObjectPtr<AShinobiGrapplePoint> PendingGrapplePoint;

	EShinobiTraversalState State = EShinobiTraversalState::None;
	FVector2D MoveInput = FVector2D::ZeroVector;
	FVector WallNormal = FVector::ForwardVector;
	float LedgeZ = 0.f;
	float GrabCooldownRemaining = 0.f;

	FVector MantleStart = FVector::ZeroVector;
	FVector MantleTarget = FVector::ZeroVector;
	float MantleAlpha = 0.f;
	TWeakObjectPtr<AShinobiRope> RopeToRetractAfterMantle;

	float RopeDistance = 0.f;
	FVector RopeFacing = FVector::ForwardVector;

	float TightropeDistance = 0.f;
	float TightropeDirection = 1.f;
	float Balance = 0.f;
	float BalanceVelocity = 0.f;

	FVector PendingLadderTop = FVector::ZeroVector;
	FVector PendingLadderExit = FVector::ForwardVector;
	float PendingLadderLength = 0.f;
	FTimerHandle DeployTimer;

	int32 RopeLadders = 0;
	float Stamina = 100.f;
	float LastStaminaDrainTime = -1000.f;

	bool bSavedOrientToMovement = true;
	float SavedMaxFlySpeed = 600.f;
	float SavedBrakingFlying = 0.f;
};
