#include "ShinobiInteractable.h"
#include "ShinobiCharacter.h"
#include "Components/TextRenderComponent.h"

AShinobiInteractable::AShinobiInteractable()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Prompt = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Prompt"));
	Prompt->SetupAttachment(Root);
	Prompt->SetRelativeLocation(FVector(0.f, 0.f, 160.f));
	Prompt->SetHorizontalAlignment(EHTA_Center);
	Prompt->SetWorldSize(32.f);
	Prompt->SetTextRenderColor(FColor(255, 220, 140));
	Prompt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AShinobiInteractable::CanInteract(const AShinobiCharacter* Shinobi) const
{
	return Shinobi && FVector::Dist(Shinobi->GetActorLocation(), GetInteractLocation()) <= InteractRadius;
}

void AShinobiInteractable::SetPrompt(const FString& Text)
{
	Prompt->SetText(FText::FromString(Text));
	Prompt->SetVisibility(!Text.IsEmpty());
}
