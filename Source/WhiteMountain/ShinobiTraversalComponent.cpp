#include "ShinobiTraversalComponent.h"
#include "WhiteMountain.h"
#include "ShinobiRope.h"
#include "ShinobiGrapplePoint.h"
#include "ShinobiTightrope.h"
#include "EscapeScoreComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UShinobiTraversalComponent::UShinobiTraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UShinobiTraversalComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogWhiteMountain, Error, TEXT("ShinobiTraversalComponent needs a Character owner"));
		SetComponentTickEnabled(false);
		return;
	}
	Movement = OwnerCharacter->GetCharacterMovement();
	Capsule = OwnerCharacter->GetCapsuleComponent();
	Score = OwnerCharacter->FindComponentByClass<UEscapeScoreComponent>();

	RopeLadders = StartingRopeLadders;
	Stamina = MaxStamina;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void UShinobiTraversalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!OwnerCharacter || !Movement)
	{
		return;
	}

	GrabCooldownRemaining = FMath::Max(0.f, GrabCooldownRemaining - DeltaTime);

	switch (State)
	{
	case EShinobiTraversalState::None:
		// Holding forward while falling next to a ledge grabs it.
		if (bAutoLedgeGrab && GrabCooldownRemaining <= 0.f && Movement->IsFalling() && MoveInput.Y > 0.1f)
		{
			TryGrabLedge();
		}
		break;
	case EShinobiTraversalState::Climbing:  TickClimbing(DeltaTime); break;
	case EShinobiTraversalState::LedgeHang: TickLedgeHang(DeltaTime); break;
	case EShinobiTraversalState::Mantling:  TickMantle(DeltaTime); break;
	case EShinobiTraversalState::OnRope:    TickRope(DeltaTime); break;
	case EShinobiTraversalState::Balancing: TickBalancing(DeltaTime); break;
	case EShinobiTraversalState::Deploying:
		Movement->StopMovementImmediately();
		break;
	}

	TickStamina(DeltaTime);
}

void UShinobiTraversalComponent::TickStamina(float DeltaTime)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bRestingOnGround = State == EShinobiTraversalState::None && Movement->IsMovingOnGround();
	if (bRestingOnGround && Now - LastStaminaDrainTime >= StaminaRegenDelay)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * DeltaTime);
	}
}

bool UShinobiTraversalComponent::DrainStamina(float Amount)
{
	LastStaminaDrainTime = GetWorld()->GetTimeSeconds();
	Stamina = FMath::Max(0.f, Stamina - Amount);
	return Stamina > 0.f;
}

bool UShinobiTraversalComponent::ConsumeSprintStamina(float DeltaTime)
{
	return DrainStamina(SprintStaminaPerSecond * DeltaTime);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

bool UShinobiTraversalComponent::HandleJumpPressed()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	switch (State)
	{
	case EShinobiTraversalState::None:
	{
		const FVector Loc = OwnerCharacter->GetActorLocation();
		const FVector Forward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
		const FVector Chest = Loc + FVector(0.f, 0.f, 20.f);
		FHitResult WallHit;
		if (!TraceWorld(Chest, Chest + Forward * (Radius() + ClimbStartDistance), WallHit) || !IsWall(WallHit))
		{
			return false;	// plain jump
		}

		// Low obstacle: vault over it.
		const float FeetZ = Loc.Z - HalfHeight();
		FVector Top;
		if (FindLedgeTop(WallHit.ImpactPoint, WallHit.ImpactNormal.GetSafeNormal2D(), FeetZ + Movement->MaxStepHeight, FeetZ + VaultMaxHeight, Top))
		{
			if (StartMantle(Top))
			{
				return true;
			}
		}

		if (IsClimbable(WallHit))
		{
			StartClimbing(WallHit);
			return true;
		}
		return false;
	}
	case EShinobiTraversalState::Climbing:
		WallJump();
		return true;
	case EShinobiTraversalState::LedgeHang:
		if (MoveInput.Y < -0.5f)
		{
			WallJump();
		}
		else
		{
			TryMantleOver(WallNormal);
		}
		return true;
	case EShinobiTraversalState::OnRope:
		JumpOffRope();
		return true;
	case EShinobiTraversalState::Balancing:
		// A small hop forward along the rope is not supported yet; jumping means leaving the rope.
		StopTraversal(0.4f);
		OwnerCharacter->LaunchCharacter(FVector(0.f, 0.f, 350.f), false, true);
		return true;
	default:
		return true;	// mantling / deploying swallow the press
	}
}

bool UShinobiTraversalComponent::HandleDropPressed()
{
	switch (State)
	{
	case EShinobiTraversalState::Climbing:
	case EShinobiTraversalState::LedgeHang:
	case EShinobiTraversalState::OnRope:
		StopTraversal(0.5f);
		return true;
	case EShinobiTraversalState::Balancing:
		FallOffTightrope();
		return true;
	default:
		return false;
	}
}

