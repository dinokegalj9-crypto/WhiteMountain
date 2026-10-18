#pragma once

#include "CoreMinimal.h"
#include "ShinobiInteractable.h"
#include "ShinobiOilTrap.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

/**
 * Escape trap: a jar of lamp oil next to a stone path. E pours it out; every guard who runs
 * onto the slick goes down for a few seconds. The oil dries after a while and the jar refills
 * (so the route can be rehearsed and used again).
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiOilTrap : public AShinobiInteractable
{
	GENERATED_BODY()

public:
	AShinobiOilTrap();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual bool CanInteract(const AShinobiCharacter* Shinobi) const override;
	virtual void Interact(AShinobiCharacter* Shinobi) override;

	/** Centre of the slick, relative to the jar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil", meta = (MakeEditWidget = true))
	FVector SlickOffset = FVector(0.f, 250.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil")
	FVector2D SlickSize = FVector2D(350.f, 300.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil")
	float SlipDuration = 2.5f;

	/** Seconds before the oil is gone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil")
	float OilDuration = 40.f;

	/** Seconds after drying before the jar can be used again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil")
	float RefillDelay = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oil")
	int32 PointsPerSlip = 30;

private:
	UFUNCTION()
	void OnSlickOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Dry();
	void Refill();

	UPROPERTY(VisibleAnywhere, Category = "Oil")
	TObjectPtr<UStaticMeshComponent> Jar;

	UPROPERTY(VisibleAnywhere, Category = "Oil")
	TObjectPtr<UStaticMeshComponent> SlickMesh;

	UPROPERTY(VisibleAnywhere, Category = "Oil")
	TObjectPtr<UBoxComponent> SlickArea;

	bool bFull = true;
	bool bOilDown = false;
	FTimerHandle OilTimer;
};
