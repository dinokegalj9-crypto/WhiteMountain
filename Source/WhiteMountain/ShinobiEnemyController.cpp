#include "ShinobiEnemyController.h"
#include "WhiteMountain.h"
#include "ShinobiEnemy.h"
#include "ShinobiCharacter.h"
#include "EscapeScoreComponent.h"
#include "ShinobiArrow.h"
#include "ShinobiEscapeRoute.h"
#include "ShinobiTraversalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
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

	CatchTimer = FMath::Max(0.f, CatchTimer - DeltaSeconds);
	ShotTimer = FMath::Max(0.f, ShotTimer - DeltaSeconds);
	if (GetEnemy()->IsStunned())
	{
		bAiming = false;
		return;	// on the ground: no seeing, chasing or catching until he gets up
	}
	StateTime += DeltaSeconds;

	switch (Alert)
	{
	case EEnemyAlert::Patrol:     TickPatrol(DeltaSeconds); break;
	case EEnemyAlert::Chase:      TickChase(DeltaSeconds); break;
	case EEnemyAlert::Search:     TickSearch(DeltaSeconds); break;
	case EEnemyAlert::Frustrated: TickFrustrated(DeltaSeconds); break;
	case EEnemyAlert::Return:     TickReturn(DeltaSeconds); break;
	}
}

void AShinobiEnemyController::RaiseAlarm(const FVector& Location)
{
	LastSeenLocation = Location;
	if (Alert != EEnemyAlert::Chase)
	{
		EnterState(EEnemyAlert::Chase);
	}
	LostSightTime = 0.f;
	RepathTimer = 0.f;
	if (GetEnemy() && GetEnemy()->IsStunned())
	{
		GetEnemy()->GetCharacterMovement()->MaxWalkSpeed = 0.f;	// Recover() restores the chase speed
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

	if (TickBow(DeltaSeconds, bSees))
	{
		return;	// standing still, drawing the bow
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

	// He cannot follow you up there, but an archer can still shoot.
	TickBow(DeltaSeconds, CanSeePlayer());

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

// ---------------------------------------------------------------------------
// Bow
// ---------------------------------------------------------------------------

bool AShinobiEnemyController::TickBow(float DeltaSeconds, bool bSees)
{
	const AShinobiEnemy* Enemy = GetEnemy();
	APawn* Player = GetPlayer();
	if (!Enemy || !Enemy->bCanShoot || !Player)
	{
		bAiming = false;
		return false;
	}

	if (bAiming)
	{
		FaceTowards(Player->GetActorLocation(), DeltaSeconds);
		AimTimer += DeltaSeconds;
		if (AimTimer >= Enemy->AimTime)
		{
			bAiming = false;
			ShotTimer = Enemy->ShotCooldown;
			if (bSees)
			{
				Shoot();
			}
		}
		return bAiming;
	}

	const float Distance = FVector::Dist(Player->GetActorLocation(), Enemy->GetActorLocation());
	if (bSees && ShotTimer <= 0.f && Distance >= Enemy->MinShootDistance && Distance <= Enemy->SightRange)
	{
		bAiming = true;
		AimTimer = 0.f;
		StopMovement();
		return true;
	}
	return false;
}

float AShinobiEnemyController::GetHitChance() const
{
	const AShinobiEnemy* Enemy = GetEnemy();
	const ACharacter* Player = Cast<ACharacter>(GetPlayer());
	if (!Enemy || !Player)
	{
		return 0.f;
	}

	float Chance = Enemy->BaseHitChance;

	// Past 10 m every extra 10 m costs a third of the chance, down to 30 %.
	const float Distance = FVector::Dist(Player->GetActorLocation(), Enemy->GetActorLocation());
	Chance *= FMath::Clamp(1.f - (Distance - 1000.f) / 3000.f, 0.3f, 1.f);

	if (Player->GetVelocity().Size2D() > 550.f)
	{
		Chance -= 0.15f;	// sprinting
	}
	if (Player->bIsCrouched)
	{
		Chance -= 0.1f;
	}
	const UShinobiTraversalComponent* Traversal = Player->FindComponentByClass<UShinobiTraversalComponent>();
	if (Traversal && Traversal->IsTraversing())
	{
		Chance += 0.1f;		// flat against a wall or on a rope: an easy target
	}

	// Father's rule: unrehearsed routes get you shot, rehearsed ones keep you in cover.
	if (const AShinobiEscapeRoute* Route = AShinobiEscapeRoute::FindRunningRoute(GetWorld()))
	{
		Chance += Route->GetRehearsals() == 0 ? 0.1f : -0.12f * Route->GetRehearsals();
	}

	return FMath::Clamp(Chance, 0.05f, 0.9f);
}

void AShinobiEnemyController::Shoot()
{
	AShinobiEnemy* Enemy = GetEnemy();
	APawn* Player = GetPlayer();
	if (!Enemy || !Player)
	{
		return;
	}

	const FVector From = Enemy->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const FVector Target = Player->GetActorLocation() + FVector(0.f, 0.f, 20.f);
	const bool bHit = FMath::FRand() < GetHitChance();

	FVector To = Target;
	bool bStick = true;
	if (!bHit)
	{
		// Close enough to hear: aim just to one side of him, then fly on past until the arrow
		// meets a wall or the ground. If it meets nothing it vanishes in the distance.
		const FVector Dir = (Target - From).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
		const FVector NearMiss = Target + Side * FMath::FRandRange(80.f, 160.f) * (FMath::RandBool() ? 1.f : -1.f);
		const FVector Far = From + (NearMiss - From).GetSafeNormal() * Enemy->SightRange * 1.5f;

		FCollisionQueryParams Query(SCENE_QUERY_STAT(ShinobiArrow), false, Enemy);
		Query.AddIgnoredActor(Player);
		FHitResult Impact;
		if (GetWorld()->LineTraceSingleByChannel(Impact, From, Far, ECC_Visibility, Query))
		{
			To = Impact.ImpactPoint;
		}
		else
		{
			To = Far;
			bStick = false;
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AShinobiArrow* Arrow = GetWorld()->SpawnActor<AShinobiArrow>(AShinobiArrow::StaticClass(), FTransform(From), Params))
	{
		Arrow->Launch(From, To, bHit ? Player : nullptr, bStick);
	}

	if (bHit)
	{
		if (AShinobiCharacter* Shinobi = Cast<AShinobiCharacter>(Player))
		{
			Shinobi->ReceiveArrowHit(Enemy->GetActorLocation());
		}
	}
	else if (UEscapeScoreComponent* Score = GetPlayerScore())
	{
		Score->PushMessage(TEXT("An arrow whistles past"));
	}
}
