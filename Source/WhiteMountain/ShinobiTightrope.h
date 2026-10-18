#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiTightrope.generated.h"

class UStaticMeshComponent;

/**
 * A rope stretched between two anchors (across a river or a gap). The shinobi walks on it
 * and has to keep balance. Start = actor location, End = EndPoint (relative, drag the widget).
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiTightrope : public AActor
{
	GENERATED_BODY()

public:
	AShinobiTightrope();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tightrope", meta = (MakeEditWidget = true))
	FVector EndPoint = FVector(1000.f, 0.f, 0.f);

	FVector GetStart() const { return GetActorLocation(); }
	FVector GetEnd() const { return GetActorTransform().TransformPosition(EndPoint); }
	float GetLength() const { return FVector::Dist(GetStart(), GetEnd()); }
	FVector GetPointAt(float Distance) const;

	/** Distance along the rope of the closest point to Location. */
	float GetClosestDistance(const FVector& Location) const;

	void SetEnds(const FVector& WorldStart, const FVector& WorldEnd);

private:
	void RebuildVisuals();

	UPROPERTY(VisibleAnywhere, Category = "Tightrope")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Tightrope")
	TObjectPtr<UStaticMeshComponent> RopeMesh;
};
