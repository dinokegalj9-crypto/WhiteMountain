#include "ShinobiEnemyController.h"
#include "WhiteMountain.h"
#include "ShinobiEnemy.h"
#include "ShinobiCharacter.h"
#include "EscapeScoreComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

AShinobiEnemyController::AShinobiEnemyController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AShinobiEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	HomeLocation = InPawn->GetActorLocation();
	EnterState(EEnemyAlert::Patrol);
}

AShinobiEnemy* AShinobiEnemyController::GetEnemy() const
{
	return Cast<AShinobiEnemy>(GetPawn());
}

APawn* AShinobiEnemyController::GetPlayer() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

UEscapeScoreComponent* AShinobiEnemyController::GetPlayerScore() const
{
	const APawn* Player = GetPlayer();
	return Player ? Player->FindComponentByClass<UEscapeScoreComponent>() : nullptr;
}

bool AShinobiEnemyController::IsIdle() const
{
	return GetMoveStatus() == EPathFollowingStatus::Idle;
}

// ---------------------------------------------------------------------------

void AShinobiEnemyController::EnterState(EEnemyAlert NewState)
{
	AShinobiEnemy* Enemy = GetEnemy();
	if (!Enemy)
	{
		return;
	}
	UCharacterMovementComponent* MoveComp = Enemy->GetCharacterMovement();

	Alert = NewState;
	StateTime = 0.f;

	switch (NewState)
	{
	case EEnemyAlert::Patrol:
		MoveComp->MaxWalkSpeed = Enemy->PatrolSpeed;
		WaitTimer = FMath::FRandRange(0.5f, 2.f);
		StopMovement();
		break;

	case EEnemyAlert::Chase:
		MoveComp->MaxWalkSpeed = Enemy->ChaseSpeed;
		UnreachableTime = 0.f;
		LostSightTime = 0.f;
		RepathTimer = 0.f;
		if (UEscapeScoreComponent* Score = GetPlayerScore())
		{
			Score->ReportPursuitStarted(Enemy);
		}
		break;

	case EEnemyAlert::Search:
		MoveComp->MaxWalkSpeed = Enemy->PatrolSpeed * 1.5f;
		MoveToLocation(LastSeenLocation, 80.f);
		break;

	case EEnemyAlert::Frustrated:
		StopMovement();
		break;

	case EEnemyAlert::Return:
		MoveComp->MaxWalkSpeed = Enemy->PatrolSpeed;
		MoveToLocation(HomeLocation, 100.f);
		break;
	}
}

void AShinobiEnemyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetEnemy())
	{
		return;
	}

	StateTime += DeltaSeconds;
	CatchTimer = FMath::Max(0.f, CatchTimer - DeltaSeconds);

	switch (Alert)
	{
	case EEnemyAlert::Patrol:     TickPatrol(DeltaSeconds); break;
	case EEnemyAlert::Chase:      TickChase(DeltaSeconds); break;
	case EEnemyAlert::Search:     TickSearch(DeltaSeconds); break;
	case EEnemyAlert::Frustrated: TickFrustrated(DeltaSeconds); break;
	case EEnemyAlert::Return:     TickReturn(DeltaSeconds); break;
	}
}

// ---------------------------------------------------------------------------

void AShinobiEnemyController::TickPatrol(float DeltaSeconds)
{
	if (CanSeePlayer())
	{
		LastSeenLocation = GetPlayer()->GetActorLocation();
		EnterState(EEnemyAlert::Chase);
		return;
	}
	if (IsIdle())
	{
		WaitTimer -= DeltaSeconds;
		if (WaitTimer <= 0.f)
		{
			MoveToRandomPointNear(HomeLocation, PatrolRadius);
			WaitTimer = FMath::FRandRange(2.f, 4.f);
		}
	}
}

