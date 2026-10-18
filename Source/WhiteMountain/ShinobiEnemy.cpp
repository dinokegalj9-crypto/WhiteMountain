#include "ShinobiEnemy.h"
#include "ShinobiEnemyController.h"
#include "ShinobiVisuals.h"
#include "EscapeScoreComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AShinobiEnemy::AShinobiEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 92.f);

	AIControllerClass = AShinobiEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 480.f, 0.f);
	MoveComp->MaxWalkSpeed = PatrolSpeed;
	MoveComp->JumpZVelocity = 0.f;
	MoveComp->bCanWalkOffLedges = false;	// guards do not drop off roofs or cliffs
	MoveComp->GetNavAgentPropertiesRef().bCanJump = false;
	MoveComp->bUseRVOAvoidance = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetStaticMesh(CylinderMesh.Object);
	PlaceholderBody->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.84f));
	ShinobiVisuals::MakeCosmetic(PlaceholderBody);

	PlaceholderFace = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderFace"));
	PlaceholderFace->SetupAttachment(GetCapsuleComponent());
	PlaceholderFace->SetStaticMesh(CubeMesh.Object);
	PlaceholderFace->SetRelativeLocation(FVector(38.f, 0.f, 60.f));
	PlaceholderFace->SetRelativeScale3D(FVector(0.15f, 0.5f, 0.12f));
	ShinobiVisuals::MakeCosmetic(PlaceholderFace);
}

void AShinobiEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (bUseGuardTypeDefaults)
	{
		ApplyGuardType();
	}
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;

	if (ShinobiVisuals::TryApplyMannequin(this))
	{
		PlaceholderBody->SetVisibility(false);
		PlaceholderFace->SetVisibility(false);
	}
	else
	{
		ShinobiVisuals::TintMesh(PlaceholderBody, BodyColor);
		ShinobiVisuals::TintMesh(PlaceholderFace, FLinearColor(0.9f, 0.9f, 0.8f));
	}
}

void AShinobiEnemy::ApplyGuardType()
{
	switch (GuardType)
	{
	case EGuardType::Spearman:
		PatrolSpeed = 180.f; ChaseSpeed = 520.f; LostSightGiveUpTime = 6.f; SearchDuration = 6.f;
		EscapePointMultiplier = 1.f; BodyColor = FLinearColor(0.55f, 0.08f, 0.06f);
		break;
	case EGuardType::Samurai:
		PatrolSpeed = 150.f; ChaseSpeed = 430.f; LostSightGiveUpTime = 7.f; SearchDuration = 8.f;
		EscapePointMultiplier = 1.5f; BodyColor = FLinearColor(0.15f, 0.15f, 0.18f);
		break;
	case EGuardType::Archer:
		PatrolSpeed = 170.f; ChaseSpeed = 400.f; LostSightGiveUpTime = 4.f; SearchDuration = 5.f;
		SightRange = 3000.f; bCanShoot = true;
		EscapePointMultiplier = 1.3f; BodyColor = FLinearColor(0.1f, 0.3f, 0.1f);
		break;
	case EGuardType::Dog:
		PatrolSpeed = 250.f; ChaseSpeed = 680.f; LostSightGiveUpTime = 12.f; SearchDuration = 8.f;
		SightRange = 1500.f; CloseAwarenessRadius = 900.f;
		EscapePointMultiplier = 2.f; BodyColor = FLinearColor(0.4f, 0.25f, 0.1f);
		GetCapsuleComponent()->SetCapsuleSize(35.f, 45.f);
		PlaceholderBody->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.9f));
		PlaceholderFace->SetRelativeLocation(FVector(35.f, 0.f, 20.f));
		break;
	}
}

EEnemyAlert AShinobiEnemy::GetAlert() const
{
	const AShinobiEnemyController* AI = Cast<AShinobiEnemyController>(GetController());
	return AI ? AI->GetAlert() : EEnemyAlert::Patrol;
}

bool AShinobiEnemy::IsAiming() const
{
	const AShinobiEnemyController* AI = Cast<AShinobiEnemyController>(GetController());
	return AI && AI->IsAiming();
}

