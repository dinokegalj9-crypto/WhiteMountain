#include "ShinobiTrainer.h"
#include "ShinobiCharacter.h"
#include "ShinobiVisuals.h"
#include "EscapeScoreComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AShinobiTrainer::AShinobiTrainer()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetStaticMesh(CylinderMesh.Object);
	Body->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Body->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 230.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(40.f);
	Label->SetTextRenderColor(FColor(255, 220, 140));
}

void AShinobiTrainer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Label->SetText(FText::FromString(FString::Printf(TEXT("%s [E]"), *TrainerName)));
}

bool AShinobiTrainer::CanTrain(const AShinobiCharacter* Shinobi) const
{
	return Shinobi && FVector::Dist2D(Shinobi->GetActorLocation(), GetActorLocation()) <= TalkRadius;
}

void AShinobiTrainer::Train(AShinobiCharacter* Shinobi)
{
	UEscapeScoreComponent* Score = Shinobi ? Shinobi->GetEscapeScore() : nullptr;
	if (!Score)
	{
		return;
	}
	if (Skill != EMasterSkill::Traversal)
	{
		Score->PushMessage(FString::Printf(TEXT("%s: I have nothing to teach you yet (coming later)."), *TrainerName));
		return;
	}

	FString Message;
	if (!Score->TryBuyNextMove(Message))
	{
		Score->PushMessage(Message);	// a successful purchase already posts its own message
	}
}