void AShinobiEnemyController::TickChase(float DeltaSeconds)
{
	APawn* Player = GetPlayer();
	if (!Player)
	{
		EnterState(EEnemyAlert::Return);
		return;
	}

	const bool bSees = CanSeePlayer();
	if (bSees)
	{
		LastSeenLocation = Player->GetActorLocation();
		LostSightTime = 0.f;
	}
	else
	{
		LostSightTime += DeltaSeconds;
	}

	RepathTimer -= DeltaSeconds;
	if (RepathTimer <= 0.f)
	{
		RepathTimer = RepathInterval;
		LastRoute = CheckRoute(Player);
		if (LastRoute.bReachable && !LastRoute.bDetour)
		{
			if (bSees)
			{
				MoveToActor(Player, 60.f);
			}
			else
			{
				MoveToLocation(LastSeenLocation, 60.f);
			}
		}
		else
		{
			// Get as close as the ground allows and stare up.
			MoveToLocation(LastRoute.End, 50.f);
		}
	}

	// Only count "no route" while the guard still knows where the shinobi is.
	const bool bKnowsWhere = bSees || LostSightTime < 2.f;
	if ((!LastRoute.bReachable || LastRoute.bDetour) && bKnowsWhere)
	{
		UnreachableTime += DeltaSeconds;
	}
	else
	{
		UnreachableTime = 0.f;
	}

	TryCatch();

	if (UnreachableTime >= UnreachableGiveUpTime)
	{
		PendingReason = LastRoute.bDetour ? EEscapeReason::Detour : EEscapeReason::Unreachable;
		EnterState(EEnemyAlert::Frustrated);
		return;
	}

	const AShinobiEnemy* Enemy = GetEnemy();
	if (LostSightTime >= Enemy->LostSightGiveUpTime)
	{
		EnterState(EEnemyAlert::Search);
	}
}

void AShinobiEnemyController::TickSearch(float DeltaSeconds)
{
	if (CanSeePlayer())
	{
		LastSeenLocation = GetPlayer()->GetActorLocation();
		EnterState(EEnemyAlert::Chase);
		return;
	}
	if (IsIdle())
	{
		MoveToRandomPointNear(LastSeenLocation, 500.f);
	}
	if (StateTime >= GetEnemy()->SearchDuration)
	{
		if (UEscapeScoreComponent* Score = GetPlayerScore())
		{
			Score->ReportEscape(GetEnemy(), EEscapeReason::LostTrail, GetEnemy()->EscapePointMultiplier);
		}
		EnterState(EEnemyAlert::Return);
	}
}

void AShinobiEnemyController::TickFrustrated(float DeltaSeconds)
{
	APawn* Player = GetPlayer();
	if (Player)
	{
		FaceTowards(Player->GetActorLocation(), DeltaSeconds);
	}

	// The shinobi came back down within reach: the chase is on again, no points.
	RepathTimer -= DeltaSeconds;
	if (Player && RepathTimer <= 0.f)
	{
		RepathTimer = 0.5f;
		const FShinobiRouteCheck Route = CheckRoute(Player);
		if (Route.bReachable && !Route.bDetour && CanSeePlayer())
		{
			EnterState(EEnemyAlert::Chase);
			return;
		}
	}

	if (StateTime >= FrustratedDuration)
	{
		if (UEscapeScoreComponent* Score = GetPlayerScore())
		{
			Score->ReportEscape(GetEnemy(), PendingReason, GetEnemy()->EscapePointMultiplier);
		}
		EnterState(EEnemyAlert::Return);
	}
}

void AShinobiEnemyController::TickReturn(float DeltaSeconds)
{
	if (CanSeePlayer())
	{
		LastSeenLocation = GetPlayer()->GetActorLocation();
		EnterState(EEnemyAlert::Chase);
		return;
	}
	if (FVector::Dist2D(GetEnemy()->GetActorLocation(), HomeLocation) < 150.f)
	{
		EnterState(EEnemyAlert::Patrol);
		return;
	}
	if (IsIdle() && StateTime > 1.f)
	{
		MoveToLocation(HomeLocation, 100.f);
		StateTime = 0.f;
	}
}

// ---------------------------------------------------------------------------

