#include "ShinobiAlarmObjective.h"
#include "ShinobiCharacter.h"
#include "ShinobiEnemy.h"
#include "ShinobiEnemyController.h"
#include "ShinobiVisuals.h"
#include "EscapeScoreComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AShinobiAlarmObjective::AShinobiAlarmObjective()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Stand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stand"));
	Stand->SetupAttachment(Root);
	Stand->SetStaticMesh(CubeMesh.Object);
	Stand->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Stand->SetRelativeScale3D(FVector(0.6f, 0.9f, 0.8f));
	ShinobiVisuals::MakeCosmetic(Stand);

	Item = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item"));
	Item->SetupAttachment(Root);
	Item->SetStaticMesh(CubeMesh.Object);
	Item->SetRelativeLocation(FVector(0.f, 0.f, 86.f));
	Item->SetRelativeScale3D(FVector(0.25f, 0.35f, 0.08f));
	ShinobiVisuals::MakeCosmetic(Item);

	InteractRadius = 180.f;
}

void AShinobiAlarmObjective::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetPrompt(FString::Printf(TEXT("[E] Steal %s"), *ItemName));
}

void AShinobiAlarmObjective::BeginPlay()
{
	Super::BeginPlay();
	ShinobiVisuals::TintMesh(Stand, FLinearColor(0.25f, 0.12f, 0.08f));
	ShinobiVisuals::TintMesh(Item, FLinearColor(0.85f, 0.8f, 0.6f));
}

bool AShinobiAlarmObjective::CanInteract(const AShinobiCharacter* Shinobi) const
{
	return !bTaken && Super::CanInteract(Shinobi);
}

void AShinobiAlarmObjective::Interact(AShinobiCharacter* Shinobi)
{
	bTaken = true;
	Item->SetVisibility(false);
	SetPrompt(FString());

	int32 Alerted = 0;
	for (TActorIterator<AShinobiEnemy> It(GetWorld()); It; ++It)
	{
		AShinobiEnemyController* AI = Cast<AShinobiEnemyController>(It->GetController());
		if (AI && FVector::Dist(It->GetActorLocation(), GetActorLocation()) <= AlarmRadius)
		{
			AI->RaiseAlarm(Shinobi->GetActorLocation());
			++Alerted;
		}
	}

	if (UEscapeScoreComponent* Score = Shinobi->GetEscapeScore())
	{
		Score->AwardBonus(Points, FString::Printf(TEXT("You took %s! %d guards raise the alarm. Get out!"), *ItemName, Alerted));
	}
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AShinobiAlarmObjective::Respawn, RespawnDelay, false);
}

void AShinobiAlarmObjective::Respawn()
{
	bTaken = false;
	Item->SetVisibility(true);
	SetPrompt(FString::Printf(TEXT("[E] Steal %s"), *ItemName));
}
