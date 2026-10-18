#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShinobiTypes.h"
#include "ShinobiEnemy.generated.h"

class UStaticMeshComponent;

/**
 * A guard. Walks only where the navmesh goes: no climbing, no jumping, no ropes.
 * That limit is what makes the shinobi's routes worth escape points.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AShinobiEnemy();

	virtual void BeginPlay() override;

	/** Applies the per-type defaults below. Called in BeginPlay; call again after changing GuardType at runtime. */
	void ApplyGuardType();

	EEnemyAlert GetAlert() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	EGuardType GuardType = EGuardType::Spearman;

	/** When true, BeginPlay overwrites the values below from GuardType. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	bool bUseGuardTypeDefaults = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float PatrolSpeed = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float ChaseSpeed = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float SightRange = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float SightHalfAngle = 60.f;

	/** While alert, the guard notices the shinobi this close in any direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float CloseAwarenessRadius = 500.f;

	/** A crouching shinobi is not noticed beyond this distance by a calm guard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float CrouchHiddenDistance = 900.f;

	/** Seconds without sight before the guard starts searching. Dogs follow scent much longer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float LostSightGiveUpTime = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Senses")
	float SearchDuration = 6.f;

	/** Escape points from this guard are multiplied by this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float EscapePointMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	FLinearColor BodyColor = FLinearColor(0.55f, 0.08f, 0.06f);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Guard")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, Category = "Guard")
	TObjectPtr<UStaticMeshComponent> PlaceholderFace;
};
