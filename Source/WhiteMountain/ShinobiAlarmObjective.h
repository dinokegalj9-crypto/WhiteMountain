#pragma once

#include "CoreMinimal.h"
#include "ShinobiInteractable.h"
#include "ShinobiAlarmObjective.generated.h"

class UStaticMeshComponent;

/**
 * Something worth stealing (the magistrate's tax ledger in the test town). Taking it with E
 * raises the alarm: every guard within AlarmRadius runs to the spot and the escape begins.
 * It comes back after RespawnDelay so the escape can be tried again.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiAlarmObjective : public AShinobiInteractable
{
	GENERATED_BODY()

public:
	AShinobiAlarmObjective();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual bool CanInteract(const AShinobiCharacter* Shinobi) const override;
	virtual void Interact(AShinobiCharacter* Shinobi) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString ItemName = TEXT("the tax ledger");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float AlarmRadius = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float RespawnDelay = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	int32 Points = 50;

private:
	void Respawn();

	UPROPERTY(VisibleAnywhere, Category = "Objective")
	TObjectPtr<UStaticMeshComponent> Stand;

	UPROPERTY(VisibleAnywhere, Category = "Objective")
	TObjectPtr<UStaticMeshComponent> Item;

	bool bTaken = false;
	FTimerHandle RespawnTimer;
};
