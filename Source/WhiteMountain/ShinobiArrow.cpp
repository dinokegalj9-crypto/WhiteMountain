#include "ShinobiArrow.h"
#include "ShinobiVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AShinobiArrow::AShinobiArrow()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// The cylinder points up (Z); lay it along X so the actor's forward vector is the flight direction.
	Shaft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shaft"));
	Shaft->SetupAttachment(SceneRoot);
	Shaft->SetStaticMesh(CylinderMesh.Object);
	Shaft->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Shaft->SetRelativeScale3D(FVector(0.03f, 0.03f, 0.8f));
	ShinobiVisuals::MakeCosmetic(Shaft);
}

void AShinobiArrow::Launch(const FVector& From, const FVector& To)
{
	Start = From;
	End = To;
	Travelled = 0.f;
	SetActorLocationAndRotation(From, (To - From).Rotation());
	ShinobiVisuals::TintMesh(Shaft, FLinearColor(0.3f, 0.22f, 0.12f));
}

void AShinobiArrow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Length = FVector::Dist(Start, End);
	Travelled += Speed * DeltaSeconds;
	if (Travelled >= Length)
	{
		SetActorLocation(End);
		SetActorTickEnabled(false);
		SetLifeSpan(StuckSeconds);
		return;
	}
	SetActorLocation(FMath::Lerp(Start, End, Travelled / FMath::Max(Length, 1.f)));
}