bool UShinobiTraversalComponent::HandleInteractPressed()
{
	if (State == EShinobiTraversalState::OnRope)
	{
		StopTraversal(0.5f);
		return true;
	}
	if (State == EShinobiTraversalState::Balancing)
	{
		FallOffTightrope();
		return true;
	}
	if (State != EShinobiTraversalState::None || !OwnerCharacter)
	{
		return false;
	}

	// Closest rope we can reach.
	const FVector Loc = OwnerCharacter->GetActorLocation();
	AShinobiRope* Best = nullptr;
	float BestDist = RopeGrabRadius;
	for (TActorIterator<AShinobiRope> It(GetWorld()); It; ++It)
	{
		AShinobiRope* Rope = *It;
		if (!IsValid(Rope) || Rope->IsRetracting())
		{
			continue;
		}
		const FVector Top = Rope->GetTop();
		if (Loc.Z < Rope->GetBottom().Z - 20.f || Loc.Z > Top.Z + HalfHeight() + 60.f)
		{
			continue;
		}
		const float Dist2D = FVector::Dist2D(Loc, Top);
		if (Dist2D < BestDist)
		{
			Best = Rope;
			BestDist = Dist2D;
		}
	}
	if (Best)
	{
		AttachToRope(Best);
		return true;
	}

	return TryMountTightrope();
}

bool UShinobiTraversalComponent::HandleRopeLadderPressed()
{
	if (State != EShinobiTraversalState::None || !Movement || !Movement->IsMovingOnGround())
	{
		return false;
	}

	const FVector Loc = OwnerCharacter->GetActorLocation();
	const float FeetZ = Loc.Z - HalfHeight();

	// Untie one of our own ladders if we stand at its top.
	for (int32 Index = DeployedLadders.Num() - 1; Index >= 0; --Index)
	{
		AShinobiRope* Ladder = DeployedLadders[Index].Get();
		if (!Ladder)
		{
			DeployedLadders.RemoveAt(Index);
			continue;
		}
		if (FVector::Dist2D(Loc, Ladder->GetTop()) < 200.f && FMath::Abs(FeetZ - Ladder->GetTop().Z) < 60.f)
		{
			Ladder->Destroy();
			DeployedLadders.RemoveAt(Index);
			++RopeLadders;
			Message(TEXT("You untied the rope ladder."));
			return true;
		}
	}

	if (!CheckUnlocked(EShinobiMove::RopeLadder))
	{
		return true;
	}
	if (RopeLadders <= 0)
	{
		Message(TEXT("No rope left for a ladder."));
		return true;
	}

	// Walk forward in 10 cm steps until the ground disappears: that is the edge.
	const FVector Forward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	float EdgeDistance = -1.f;
	for (float D = 0.f; D <= EdgeSearchDistance; D += 10.f)
	{
		const FVector P = Loc + Forward * D;
		FHitResult Ground;
		if (!TraceWorld(FVector(P.X, P.Y, FeetZ + 30.f), FVector(P.X, P.Y, FeetZ - 40.f), Ground))
		{
			EdgeDistance = D;
			break;
		}
	}
	if (EdgeDistance < 0.f)
	{
		Message(TEXT("Ladders are tied at an edge. Stand at the edge and face the drop."));
		return true;
	}

	const FVector EdgeXY = Loc + Forward * (EdgeDistance + 12.f);
	const FVector Top(EdgeXY.X, EdgeXY.Y, FeetZ);
	FHitResult Bottom;
	if (!TraceWorld(Top, Top - FVector(0.f, 0.f, MaxLadderLength), Bottom))
	{
		Message(TEXT("Too deep for your rope."));
		return true;
	}
	const float Drop = Top.Z - Bottom.ImpactPoint.Z;
	if (Drop < MinLadderDrop)
	{
		Message(TEXT("Too low for a ladder, just jump."));
		return true;
	}

	PendingLadderTop = Top;
	PendingLadderExit = -Forward;
	PendingLadderLength = Drop;
	EnterTraversalMovement(EShinobiTraversalState::Deploying);
	GetWorld()->GetTimerManager().SetTimer(DeployTimer, this, &UShinobiTraversalComponent::SpawnRopeLadder, LadderDeployTime, false);
	Message(TEXT("Tying the rope ladder..."));
	return true;
}

bool UShinobiTraversalComponent::HandleKaginawaPressed()
{
	if (State != EShinobiTraversalState::None || !OwnerCharacter)
	{
		return false;
	}
	if (!CheckUnlocked(EShinobiMove::Kaginawa))
	{
		return true;
	}

	AShinobiGrapplePoint* Target = FindKaginawaTarget();
	if (!Target)
	{
		Message(TEXT("No hook anchor in that direction."));
		return true;
	}

	PendingGrapplePoint = Target;
	OwnerCharacter->SetActorRotation(FRotator(0.f, (Target->GetActorLocation() - OwnerCharacter->GetActorLocation()).Rotation().Yaw, 0.f));
	EnterTraversalMovement(EShinobiTraversalState::Deploying);
	GetWorld()->GetTimerManager().SetTimer(DeployTimer, this, &UShinobiTraversalComponent::SpawnKaginawaRope, KaginawaThrowTime, false);
	return true;
}

// ---------------------------------------------------------------------------
// Shared state handling
// ---------------------------------------------------------------------------

