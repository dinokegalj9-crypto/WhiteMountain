#include "ShinobiEscapeRoute.h"
#include "WhiteMountain.h"
#include "ShinobiVisuals.h"
#include "EscapeScoreComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor NextColor(1.f, 0.75f, 0.2f);
	const FLinearColor KnownColor(0.35f, 0.3f, 0.2f);
}

AShinobiEscapeRoute::AShinobiEscapeRoute()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	MarkerMesh = Sphere.Object;
}

void AShinobiEscapeRoute::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Checkpoints.Num() == 0)
	{
		Checkpoints = { FVector::ZeroVector, FVector(800.f, 0.f, 0.f) };
	}
}

void AShinobiEscapeRoute::BeginPlay()
{
	Super::BeginPlay();

	for (int32 Index = 0; Index < Checkpoints.Num(); ++Index)
	{
		UStaticMeshComponent* Marker = NewObject<UStaticMeshComponent>(this);
		Marker->SetupAttachment(Root);
		Marker->SetStaticMesh(MarkerMesh);
		Marker->SetRelativeLocation(Checkpoints[Index] + FVector(0.f, 0.f, 120.f));
		Marker->SetRelativeScale3D(FVector(0.25f));
		ShinobiVisuals::MakeCosmetic(Marker);
		Marker->RegisterComponent();
		Markers.Add(Marker);

		UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
		Label->SetupAttachment(Root);
		Label->SetRelativeLocation(Checkpoints[Index] + FVector(0.f, 0.f, 170.f));
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetWorldSize(30.f);
		Label->SetTextRenderColor(FColor(255, 220, 140));
		Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Label->SetText(FText::FromString(Index == 0
			? FString::Printf(TEXT("Escape route: %s"), *RouteName)
			: FString::Printf(TEXT("%d / %d"), Index + 1, Checkpoints.Num())));
		Label->RegisterComponent();
		MarkerLabels.Add(Label);
	}
	UpdateMarkers();
}

FVector AShinobiEscapeRoute::GetCheckpointWorld(int32 Index) const
{
	return GetActorTransform().TransformPosition(Checkpoints[Index]);
}

void AShinobiEscapeRoute::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	UEscapeScoreComponent* Score = Player ? Player->FindComponentByClass<UEscapeScoreComponent>() : nullptr;
	if (!Score || Checkpoints.Num() < 2)
	{
		return;
	}

	// Checkpoints are on the ground or a roof; compare against the shinobi's feet, loosely.
	const FVector Feet = Player->GetActorLocation() - FVector(0.f, 0.f, 90.f);
	auto IsAt = [&](int32 Index)
	{
		const FVector Point = GetCheckpointWorld(Index);
		return FVector::Dist2D(Feet, Point) < CheckpointRadius && FMath::Abs(Feet.Z - Point.Z) < 200.f;
	};

	if (NextIndex == 0)
	{
		if (IsAt(0))
		{
			StartRun(Score);
		}
		return;
	}

	StepTime += DeltaSeconds;
	if (!bChasedDuringRun && Score->GetPursuerCount() > 0)
	{
		bChasedDuringRun = true;
		UpdateMarkers();
	}

	if (IsAt(NextIndex))
	{
		++NextIndex;
		StepTime = 0.f;
		if (NextIndex >= Checkpoints.Num())
		{
			FinishRun(Score);
		}
		else
		{
			UpdateMarkers();
		}
	}
	else if (StepTime > StepTimeout)
	{
		AbandonRun(Score, TEXT("too slow"));
	}
}

void AShinobiEscapeRoute::StartRun(UEscapeScoreComponent* Score)
{
	NextIndex = 1;
	StepTime = 0.f;
	bChasedDuringRun = Score->GetPursuerCount() > 0;
	if (!bChasedDuringRun)
	{
		Score->PushMessage(FString::Printf(TEXT("Rehearsing the route \"%s\" (%d / %d done)"), *RouteName, Rehearsals, MaxRehearsals));
	}
	UpdateMarkers();
}

void AShinobiEscapeRoute::FinishRun(UEscapeScoreComponent* Score)
{
	if (bChasedDuringRun)
	{
		const float Multiplier = 1.f + RehearsalBonusStep * Rehearsals;
		const int32 Earned = FMath::RoundToInt(EscapeBonus * Multiplier);
		Score->AwardBonus(Earned, FString::Printf(TEXT("Escaped by the route \"%s\" (%d x%.1f, rehearsed %d times)"),
			*RouteName, EscapeBonus, Multiplier, Rehearsals));
	}
	else if (Rehearsals < MaxRehearsals)
	{
		++Rehearsals;
		Score->PushMessage(FString::Printf(TEXT("Route \"%s\" rehearsed (%d / %d): escapes here are worth more"), *RouteName, Rehearsals, MaxRehearsals));
	}
	else
	{
		Score->PushMessage(FString::Printf(TEXT("You know the route \"%s\" by heart"), *RouteName));
	}
	UE_LOG(LogWhiteMountain, Log, TEXT("Route %s finished, chased=%d, rehearsals=%d"), *RouteName, bChasedDuringRun ? 1 : 0, Rehearsals);

	NextIndex = 0;
	UpdateMarkers();
}

void AShinobiEscapeRoute::AbandonRun(UEscapeScoreComponent* Score, const TCHAR* Why)
{
	Score->PushMessage(FString::Printf(TEXT("Route \"%s\" abandoned (%s)"), *RouteName, Why));
	NextIndex = 0;
	UpdateMarkers();
}

void AShinobiEscapeRoute::UpdateMarkers()
{
	// Calm runs teach the way; under pursuit only a rehearsed route shows where to go next.
	const bool bShowNext = !bChasedDuringRun || Rehearsals > 0;

	for (int32 Index = 0; Index < Markers.Num(); ++Index)
	{
		bool bVisible = false;
		bool bNext = false;
		if (NextIndex == 0)
		{
			bVisible = Index == 0 || Rehearsals > 0;
			bNext = Index == 0;
		}
		else if (Index == NextIndex)
		{
			bVisible = bShowNext;
			bNext = true;
		}

		Markers[Index]->SetVisibility(bVisible);
		MarkerLabels[Index]->SetVisibility(bVisible);
		if (bVisible)
		{
			ShinobiVisuals::TintMesh(Markers[Index], bNext ? NextColor : KnownColor);
		}
	}
}
