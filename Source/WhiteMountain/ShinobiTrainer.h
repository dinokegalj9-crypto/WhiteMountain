#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiTypes.h"
#include "ShinobiTrainer.generated.h"

class AShinobiCharacter;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * A master in the shinobi village who teaches in exchange for escape points
 * (hunter: climbing and ropes, smith: weapons, herbalist: herbs).
 * Kept generic on purpose: the masters themselves are still being defined in the design doc.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiTrainer : public AActor
{
	GENERATED_BODY()

public:
	AShinobiTrainer();

	bool CanTrain(const AShinobiCharacter* Shinobi) const;
	void Train(AShinobiCharacter* Shinobi);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	FString TrainerName = TEXT("Hunter");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	EMasterSkill Skill = EMasterSkill::Traversal;

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trainer")
	float TalkRadius = 250.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Trainer")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Trainer")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Trainer")
	TObjectPtr<UTextRenderComponent> Label;
};
