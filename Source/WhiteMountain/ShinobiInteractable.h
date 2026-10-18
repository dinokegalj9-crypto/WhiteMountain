#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiInteractable.generated.h"

class AShinobiCharacter;
class UTextRenderComponent;

/**
 * Something in the world the shinobi uses with E: a jar of oil, a log to push down, a ledger to steal.
 * The character asks every interactable in range and uses the closest one that says yes.
 */
UCLASS(Abstract)
class WHITEMOUNTAIN_API AShinobiInteractable : public AActor
{
	GENERATED_BODY()

public:
	AShinobiInteractable();

	/** Where the shinobi has to stand. Defaults to the actor location. */
	virtual FVector GetInteractLocation() const { return GetActorLocation(); }

	/** True when Shinobi is close enough and the thing can be used right now. */
	virtual bool CanInteract(const AShinobiCharacter* Shinobi) const;

	virtual void Interact(AShinobiCharacter* Shinobi) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	float InteractRadius = 200.f;

protected:
	/** Floating "[E] ..." hint. Hidden while the thing cannot be used. */
	void SetPrompt(const FString& Text);

	UPROPERTY(VisibleAnywhere, Category = "Interact")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Interact")
	TObjectPtr<UTextRenderComponent> Prompt;
};