bool AShinobiEnemy::Slip(float Duration)
{
	if (bStunned)
	{
		return false;
	}
	bStunned = true;

	if (AController* AI = GetController())
	{
		AI->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = 0.f;
	FallOver(75.f);

	GetWorldTimerManager().SetTimer(RecoverTimer, this, &AShinobiEnemy::Recover, FMath::Max(Duration, 0.1f), false);
	return true;
}

void AShinobiEnemy::FallOver(float Roll)
{
	if (!bFallen)
	{
		BodyRotationBeforeSlip = PlaceholderBody->GetRelativeRotation();
		MeshRotationBeforeSlip = GetMesh()->GetRelativeRotation();
		bFallen = true;
	}
	PlaceholderBody->SetRelativeRotation(BodyRotationBeforeSlip + FRotator(0.f, 0.f, Roll));
	PlaceholderFace->SetVisibility(false);
	GetMesh()->SetRelativeRotation(MeshRotationBeforeSlip + FRotator(0.f, 0.f, Roll));
}

void AShinobiEnemy::Recover()
{
	const bool bWasUnconscious = bUnconscious;
	bStunned = false;
	bHeld = false;
	bUnconscious = false;
	bBodyFound = false;
	RestorePawnBlock();
	if (bFallen)
	{
		bFallen = false;
		PlaceholderBody->SetRelativeRotation(BodyRotationBeforeSlip);
		PlaceholderFace->SetVisibility(PlaceholderBody->IsVisible());
		GetMesh()->SetRelativeRotation(MeshRotationBeforeSlip);
	}
	// Same speeds as AShinobiEnemyController::EnterState.
	switch (GetAlert())
	{
	case EEnemyAlert::Chase:  GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed; break;
	case EEnemyAlert::Search: GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed * 1.5f; break;
	default:                  GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed; break;
	}

	if (bWasUnconscious)
	{
		// He comes to with a sore neck and looks around for whoever did it.
		if (AShinobiEnemyController* AI = Cast<AShinobiEnemyController>(GetController()))
		{
			AI->Investigate(GetActorLocation(), true);
		}
	}
}

void AShinobiEnemy::RestorePawnBlock()
{
	// Do not stand up inside the shinobi if he is standing over the body: wait until he steps off.
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player && FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) < 100.f)
	{
		GetWorldTimerManager().SetTimer(BlockTimer, this, &AShinobiEnemy::RestorePawnBlock, 0.3f, false);
		return;
	}
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

// ---------------------------------------------------------------------------
// Takedowns
// ---------------------------------------------------------------------------

bool AShinobiEnemy::CanBeTakenDown(const FVector& ShinobiLocation) const
{
	if (bUnconscious || bHeld)
	{
		return false;
	}
	const FVector Delta = ShinobiLocation - GetActorLocation();
	if (Delta.Size2D() > TakedownRange || FMath::Abs(Delta.Z) > 80.f)
	{
		return false;
	}
	if (bStunned)
	{
		return true;	// slipped on oil or knocked down by the log: finish him from any side
	}

	// A guard who is hunting or searching watches his back.
	const EEnemyAlert Alert = GetAlert();
	if (Alert == EEnemyAlert::Chase || Alert == EEnemyAlert::Frustrated || Alert == EEnemyAlert::Search)
	{
		return false;
	}
	const FVector Behind = -GetActorForwardVector().GetSafeNormal2D();
	const float CosAngle = FVector::DotProduct(Behind, Delta.GetSafeNormal2D());
	return CosAngle >= FMath::Cos(FMath::DegreesToRadians(TakedownBehindAngle));
}

void AShinobiEnemy::BeginTakedown()
{
	GetWorldTimerManager().ClearTimer(RecoverTimer);	// a slipped guard does not get up mid-takedown
	bStunned = true;
	bHeld = true;
	if (AController* AI = GetController())
	{
		AI->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = 0.f;
}

void AShinobiEnemy::FinishTakedown(float Seconds)
{
	bHeld = false;
	bUnconscious = true;
	bBodyFound = false;
	FallOver(90.f);
	// Lying flat: the shinobi can step over him.
	GetWorldTimerManager().ClearTimer(BlockTimer);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// He is out of the hunt: no escape points for him, but no pursuer either.
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (UEscapeScoreComponent* Score = Player->FindComponentByClass<UEscapeScoreComponent>())
		{
			Score->ReportPursuerGone(this);
		}
	}
	GetWorldTimerManager().SetTimer(RecoverTimer, this, &AShinobiEnemy::Recover, FMath::Max(Seconds, 0.1f), false);
}

void AShinobiEnemy::BreakTakedown(const FVector& ShinobiLocation)
{
	if (!bHeld)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(RecoverTimer);
	Recover();
	if (AShinobiEnemyController* AI = Cast<AShinobiEnemyController>(GetController()))
	{
		AI->RaiseAlarm(ShinobiLocation);
	}
}
