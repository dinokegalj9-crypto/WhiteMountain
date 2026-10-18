#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiGrapplePoint.generated.h"

class AShinobiRope;
class UStaticMeshComponent;

/**
 * Anchor for the kaginawa (hook and rope): a beam end, a branch, the top of a wall.
 * Put it on the edge. Forward (X) must point onto the roof / ledge the rope leads to.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiGrapplePoint : public AActor
{
	GENERATED_BODY()

public:
	AShinobiGrapplePoint();

	/** Longest rope the hook can drop from here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kaginawa", meta = (ClampMin = "100"))
	float MaxRopeLength = 1500.f;

	/** Rope currently hanging from this anchor, if any. */
	TWeakObjectPtr<AShinobiRope> ActiveRope;

private:
	UPROPERTY(VisibleAnywhere, Category = "Kaginawa")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Kaginawa")
	TObjectPtr<UStaticMeshComponent> Marker;
};
