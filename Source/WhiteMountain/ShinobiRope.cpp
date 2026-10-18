#include "ShinobiRope.h"
#include "ShinobiVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor RopeColor(0.45f, 0.33f, 0.18f);
	constexpr float RopeThickness = 4.f;
	constexpr float LadderHalfWidth = 20.f;
	constexpr float RungSpacing = 35.f;
}

AShinobiRope::AShinobiRope()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	RailA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RailA"));
	RailA->SetupAttachment(Root);
	RailA->SetStaticMesh(CylinderMesh.Object);
	ShinobiVisuals::MakeCosmetic(RailA);

	RailB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RailB"));
	RailB->SetupAttachment(Root);
	RailB->SetStaticMesh(CylinderMesh.Object);
	ShinobiVisuals::MakeCosmetic(RailB);

	Rungs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rungs"));
	Rungs->SetupAttachment(Root);
	Rungs->SetStaticMesh(CubeMesh.Object);
	ShinobiVisuals::MakeCosmetic(Rungs);
}

void AShinobiRope::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildVisuals();
}

void AShinobiRope::SetupRope(float InLength, bool bInLadder, bool bInTopExit)
{
	Length = FMath::Max(100.f, InLength);
	bIsLadder = bInLadder;
	bTopExit = bInTopExit;
	RebuildVisuals();
}

FVector AShinobiRope::GetPointAtDistance(float DistanceFromTop) const
{
	return GetTop() - FVector(0.f, 0.f, FMath::Clamp(DistanceFromTop, 0.f, Length));
}

FVector AShinobiRope::GetExitDirection() const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	return Forward.IsNearlyZero() ? FVector::ForwardVector : Forward;
}

void AShinobiRope::Retract(float Delay)
{
	if (bRetracting)
	{
		return;
	}
	bRetracting = true;
	GetWorldTimerManager().SetTimer(RetractTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		Destroy();
	}), FMath::Max(0.01f, Delay), false);
}

void AShinobiRope::RebuildVisuals()
{
	// BasicShapes are 100 cm and centred on their pivot.
	const FVector RailScale(RopeThickness / 100.f, RopeThickness / 100.f, Length / 100.f);
	const float RailSide = bIsLadder ? LadderHalfWidth : 0.f;

	RailA->SetRelativeLocation(FVector(0.f, -RailSide, -Length * 0.5f));
	RailA->SetRelativeScale3D(RailScale);
	ShinobiVisuals::TintMesh(RailA, RopeColor);

	RailB->SetVisibility(bIsLadder);
	RailB->SetRelativeLocation(FVector(0.f, RailSide, -Length * 0.5f));
	RailB->SetRelativeScale3D(RailScale);
	ShinobiVisuals::TintMesh(RailB, RopeColor);

	Rungs->ClearInstances();
	if (bIsLadder)
	{
		const FVector RungScale(0.03f, (LadderHalfWidth * 2.f) / 100.f, 0.03f);
		for (float Z = RungSpacing; Z < Length; Z += RungSpacing)
		{
			Rungs->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, -Z), RungScale));
		}
	}
}