void UShinobiTraversalComponent::EnterTraversalMovement(EShinobiTraversalState NewState)
{
	if (State == EShinobiTraversalState::None)
	{
		bSavedOrientToMovement = Movement->bOrientRotationToMovement;
		SavedMaxFlySpeed = Movement->MaxFlySpeed;
		SavedBrakingFlying = Movement->BrakingDecelerationFlying;
	}
	State = NewState;

	Movement->bOrientRotationToMovement = false;
	Movement->StopMovementImmediately();
	if (NewState != EShinobiTraversalState::Deploying)
	{
		Movement->SetMovementMode(MOVE_Flying);
	}
}

void UShinobiTraversalComponent::StopTraversal(float GrabCooldown)
{
	if (State == EShinobiTraversalState::None)
	{
		return;
	}
	State = EShinobiTraversalState::None;
	CurrentRope = nullptr;
	CurrentTightrope = nullptr;
	Balance = 0.f;
	BalanceVelocity = 0.f;
	GrabCooldownRemaining = GrabCooldown;

	Movement->bOrientRotationToMovement = bSavedOrientToMovement;
	Movement->MaxFlySpeed = SavedMaxFlySpeed;
	Movement->BrakingDecelerationFlying = SavedBrakingFlying;
	if (Movement->MovementMode == MOVE_Flying)
	{
		Movement->SetMovementMode(MOVE_Falling);
	}

	// Keep the body upright after facing walls.
	OwnerCharacter->SetActorRotation(FRotator(0.f, OwnerCharacter->GetActorRotation().Yaw, 0.f));
}

// ---------------------------------------------------------------------------
// Climbing
// ---------------------------------------------------------------------------

void UShinobiTraversalComponent::StartClimbing(const FHitResult& WallHit)
{
	if (!CheckUnlocked(EShinobiMove::Climb))
	{
		return;
	}
	if (Stamina <= 1.f)
	{
		Message(TEXT("Too tired to climb."));
		return;
	}
	WallNormal = WallHit.ImpactNormal.GetSafeNormal2D();
	EnterTraversalMovement(EShinobiTraversalState::Climbing);
	Movement->MaxFlySpeed = ClimbSpeed;
	Movement->BrakingDecelerationFlying = 4000.f;
	OwnerCharacter->SetActorRotation((-WallNormal).Rotation());
	Notify(EShinobiMove::Climb);
}

void UShinobiTraversalComponent::TickClimbing(float DeltaTime)
{
	const FVector Loc = OwnerCharacter->GetActorLocation();
	const FVector Into = -WallNormal;

	// Climbing costs stamina, more when moving.
	const float Drain = MoveInput.IsNearlyZero() ? HangStaminaPerSecond : ClimbStaminaPerSecond;
	if (!DrainStamina(Drain * DeltaTime))
	{
		Message(TEXT("Your grip gave out!"));
		StopTraversal(0.8f);
		return;
	}

	FHitResult Hit;
	if (!TraceWorld(Loc, Loc + Into * (Radius() + WallProbeDistance), Hit) || !IsClimbable(Hit))
	{
		// No wall at chest height any more: we are at the top, or the marked surface ended.
		if (!(MoveInput.Y > 0.1f && TryMantleOver(WallNormal)))
		{
			StopTraversal(0.3f);
		}
		return;
	}
	WallNormal = Hit.ImpactNormal.GetSafeNormal2D();

	// Stay glued to the wall at a fixed distance.
	const float PlaneDistance = FVector::DotProduct(Loc - Hit.ImpactPoint, WallNormal);
	const float WantedDistance = Radius() + WallOffset;
	const float NewDistance = FMath::FInterpTo(PlaneDistance, WantedDistance, DeltaTime, 15.f);
	OwnerCharacter->SetActorLocation(Loc + WallNormal * (NewDistance - PlaneDistance));
	OwnerCharacter->SetActorRotation(FMath::RInterpTo(OwnerCharacter->GetActorRotation(), (-WallNormal).Rotation(), DeltaTime, 12.f));

	const FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, WallNormal).GetSafeNormal();
	const FVector WallRight = FRotationMatrix((-WallNormal).Rotation()).GetUnitAxis(EAxis::Y);

	float Up = MoveInput.Y;
	float Side = MoveInput.X;

	// Head above the wall top: climbing further up means climbing over.
	const FVector Head = Loc + FVector(0.f, 0.f, HalfHeight() * 0.8f);
	FHitResult HeadHit;
	const bool bWallAtHead = TraceWorld(Head, Head + Into * (Radius() + WallProbeDistance), HeadHit) && IsClimbable(HeadHit);
	if (!bWallAtHead && Up > 0.1f)
	{
		if (TryMantleOver(WallNormal))
		{
			return;
		}
		Up = 0.f;
	}

	// Reached the ground while climbing down: step off.
	if (Up < -0.1f && HasFloorBelow(15.f))
	{
		StopTraversal(0.4f);
		return;
	}

	// Do not slide sideways off the marked surface.
	if (FMath::Abs(Side) > 0.1f)
	{
		const FVector SideStart = Loc + WallRight * FMath::Sign(Side) * Radius();
		FHitResult SideHit;
		if (!TraceWorld(SideStart, SideStart + Into * (Radius() + WallProbeDistance), SideHit) || !IsClimbable(SideHit))
		{
			Side = 0.f;
		}
	}

	OwnerCharacter->AddMovementInput(WallUp, Up);
	OwnerCharacter->AddMovementInput(WallRight, Side);
}

