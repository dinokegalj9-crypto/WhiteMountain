#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShinobiTypes.h"
#include "TraversalTestCourse.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class AShinobiEnemy;

/**
 * Grey-box test course: drop one into an empty level, add a NavMeshBoundsVolume, press Play.
 * Builds its geometry in the editor (so the navmesh sees it) and spawns ropes, anchors,
 * a tightrope, a village master and guards when play starts.
 *
 * Stations: 0 vault crate, 1 climbing wall, 2 ledge wall, 3 kaginawa tower, 4 rope wall,
 * 5 cliff for rope ladders (with a long ramp guards can use), 6 tightrope between pillars.
 */
UCLASS()
class WHITEMOUNTAIN_API ATraversalTestCourse : public AActor
{
	GENERATED_BODY()

public:
	ATraversalTestCourse();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Course")
	bool bBuildFloor = true;

	UPROPERTY(EditAnywhere, Category = "Course")
	bool bSpawnGuards = true;

	UPROPERTY(EditAnywhere, Category = "Course")
	TSubclassOf<AShinobiEnemy> GuardClass;

	UPROPERTY(EditAnywhere, Category = "Course")
	bool bMovePlayerToStart = true;

	UPROPERTY(EditAnywhere, Category = "Course")
	FVector PlayerStart = FVector(-800.f, 0.f, 100.f);

private:
	UStaticMeshComponent* AddBlock(const TCHAR* Name, const FVector& Center, const FVector& Size, const FRotator& Rotation, const FLinearColor& Color, FName Tag);
	void AddLabel(const TCHAR* Name, const FVector& Location, const FString& Text);
	void SpawnGuard(EGuardType Type, const FVector& LocalLocation);
	void MovePlayerToStart();

	template <typename T>
	T* SpawnLocal(const FVector& LocalLocation, const FRotator& LocalRotation);

	UPROPERTY(VisibleAnywhere, Category = "Course")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Generated;

	FTimerHandle StartTimer;
};
