#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiArrow.generated.h"

class UStaticMeshComponent;

/**
 * Cosmetic arrow: flies in a straight line to where the archer's shot was decided to land.
 * Hit or miss is rolled by the archer, not by physics, so the arrow never collides with anything.
 * At the end it sticks in what it hit (a wall, the ground, the shinobi, following him),
 * or simply disappears if it flew off into the distance.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiArrow : public AActor
{
	GENERATED_BODY()

public:
	AShinobiArrow();

	virtual void Tick(float DeltaSeconds) override;

	/** StickTo: actor to stay attached to on arrival. bStick false = vanish on arrival (flew off into the distance). */
	void Launch(const FVector& From, const FVector& To, AActor* StickTo, bool bStick);

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
	bool bStickAtEnd = true;
	TWeakObjectPtr<AActor> StickTarget;
	FVector StickOffset = FVector::ZeroVector;	// where on StickTarget the arrow lands
};
