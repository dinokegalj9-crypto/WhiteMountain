#include "ShinobiEnemy.h"
#include "ShinobiEnemyController.h"
#include "ShinobiVisuals.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
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
		SightRange = 3000.f;
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