void UShinobiTraversalComponent::WallJump()
{
	if (!CheckUnlocked(EShinobiMove::WallJump))
	{
		return;
	}
	const FVector Away = WallNormal;
	StopTraversal(0.35f);
	OwnerCharacter->SetActorRotation(Away.Rotation());
	OwnerCharacter->LaunchCharacter(Away * WallJumpAway + FVector(0.f, 0.f, WallJumpUp), true, true);
	DrainStamina(10.f);
	Notify(EShinobiMove::WallJump);
}

// ---------------------------------------------------------------------------
// Ledge hang
// ---------------------------------------------------------------------------

bool UShinobiTraversalComponent::TryGrabLedge()
{
	if (Stamina <= 1.f)
	{
		return false;
	}

	const FVector Loc = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector Chest = Loc + FVector(0.f, 0.f, 20.f);

	FHitResult WallHit;
	if (!TraceWorld(Chest, Chest + Forward * (Radius() + LedgeGrabReach), WallHit) || !IsWall(WallHit) || HasTag(WallHit, NoClimbTag))
	{
		return false;
	}

	const FVector Normal = WallHit.ImpactNormal.GetSafeNormal2D();
	FVector Top;
	if (!FindLedgeTop(WallHit.ImpactPoint, Normal, Loc.Z + 10.f, Loc.Z + HalfHeight() + LedgeReachAbove, Top))
	{
		return false;
	}

	StartLedgeHang(WallHit.ImpactPoint, Normal, Top.Z);
	return true;
}

void UShinobiTraversalComponent::StartLedgeHang(const FVector& WallPoint, const FVector& Normal, float InLedgeZ)
{
	if (!CheckUnlocked(EShinobiMove::LedgeGrab))
	{
		return;
	}
	WallNormal = Normal;
	LedgeZ = InLedgeZ;
	EnterTraversalMovement(EShinobiTraversalState::LedgeHang);

	const FVector HangXY = WallPoint + WallNormal * (Radius() + WallOffset);
	OwnerCharacter->SetActorLocation(FVector(HangXY.X, HangXY.Y, LedgeZ - HalfHeight() - HangCapsuleTopBelowLedge));
	OwnerCharacter->SetActorRotation((-WallNormal).Rotation());
	Notify(EShinobiMove::LedgeGrab);
}

void UShinobiTraversalComponent::TickLedgeHang(float DeltaTime)
{
	Movement->StopMovementImmediately();

	if (!DrainStamina(HangStaminaPerSecond * DeltaTime))
	{
		Message(TEXT("Your grip gave out!"));
		StopTraversal(0.8f);
		return;
	}

	// Hold up to climb over.
	if (MoveInput.Y > 0.5f)
	{
		if (TryMantleOver(WallNormal))
		{
			return;
		}
	}

	// Hold down on a climbable wall to climb down its face.
	if (MoveInput.Y < -0.5f)
	{
		const FVector Loc = OwnerCharacter->GetActorLocation();
		FHitResult Hit;
		if (TraceWorld(Loc, Loc - WallNormal * (Radius() + WallProbeDistance), Hit) && IsClimbable(Hit))
		{
			State = EShinobiTraversalState::Climbing;
			Movement->MaxFlySpeed = ClimbSpeed;
			Movement->BrakingDecelerationFlying = 4000.f;
			Notify(EShinobiMove::Climb);
			return;
		}
	}

	// Shimmy along the ledge.
	if (FMath::Abs(MoveInput.X) > 0.1f)
	{
		const FVector Right = FRotationMatrix((-WallNormal).Rotation()).GetUnitAxis(EAxis::Y);
		const FVector Next = OwnerCharacter->GetActorLocation() + Right * FMath::Sign(MoveInput.X) * ShimmySpeed * DeltaTime;
		const FVector Probe = Next + FVector(0.f, 0.f, HalfHeight() * 0.5f);

		FHitResult WallHit;
		FVector Top;
		if (TraceWorld(Probe, Probe - WallNormal * (Radius() + WallProbeDistance), WallHit) && IsWall(WallHit)
			&& FindLedgeTop(WallHit.ImpactPoint, WallNormal, LedgeZ - 40.f, LedgeZ + 40.f, Top))
		{
			LedgeZ = Top.Z;
			WallNormal = WallHit.ImpactNormal.GetSafeNormal2D();
			const FVector HangXY = WallHit.ImpactPoint + WallNormal * (Radius() + WallOffset);
			OwnerCharacter->SetActorLocation(FVector(HangXY.X, HangXY.Y, LedgeZ - HalfHeight() - HangCapsuleTopBelowLedge));
		}
	}
}