bool AShinobiEnemyController::CanSeePlayer() const
{
	const AShinobiEnemy* Enemy = GetEnemy();
	const APawn* Player = GetPlayer();
	if (!Enemy || !Player)
	{
		return false;
	}

	const FVector ToPlayer = Player->GetActorLocation() - Enemy->GetActorLocation();
	const float Distance = ToPlayer.Size();
	if (Distance > Enemy->SightRange || Distance < KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const bool bAlert = Alert != EEnemyAlert::Patrol;
	const bool bCloseAndAlert = bAlert && Distance < Enemy->CloseAwarenessRadius;
	if (!bCloseAndAlert)
	{
		const float CosAngle = FVector::DotProduct(Enemy->GetActorForwardVector(), ToPlayer / Distance);
		if (CosAngle < FMath::Cos(FMath::DegreesToRadians(Enemy->SightHalfAngle)))
		{
			return false;
		}
	}

	// Calm guards miss a crouching shinobi at range.
	const ACharacter* PlayerCharacter = Cast<ACharacter>(Player);
	if (!bAlert && PlayerCharacter && PlayerCharacter->bIsCrouched && Distance > Enemy->CrouchHiddenDistance)
	{
		return false;
	}

	return LineOfSightTo(Player);
}

FShinobiRouteCheck AShinobiEnemyController::CheckRoute(const APawn* Target) const
{
	FShinobiRouteCheck Result;
	Result.End = LastSeenLocation;

	const APawn* Me = GetPawn();
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Me || !Target || !Nav)
	{
		return Result;
	}

	const ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	const float TargetHalfHeight = TargetCharacter ? TargetCharacter->GetSimpleCollisionHalfHeight() : 90.f;
	const FVector TargetFeet = Target->GetActorLocation() - FVector(0.f, 0.f, TargetHalfHeight);

	UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), Me->GetActorLocation(), Target->GetActorLocation(), const_cast<APawn*>(Me));
	if (!Path || !Path->IsValid() || Path->PathPoints.Num() == 0)
	{
		return Result;
	}

	Result.End = Path->PathPoints.Last();
	// A climbing or hanging shinobi projects onto the ground below him, so check the gap to his feet too.
	Result.bReachable = !Path->IsPartial() && FVector::Dist(Result.End, TargetFeet) < 150.f;
	if (Result.bReachable)
	{
		const float PathLength = Path->GetPathLength();
		const float Straight = FVector::Dist(Me->GetActorLocation(), Target->GetActorLocation());
		Result.bDetour = PathLength > DetourMinLength && PathLength > Straight * DetourRatio;
	}
	return Result;
}

void AShinobiEnemyController::MoveToRandomPointNear(const FVector& Center, float Radius)
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Point;
	if (Nav && Nav->GetRandomReachablePointInRadius(Center, Radius, Point))
	{
		MoveToLocation(Point.Location, 50.f);
	}
}

void AShinobiEnemyController::FaceTowards(const FVector& Location, float DeltaSeconds)
{
	APawn* Me = GetPawn();
	const FVector Flat = (Location - Me->GetActorLocation()).GetSafeNormal2D();
	if (!Flat.IsNearlyZero())
	{
		Me->SetActorRotation(FMath::RInterpTo(Me->GetActorRotation(), Flat.Rotation(), DeltaSeconds, 6.f));
	}
}

void AShinobiEnemyController::TryCatch()
{
	APawn* Player = GetPlayer();
	const APawn* Me = GetPawn();
	if (!Player || !Me || CatchTimer > 0.f)
	{
		return;
	}
	const FVector Delta = Player->GetActorLocation() - Me->GetActorLocation();
	if (Delta.Size2D() < CatchRadius && FMath::Abs(Delta.Z) < 120.f)
	{
		CatchTimer = CatchCooldown;
		if (UEscapeScoreComponent* Score = GetPlayerScore())
		{
			Score->ReportCaught(GetEnemy());
		}
		if (AShinobiCharacter* Shinobi = Cast<AShinobiCharacter>(Player))
		{
			Shinobi->ReceiveCatch(Me->GetActorLocation());
		}
	}
}
