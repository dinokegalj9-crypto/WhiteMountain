#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiRope.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

/**
 * A vertical rope or rope ladder hanging down from its actor location.
 * Placed in a level, thrown by the kaginawa, or tied at an edge by the player.
 * It never touches the navmesh, so guards cannot use it: that is the whole point.
 * Forward (X) points onto the ledge the rope leads to (only used when bTopExit).
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiRope : public AActor
{
	GENERATED_BODY()

public:
	AShinobiRope();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Hanging length in cm, measured down from the actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope", meta = (ClampMin = "100"))
	float Length = 500.f;

	/** Rope ladder: faster to climb and looks different. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	bool bIsLadder = false;

	/** Hangs against a wall: at the top the shinobi climbs over onto the ledge in front (actor forward). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rope")
	bool bTopExit = true;

	/** Set when the player tied this ladder, so they can untie it again. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	bool bDeployedByPlayer = false;

	/** Kaginawa ropes are pulled up after the shinobi climbs over the top. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	bool bRetractWhenLeft = false;

	void SetupRope(float InLength, bool bInLadder, bool bInTopExit);

	FVector GetTop() const { return GetActorLocation(); }
	FVector GetBottom() const { return GetActorLocation() - FVector(0.f, 0.f, Length); }
	FVector GetPointAtDistance(float DistanceFromTop) const;
	FVector GetExitDirection() const;
	float GetClimbSpeedMultiplier() const { return bIsLadder ? 1.4f : 1.f; }

	/** Pull the rope up (destroy) after a delay. */
	void Retract(float Delay);
	bool IsRetracting() const { return bRetracting; }

private:
	void RebuildVisuals();

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UStaticMeshComponent> RailA;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UStaticMeshComponent> RailB;

	UPROPERTY(VisibleAnywhere, Category = "Rope")
	TObjectPtr<UInstancedStaticMeshComponent> Rungs;

	bool bRetracting = false;
	FTimerHandle RetractTimer;
};