// ---------------------------------------------------------------------------
// Mantle / vault
// ---------------------------------------------------------------------------

bool UShinobiTraversalComponent::TryMantleOver(const FVector& WallNormal2D)
{
	const FVector Loc = OwnerCharacter->GetActorLocation();
	FVector Top;
	// Look for a floor just behind the wall face, from above the head down to the knees.
	const FVector WallPoint = Loc - WallNormal2D * (Radius() + WallOffset);
	if (!FindLedgeTop(WallPoint, WallNormal2D, Loc.Z - HalfHeight() * 0.5f, Loc.Z + HalfHeight() + MantleReachAbove, Top))
	{
		return false;
	}
	return StartMantle(Top);
}

bool UShinobiTraversalComponent::StartMantle(const FVector& LedgeTop)
{
	const FVector Target = LedgeTop + FVector(0.f, 0.f, HalfHeight() + 2.f);
	if (!CapsuleFitsAt(Target))
	{
		return false;
	}

	const bool bWasOnRope = State == EShinobiTraversalState::OnRope;
	EnterTraversalMovement(EShinobiTraversalState::Mantling);
	MantleStart = OwnerCharacter->GetActorLocation();
	MantleTarget = Target;
	MantleAlpha = 0.f;

	const FVector Flat = (MantleTarget - MantleStart).GetSafeNormal2D();
	if (!Flat.IsNearlyZero())
	{
		OwnerCharacter->SetActorRotation(Flat.Rotation());
	}
	if (!bWasOnRope)
	{
		Notify(EShinobiMove::Mantle);
	}
	return true;
}

void UShinobiTraversalComponent::TickMantle(float DeltaTime)
{
	Movement->StopMovementImmediately();
	MantleAlpha = FMath::Min(1.f, MantleAlpha + DeltaTime / FMath::Max(0.05f, MantleDuration));

	// Up first, then forward, so the capsule never cuts through the ledge corner.
	constexpr float UpPhase = 0.6f;
	FVector P;
	if (MantleAlpha < UpPhase)
	{
		const float A = FMath::InterpEaseOut(0.f, 1.f, MantleAlpha / UpPhase, 2.f);
		P = FVector(MantleStart.X, MantleStart.Y, FMath::Lerp(MantleStart.Z, MantleTarget.Z, A));
	}
	else
	{
		const float A = FMath::InterpEaseInOut(0.f, 1.f, (MantleAlpha - UpPhase) / (1.f - UpPhase), 2.f);
		P = FVector(FMath::Lerp(MantleStart.X, MantleTarget.X, A), FMath::Lerp(MantleStart.Y, MantleTarget.Y, A), MantleTarget.Z);
	}
	OwnerCharacter->SetActorLocation(P);

	if (MantleAlpha >= 1.f)
	{
		StopTraversal();
		Movement->SetMovementMode(MOVE_Walking);
		if (AShinobiRope* Rope = RopeToRetractAfterMantle.Get())
		{
			Rope->Retract(KaginawaRetractDelay);
			Message(TEXT("You pulled the rope up behind you."));
		}
		RopeToRetractAfterMantle.Reset();
	}
}

// ---------------------------------------------------------------------------
// Rope and rope ladder
// ---------------------------------------------------------------------------

void UShinobiTraversalComponent::AttachToRope(AShinobiRope* Rope)
{
	if (!CheckUnlocked(Rope->bIsLadder ? EShinobiMove::RopeLadder : EShinobiMove::Rope))
	{
		return;
	}

	const FVector Loc = OwnerCharacter->GetActorLocation();
	CurrentRope = Rope;
	EnterTraversalMovement(EShinobiTraversalState::OnRope);

	RopeDistance = FMath::Clamp(Rope->GetTop().Z - Loc.Z, HalfHeight() * 0.5f, FMath::Max(HalfHeight() * 0.5f, Rope->Length - HalfHeight()));
	if (Rope->bTopExit)
	{
		// Against a wall: always face the wall the rope hangs on.
		RopeFacing = Rope->GetExitDirection();
	}
	else
	{
		RopeFacing = (Rope->GetTop() - Loc).GetSafeNormal2D();
		if (RopeFacing.IsNearlyZero())
		{
			RopeFacing = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
		}
	}
	UpdateRopePosition();
	Notify(Rope->bIsLadder ? EShinobiMove::RopeLadder : EShinobiMove::Rope);
}

void UShinobiTraversalComponent::UpdateRopePosition()
{
	const FVector RopePoint = CurrentRope->GetPointAtDistance(RopeDistance);
	OwnerCharacter->SetActorLocation(RopePoint - RopeFacing * (Radius() + RopeHoldOffset));
	OwnerCharacter->SetActorRotation(RopeFacing.Rotation());
}

