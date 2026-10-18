#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiArrow.generated.h"

class UStaticMeshComponent;

/**
 * Cosmetic arrow: flies in a straight line to where the archer's shot was decided to land,
 * sticks there for a few seconds and disappears. Hit or miss is rolled by the archer, not by physics,
 * so the arrow never collides with anything.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiArrow : public AActor
{
	GENERATED_BODY()

public:
	AShinobiArrow();

	virtual void Tick(float DeltaSeconds) override;

	void Launch(const FVector& From, const FVector& To);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	float Speed = 3500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	float StuckSeconds = 4.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Arrow")
	TObjectPtr<UStaticMeshComponent> Shaft;

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	float Travelled = 0.f;
};
