#pragma once

#include "CoreMinimal.h"
#include "ShinobiInteractable.h"
#include "ShinobiDropLog.generated.h"

class UStaticMeshComponent;

/**
 * Escape trap: a heavy log leaned against a roof as a ramp. Guards can run up it like anyone else.
 * From the top the shinobi pushes it down with E: the log falls flat, the navmesh loses the ramp,
 * the guards below have no way up (an escape), and anyone under it is knocked down.
 * The log is put back up after a while once the shinobi is gone, so the route can be used again.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiDropLog : public AShinobiInteractable
{
	GENERATED_BODY()

public:
	AShinobiDropLog();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetInteractLocation() const override;
	virtual bool CanInteract(const AShinobiCharacter* Shinobi) const override;
	virtual void Interact(AShinobiCharacter* Shinobi) override;

	/** Upper end of the log (on the roof edge), relative to the foot at the actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log", meta = (MakeEditWidget = true))
	FVector TopPoint = FVector(700.f, 0.f, 450.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	float Width = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	float Thickness = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	float FallTime = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	float KnockdownDuration = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	int32 PointsPerKnockdown = 50;

	/** Seconds after falling before the log is leaned back up (only once the shinobi is not near). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	float ResetDelay = 30.f;

	bool IsDown() const { return bDown; }

private:
	FTransform GetRampTransform() const;
	FTransform GetFlatTransform() const;
	void KnockDownGuardsUnderLog();
	void TryReset();
	void UpdateNavigation();

	UPROPERTY(VisibleAnywhere, Category = "Log")
	TObjectPtr<UStaticMeshComponent> LogMesh;

	bool bDown = false;
	bool bFalling = false;
	float FallAlpha = 0.f;
	FTimerHandle ResetTimer;
};