void UShinobiTraversalComponent::TickRope(float DeltaTime)
{
	Movement->StopMovementImmediately();
	if (!IsValid(CurrentRope) || CurrentRope->IsRetracting())
	{
		StopTraversal(0.5f);
		return;
	}

	if (!MoveInput.IsNearlyZero() && !CurrentRope->bIsLadder)
	{
		if (!DrainStamina(RopeStaminaPerSecond * DeltaTime))
		{
			Message(TEXT("Your grip gave out!"));
			StopTraversal(0.8f);
			return;
		}
	}

	const float Speed = RopeClimbSpeed * CurrentRope->GetClimbSpeedMultiplier();
	RopeDistance -= MoveInput.Y * Speed * DeltaTime;

	const float MinDistance = HalfHeight() * 0.5f;
	const float MaxDistance = FMath::Max(MinDistance, CurrentRope->Length - HalfHeight());

	if (RopeDistance <= MinDistance)
	{
		RopeDistance = MinDistance;
		if (MoveInput.Y > 0.5f && CurrentRope->bTopExit && TryExitRopeTop())
		{
			return;
		}
	}
	if (RopeDistance >= MaxDistance)
	{
		RopeDistance = MaxDistance;
		if (MoveInput.Y < -0.5f)
		{
			StopTraversal(0.5f);	// feet on the ground (or let go of a short rope)
			return;
		}
	}

	if (!CurrentRope->bTopExit && FMath::Abs(MoveInput.X) > 0.1f)
	{
		RopeFacing = RopeFacing.RotateAngleAxis(-MoveInput.X * RopeOrbitSpeed * DeltaTime, FVector::UpVector);
	}
	UpdateRopePosition();
}

bool UShinobiTraversalComponent::TryExitRopeTop()
{
	const FVector Top = CurrentRope->GetTop();
	const FVector Exit = CurrentRope->GetExitDirection();
	const FVector Start = Top + Exit * (Radius() + 40.f) + FVector(0.f, 0.f, 100.f);
	FHitResult Floor;
	if (!TraceWorld(Start, Start - FVector(0.f, 0.f, 160.f), Floor) || Floor.ImpactNormal.Z < WalkableNormalZ)
	{
		return false;
	}

	AShinobiRope* Rope = CurrentRope;
	if (!StartMantle(Floor.ImpactPoint))
	{
		return false;
	}
	if (Rope->bRetractWhenLeft)
	{
		RopeToRetractAfterMantle = Rope;
	}
	return true;
}

void UShinobiTraversalComponent::JumpOffRope()
{
	const FVector Away = -RopeFacing;
	StopTraversal(0.5f);
	OwnerCharacter->SetActorRotation(Away.Rotation());
	OwnerCharacter->LaunchCharacter(Away * 350.f + FVector(0.f, 0.f, 400.f), true, true);
}

void UShinobiTraversalComponent::SpawnRopeLadder()
{
	State = EShinobiTraversalState::None;	// leave Deploying without touching movement
	Movement->bOrientRotationToMovement = bSavedOrientToMovement;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AShinobiRope* Ladder = GetWorld()->SpawnActor<AShinobiRope>(AShinobiRope::StaticClass(), PendingLadderTop, PendingLadderExit.Rotation(), Params);
	if (!Ladder)
	{
		return;
	}
	Ladder->SetupRope(PendingLadderLength, true, true);
	Ladder->bDeployedByPlayer = true;
	DeployedLadders.Add(Ladder);
	--RopeLadders;

	Message(FString::Printf(TEXT("Ladder placed (%.0f m). Ropes left: %d"), PendingLadderLength / 100.f, RopeLadders));
	AttachToRope(Ladder);
}

AShinobiGrapplePoint* UShinobiTraversalComponent::FindKaginawaTarget() const
{
	const APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return nullptr;
	}
	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CamDir = PC->PlayerCameraManager->GetCameraRotation().Vector();
	const FVector Loc = OwnerCharacter->GetActorLocation();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(KaginawaAimAngle));

	AShinobiGrapplePoint* Best = nullptr;
	float BestDot = MinDot;
	for (TActorIterator<AShinobiGrapplePoint> It(GetWorld()); It; ++It)
	{
		AShinobiGrapplePoint* Point = *It;
		const FVector P = Point->GetActorLocation();
		if (FVector::Dist(P, Loc) > KaginawaRange || P.Z < Loc.Z + 100.f)
		{
			continue;
		}
		const float Dot = FVector::DotProduct(CamDir, (P - CamLoc).GetSafeNormal());
		if (Dot < BestDot)
		{
			continue;
		}

		// Line of sight; hitting the anchor's own edge is fine.
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(KaginawaSight), false, OwnerCharacter);
		Params.AddIgnoredActor(Point);
		if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, P, ECC_Visibility, Params) && FVector::Dist(Hit.ImpactPoint, P) > 60.f)
		{
			continue;
		}
		Best = Point;
		BestDot = Dot;
	}
	return Best;
}

