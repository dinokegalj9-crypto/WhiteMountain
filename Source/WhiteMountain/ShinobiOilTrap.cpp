#include "ShinobiOilTrap.h"
#include "ShinobiCharacter.h"
#include "ShinobiEnemy.h"
#include "ShinobiVisuals.h"
#include "EscapeScoreComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AShinobiOilTrap::AShinobiOilTrap()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Jar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Jar"));
	Jar->SetupAttachment(Root);
	Jar->SetStaticMesh(CylinderMesh.Object);
	Jar->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Jar->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));
	ShinobiVisuals::MakeCosmetic(Jar);

	SlickMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlickMesh"));
	SlickMesh->SetupAttachment(Root);
	SlickMesh->SetStaticMesh(CubeMesh.Object);
	ShinobiVisuals::MakeCosmetic(SlickMesh);
	SlickMesh->SetVisibility(false);

	SlickArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SlickArea"));
	SlickArea->SetupAttachment(Root);
	SlickArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SlickArea->SetGenerateOverlapEvents(true);
	SlickArea->SetCanEverAffectNavigation(false);
	SlickArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractRadius = 180.f;
}

void AShinobiOilTrap::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SlickMesh->SetRelativeLocation(SlickOffset + FVector(0.f, 0.f, 1.f));
	SlickMesh->SetRelativeScale3D(FVector(SlickSize.X / 100.f, SlickSize.Y / 100.f, 0.01f));
	SlickArea->SetRelativeLocation(SlickOffset + FVector(0.f, 0.f, 60.f));
	SlickArea->SetBoxExtent(FVector(SlickSize.X * 0.5f, SlickSize.Y * 0.5f, 60.f));
	SetPrompt(TEXT("[E] Pour oil"));
}

void AShinobiOilTrap::BeginPlay()
{
	Super::BeginPlay();
	ShinobiVisuals::TintMesh(Jar, FLinearColor(0.45f, 0.25f, 0.12f));
	ShinobiVisuals::TintMesh(SlickMesh, FLinearColor(0.08f, 0.07f, 0.03f));
	SlickArea->OnComponentBeginOverlap.AddDynamic(this, &AShinobiOilTrap::OnSlickOverlap);
}

bool AShinobiOilTrap::CanInteract(const AShinobiCharacter* Shinobi) const
{
	return bFull && Super::CanInteract(Shinobi);
}

void AShinobiOilTrap::Interact(AShinobiCharacter* Shinobi)
{
	bFull = false;
	bOilDown = true;
	SlickMesh->SetVisibility(true);
	SlickArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetPrompt(FString());
	GetWorldTimerManager().SetTimer(OilTimer, this, &AShinobiOilTrap::Dry, OilDuration, false);

	if (UEscapeScoreComponent* Score = Shinobi ? Shinobi->GetEscapeScore() : nullptr)
	{
		Score->PushMessage(TEXT("Oil poured on the stones"));
	}

	// Guards already standing on the stones go down too.
	TArray<AActor*> Inside;
	SlickArea->UpdateOverlaps();
	SlickArea->GetOverlappingActors(Inside, AShinobiEnemy::StaticClass());
	for (AActor* Actor : Inside)
	{
		OnSlickOverlap(SlickArea, Actor, nullptr, 0, false, FHitResult());
	}
}

void AShinobiOilTrap::OnSlickOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AShinobiEnemy* Guard = Cast<AShinobiEnemy>(OtherActor);
	if (!bOilDown || !Guard || !Guard->Slip(SlipDuration))
	{
		return;
	}
	// Only a guard who is after you is worth points; a patrol slipping is just luck.
	if (Guard->GetAlert() != EEnemyAlert::Chase)
	{
		return;
	}
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (UEscapeScoreComponent* Score = Player ? Player->FindComponentByClass<UEscapeScoreComponent>() : nullptr)
	{
		Score->AwardBonus(PointsPerSlip, TEXT("Guard slipped on the oil"));
	}
}

void AShinobiOilTrap::Dry()
{
	bOilDown = false;
	SlickMesh->SetVisibility(false);
	SlickArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorldTimerManager().SetTimer(OilTimer, this, &AShinobiOilTrap::Refill, RefillDelay, false);
}

void AShinobiOilTrap::Refill()
{
	bFull = true;
	SetPrompt(TEXT("[E] Pour oil"));
}
