#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiEscapeRoute.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UEscapeScoreComponent;

/**
 * A planned way out of a town: checkpoints the shinobi runs through in order.
 *
 * Run it calmly (nobody chasing) and it counts as a rehearsal, up to MaxRehearsals.
 * Run it with guards on your heels and it is an escape: bonus points, more for every rehearsal.
 * Rehearsing also shows the next checkpoint during a real escape; an unrehearsed route shows nothing.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiEscapeRoute : public AActor
{
	GENERATED_BODY()

public:
	AShinobiEscapeRoute();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Route")
	int32 GetRehearsals() const { return Rehearsals; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	FString RouteName = TEXT("Storehouse roof");

	/** Checkpoints in order, relative to the actor. The first one starts the run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route", meta = (MakeEditWidget = true))
	TArray<FVector> Checkpoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	float CheckpointRadius = 220.f;

	/** Seconds allowed between two checkpoints before the run is abandoned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	float StepTimeout = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	int32 MaxRehearsals = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	int32 EscapeBonus = 150;

	/** Each rehearsal adds this much to the escape bonus multiplier (3 rehearsals = x2.5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	float RehearsalBonusStep = 0.5f;

private:
	FVector GetCheckpointWorld(int32 Index) const;
	void StartRun(UEscapeScoreComponent* Score);
	void FinishRun(UEscapeScoreComponent* Score);
	void AbandonRun(UEscapeScoreComponent* Score, const TCHAR* Why);
	void UpdateMarkers();

	UPROPERTY(VisibleAnywhere, Category = "Route")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Markers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> MarkerLabels;

	UPROPERTY()
	TObjectPtr<UStaticMesh> MarkerMesh;

	int32 Rehearsals = 0;
	int32 NextIndex = 0;	// 0 = not running
	bool bChasedDuringRun = false;
	float StepTime = 0.f;
};