void UShinobiTraversalComponent::SpawnKaginawaRope()
{
	State = EShinobiTraversalState::None;
	Movement->bOrientRotationToMovement = bSavedOrientToMovement;

	AShinobiGrapplePoint* Point = PendingGrapplePoint.Get();
	PendingGrapplePoint.Reset();
	if (!Point)
	{
		return;
	}
	if (AShinobiRope* Old = Point->ActiveRope.Get())
	{
		Old->Destroy();
	}

	const FVector Exit = Point->GetActorForwardVector().GetSafeNormal2D();
	const FVector Top = Point->GetActorLocation() - Exit * 15.f;
	FHitResult Ground;
	const float Length = TraceWorld(Top, Top - FVector(0.f, 0.f, Point->MaxRopeLength), Ground)
		? Top.Z - Ground.ImpactPoint.Z
		: Point->MaxRopeLength;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AShinobiRope* Rope = GetWorld()->SpawnActor<AShinobiRope>(AShinobiRope::StaticClass(), Top, Exit.Rotation(), Params);
	if (!Rope)
	{
		return;
	}
	Rope->SetupRope(Length, false, true);
	Rope->bRetractWhenLeft = true;
	Point->ActiveRope = Rope;
	Notify(EShinobiMove::Kaginawa);
	Message(TEXT("The hook caught."));

	// Close enough: grab it straight away.
	if (FVector::Dist2D(OwnerCharacter->GetActorLocation(), Top) < RopeGrabRadius * 2.f)
	{
		AttachToRope(Rope);
	}
}

// ---------------------------------------------------------------------------
// Tightrope
// ---------------------------------------------------------------------------

bool UShinobiTraversalComponent::TryMountTightrope()
{
	const FVector Loc = OwnerCharacter->GetActorLocation();
	const FVector Feet = Loc - FVector(0.f, 0.f, HalfHeight());
	for (TActorIterator<AShinobiTightrope> It(GetWorld()); It; ++It)
	{
		AShinobiTightrope* Rope = *It;
		const FVector Ends[2] = { Rope->GetStart(), Rope->GetEnd() };
		for (int32 EndIndex = 0; EndIndex < 2; ++EndIndex)
		{
			if (FVector::Dist2D(Feet, Ends[EndIndex]) < TightropeMountRadius && FMath::Abs(Feet.Z - Ends[EndIndex].Z) < 80.f)
			{
				if (!CheckUnlocked(EShinobiMove::Tightrope))
				{
					return true;
				}
				CurrentTightrope = Rope;
				TightropeDirection = EndIndex == 0 ? 1.f : -1.f;
				TightropeDistance = EndIndex == 0 ? 0.f : Rope->GetLength();
				Balance = 0.f;
				BalanceVelocity = 0.f;
				EnterTraversalMovement(EShinobiTraversalState::Balancing);
				Notify(EShinobiMove::Tightrope);
				Message(TEXT("On the tightrope. W forward, A/D keep balance."));
				return true;
			}
		}
	}
	return false;
}

void UShinobiTraversalComponent::TickBalancing(float DeltaTime)
{
	Movement->StopMovementImmediately();
	if (!IsValid(CurrentTightrope))
	{
		StopTraversal(0.5f);
		return;
	}

	const float Length = CurrentTightrope->GetLength();
	const float Step = MoveInput.Y * TightropeSpeed * DeltaTime;
	TightropeDistance += Step * TightropeDirection;

	// The faster you walk the more you wobble. A/D push back towards the centre.
	const float SpeedFactor = 0.35f + FMath::Abs(MoveInput.Y);
	const float Noise = FMath::FRandRange(-1.f, 1.f) * BalanceDrift * SpeedFactor;
	BalanceVelocity += (Noise + Balance * 0.8f - MoveInput.X * BalanceCorrection) * DeltaTime;
	BalanceVelocity *= FMath::Max(0.f, 1.f - 1.5f * DeltaTime);
	Balance += BalanceVelocity * DeltaTime * 2.f;

	if (FMath::Abs(Balance) >= 1.f)
	{
		FallOffTightrope();
		return;
	}

	// Walked off one end: step onto whatever is there.
	if (TightropeDistance < -10.f || TightropeDistance > Length + 10.f)
	{
		const FVector Dir = (CurrentTightrope->GetEnd() - CurrentTightrope->GetStart()).GetSafeNormal2D() * TightropeDirection;
		StopTraversal(0.3f);
		OwnerCharacter->LaunchCharacter(Dir * 200.f, true, false);
		return;
	}

	const FVector Along = (CurrentTightrope->GetEnd() - CurrentTightrope->GetStart()).GetSafeNormal() * TightropeDirection;
	const FVector Point = CurrentTightrope->GetPointAt(FMath::Clamp(TightropeDistance, 0.f, Length));
	const FVector SideLean = FVector::CrossProduct(FVector::UpVector, Along).GetSafeNormal() * Balance * 8.f;
	OwnerCharacter->SetActorLocation(Point + FVector(0.f, 0.f, HalfHeight() + 2.f) + SideLean);
	OwnerCharacter->SetActorRotation(FRotator(0.f, Along.Rotation().Yaw, Balance * 25.f));
}

