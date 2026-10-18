#include "ShinobiTightrope.h"
#include "ShinobiVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AShinobiTightrope::AShinobiTightrope()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	RopeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RopeMesh"));
	RopeMesh->SetupAttachment(Root);
	RopeMesh->SetStaticMesh(CylinderMesh.Object);
	RopeMesh->SetUsingAbsoluteRotation(true);
	RopeMesh->SetUsingAbsoluteScale(true);
	ShinobiVisuals::MakeCosmetic(RopeMesh);
}

void AShinobiTightrope::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildVisuals();
}

FVector AShinobiTightrope::GetPointAt(float Distance) const
{
	const float Len = GetLength();
	const float Alpha = Len > KINDA_SMALL_NUMBER ? FMath::Clamp(Distance / Len, 0.f, 1.f) : 0.f;
	return FMath::Lerp(GetStart(), GetEnd(), Alpha);
}

float AShinobiTightrope::GetClosestDistance(const FVector& Location) const
{
	const FVector Closest = FMath::ClosestPointOnSegment(Location, GetStart(), GetEnd());
	return FVector::Dist(GetStart(), Closest);
}

void AShinobiTightrope::SetEnds(const FVector& WorldStart, const FVector& WorldEnd)
{
	SetActorLocation(WorldStart);
	EndPoint = GetActorTransform().InverseTransformPosition(WorldEnd);
	RebuildVisuals();
}

void AShinobiTightrope::RebuildVisuals()
{
	const FVector Start = GetStart();
	const FVector End = GetEnd();
	const FVector Dir = (End - Start).GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return;
	}
	// The cylinder's long axis is Z, so rotate Z onto the rope direction.
	RopeMesh->SetWorldLocation((Start + End) * 0.5f);
	RopeMesh->SetWorldRotation(FRotationMatrix::MakeFromZ(Dir).Rotator());
	RopeMesh->SetWorldScale3D(FVector(0.04f, 0.04f, GetLength() / 100.f));
	ShinobiVisuals::TintMesh(RopeMesh, FLinearColor(0.45f, 0.33f, 0.18f));
}