void UShinobiTraversalComponent::FallOffTightrope()
{
	const float Side = FMath::Sign(Balance == 0.f ? 1.f : Balance);
	FVector Along = FVector::ForwardVector;
	if (IsValid(CurrentTightrope))
	{
		Along = (CurrentTightrope->GetEnd() - CurrentTightrope->GetStart()).GetSafeNormal2D() * TightropeDirection;
	}
	const FVector SideDir = FVector::CrossProduct(FVector::UpVector, Along).GetSafeNormal() * Side;
	StopTraversal(0.6f);
	OwnerCharacter->LaunchCharacter(SideDir * 250.f, true, false);
	Message(TEXT("You lost your balance!"));
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

float UShinobiTraversalComponent::Radius() const
{
	return Capsule ? Capsule->GetScaledCapsuleRadius() : 42.f;
}

float UShinobiTraversalComponent::HalfHeight() const
{
	return Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 92.f;
}

bool UShinobiTraversalComponent::TraceWorld(const FVector& Start, const FVector& End, FHitResult& OutHit) const
{
	// Only level geometry counts, never pawns or ropes.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ShinobiTraversal), false, GetOwner());

	const bool bHit = GetWorld()->LineTraceSingleByObjectType(OutHit, Start, End, ObjectParams, Params);
#if ENABLE_DRAW_DEBUG
	if (bDebugDraw)
	{
		DrawDebugLine(GetWorld(), Start, bHit ? OutHit.ImpactPoint : End, bHit ? FColor::Green : FColor::Red, false, 0.f, 0, 1.f);
	}
#endif
	return bHit;
}

bool UShinobiTraversalComponent::HasTag(const FHitResult& Hit, FName Tag) const
{
	const AActor* HitActor = Hit.GetActor();
	const UPrimitiveComponent* HitComponent = Hit.GetComponent();
	return (HitActor && HitActor->ActorHasTag(Tag)) || (HitComponent && HitComponent->ComponentHasTag(Tag));
}

bool UShinobiTraversalComponent::IsWall(const FHitResult& Hit) const
{
	return Hit.bBlockingHit && FMath::Abs(Hit.ImpactNormal.Z) <= MaxClimbableNormalZ;
}

bool UShinobiTraversalComponent::IsClimbable(const FHitResult& Hit) const
{
	if (!IsWall(Hit) || HasTag(Hit, NoClimbTag))
	{
		return false;
	}
	return !bRequireClimbableTag || HasTag(Hit, ClimbableTag);
}

bool UShinobiTraversalComponent::FindLedgeTop(const FVector& WallPoint, const FVector& WallNormal2D, float MinZ, float MaxZ, FVector& OutTop) const
{
	// Trace down just behind the wall face. If the wall continues above MaxZ the trace starts inside it and finds nothing.
	const FVector Behind = WallPoint - WallNormal2D * LedgeDepth;
	FHitResult Hit;
	if (!TraceWorld(FVector(Behind.X, Behind.Y, MaxZ), FVector(Behind.X, Behind.Y, MinZ), Hit))
	{
		return false;
	}
	if (Hit.bStartPenetrating || Hit.ImpactNormal.Z < WalkableNormalZ)
	{
		return false;
	}
	OutTop = Hit.ImpactPoint;
	return true;
}

bool UShinobiTraversalComponent::CapsuleFitsAt(const FVector& Location) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ShinobiFit), false, GetOwner());
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius() - 2.f, HalfHeight() - 2.f);
	return !GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, Shape, Params);
}

bool UShinobiTraversalComponent::HasFloorBelow(float MaxDistance) const
{
	const FVector Feet = OwnerCharacter->GetActorLocation() - FVector(0.f, 0.f, HalfHeight());
	FHitResult Hit;
	return TraceWorld(Feet + FVector(0.f, 0.f, 5.f), Feet - FVector(0.f, 0.f, MaxDistance), Hit) && Hit.ImpactNormal.Z >= WalkableNormalZ;
}

// ---------------------------------------------------------------------------
// Score / progression glue
// ---------------------------------------------------------------------------

bool UShinobiTraversalComponent::CheckUnlocked(EShinobiMove Move)
{
	if (!Score || Score->IsMoveUnlocked(Move))
	{
		return true;
	}
	Message(TEXT("You have not learned that move yet. Earn escape points and visit a village master."));
	return false;
}

void UShinobiTraversalComponent::Notify(EShinobiMove Move)
{
	if (Score)
	{
		Score->NotifyTraversal(Move);
	}
}

void UShinobiTraversalComponent::Message(const FString& Text)
{
	if (Score)
	{
		Score->PushMessage(Text);
	}
	UE_LOG(LogWhiteMountain, Verbose, TEXT("%s"), *Text);
}

FString UShinobiTraversalComponent::GetStateDisplayName() const
{
	switch (State)
	{
	case EShinobiTraversalState::None:      return Movement && Movement->IsFalling() ? TEXT("In the air") : TEXT("On the ground");
	case EShinobiTraversalState::Climbing:  return TEXT("Climbing");
	case EShinobiTraversalState::LedgeHang: return TEXT("Hanging on a ledge");
	case EShinobiTraversalState::Mantling:  return TEXT("Climbing over");
	case EShinobiTraversalState::OnRope:    return CurrentRope && CurrentRope->bIsLadder ? TEXT("On a ladder") : TEXT("On a rope");
	case EShinobiTraversalState::Balancing: return TEXT("Balancing");
	case EShinobiTraversalState::Deploying: return TEXT("Preparing rope");
	}
	return TEXT("");
}
