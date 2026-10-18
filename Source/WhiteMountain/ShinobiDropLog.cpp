#include "ShinobiDropLog.h"
#include "WhiteMountain.h"
#include "ShinobiCharacter.h"
#include "ShinobiEnemy.h"
#include "ShinobiVisuals.h"
#include "EscapeScoreComponent.h"
#include "AI/NavigationSystemBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The log sticks out this far past both ends of the ramp so there is no lip to trip on. */
	constexpr float LogOverhang = 30.f;
}

AShinobiDropLog::AShinobiDropLog()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	LogMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LogMesh"));
	LogMesh->SetupAttachment(Root);
	LogMesh->SetStaticMesh(CubeMesh.Object);
	LogMesh->SetMobility(EComponentMobility::Movable);
	LogMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	LogMesh->SetCanEverAffectNavigation(true);

	InteractRadius = 250.f;
}

void AShinobiDropLog::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	LogMesh->SetRelativeTransform(GetRampTransform());
	ShinobiVisuals::TintMesh(LogMesh, FLinearColor(0.42f, 0.3f, 0.18f));
	Prompt->SetRelativeLocation(TopPoint + FVector(0.f, 0.f, 120.f));
	SetPrompt(TEXT("[E] Push the log down"));
}

FTransform AShinobiDropLog::GetRampTransform() const
{
	const FVector2D Flat(TopPoint.X, TopPoint.Y);
	const float Run = FMath::Max(Flat.Size(), 1.f);
	const float Rise = TopPoint.Z;
	const float Slope = FMath::Sqrt(Run * Run + Rise * Rise);
	const FVector Dir(Flat.X / Run, Flat.Y / Run, 0.f);

	// Put the top face of the log on the line from the foot to TopPoint.
	const FVector Up = FVector(-Dir.X * Rise / Slope, -Dir.Y * Rise / Slope, Run / Slope);
	const FVector Center = TopPoint * 0.5f - Up * (Thickness * 0.5f);

	const FRotator Rotation(FMath::RadiansToDegrees(FMath::Atan2(Rise, Run)), FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)), 0.f);
	const FVector Scale((Slope + 2.f * LogOverhang) / 100.f, Width / 100.f, Thickness / 100.f);
	return FTransform(Rotation, Center, Scale);
}

FTransform AShinobiDropLog::GetFlatTransform() const
{
	const FVector2D Flat(TopPoint.X, TopPoint.Y);
	const float Run = FMath::Max(Flat.Size(), 1.f);
	const float Slope = FMath::Sqrt(Run * Run + TopPoint.Z * TopPoint.Z);
	const float Length = Slope + 2.f * LogOverhang;
	const FVector Dir(Flat.X / Run, Flat.Y / Run, 0.f);

	// Lying on the ground beside where it stood (so it does not cover the path), its far end just short of the wall.
	const FVector Side(Dir.Y, -Dir.X, 0.f);
	const FVector Center = Dir * (Run - 20.f - Length * 0.5f) + Side * (Width + 100.f) + FVector(0.f, 0.f, Thickness * 0.5f);
	const FRotator Rotation(0.f, FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)), 0.f);
	return FTransform(Rotation, Center, FVector(Length / 100.f, Width / 100.f, Thickness / 100.f));
}

FVector AShinobiDropLog::GetInteractLocation() const
{
	return GetActorTransform().TransformPosition(TopPoint);
}

bool AShinobiDropLog::CanInteract(const AShinobiCharacter* Shinobi) const
{
	// Only from up on the roof: you push it away, you cannot pull it down from below.
	return !bDown && !bFalling && Super::CanInteract(Shinobi)
		&& Shinobi->GetActorLocation().Z > GetInteractLocation().Z - 30.f;
}

void AShinobiDropLog::Interact(AShinobiCharacter* Shinobi)
{
	bFalling = true;
	FallAlpha = 0.f;
	SetPrompt(FString());
	LogMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorTickEnabled(true);

	// Anyone on the ramp or where it lands goes down.
	KnockDownGuardsUnderLog();

	if (UEscapeScoreComponent* Score = Shinobi ? Shinobi->GetEscapeScore() : nullptr)
	{
		Score->PushMessage(TEXT("Log pushed down: nobody follows you up here"));
	}
}

void AShinobiDropLog::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bFalling)
	{
		SetActorTickEnabled(false);
		return;
	}

	FallAlpha = FMath::Min(1.f, FallAlpha + DeltaSeconds / FMath::Max(FallTime, 0.05f));
	const float Eased = FallAlpha * FallAlpha;	// slow tip, fast crash
	const FTransform From = GetRampTransform();
	const FTransform To = GetFlatTransform();
	FTransform Blend;
	Blend.SetLocation(FMath::Lerp(From.GetLocation(), To.GetLocation(), Eased));
	Blend.SetRotation(FQuat::Slerp(From.GetRotation(), To.GetRotation(), Eased));
	Blend.SetScale3D(To.GetScale3D());
	LogMesh->SetRelativeTransform(Blend);

	if (FallAlpha >= 1.f)
	{
		bFalling = false;
		bDown = true;
		LogMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		UpdateNavigation();
		GetWorldTimerManager().SetTimer(ResetTimer, this, &AShinobiDropLog::TryReset, ResetDelay, false);
	}
}

void AShinobiDropLog::KnockDownGuardsUnderLog()
{
	const FTransform Ramp = GetRampTransform() * GetActorTransform();
	const FTransform Flat = GetFlatTransform() * GetActorTransform();
	const float HalfLength = Flat.GetScale3D().X * 50.f + 40.f;
	const float HalfWidth = Width * 0.5f + 60.f;

	UEscapeScoreComponent* Score = nullptr;
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Score = Player->FindComponentByClass<UEscapeScoreComponent>();
	}

	for (TActorIterator<AShinobiEnemy> It(GetWorld()); It; ++It)
	{
		const FVector Location = It->GetActorLocation();
		bool bHit = false;
		for (const FTransform& Shape : { Ramp, Flat })
		{
			// Compare in the log's own frame, ignoring its scale.
			const FVector Local = FTransform(Shape.GetRotation(), Shape.GetLocation()).InverseTransformPosition(Location);
			if (FMath::Abs(Local.X) < HalfLength && FMath::Abs(Local.Y) < HalfWidth && FMath::Abs(Local.Z) < 150.f)
			{
				bHit = true;
				break;
			}
		}
		if (bHit && It->Slip(KnockdownDuration) && Score)
		{
			Score->AwardBonus(PointsPerKnockdown, TEXT("Guard knocked down by the log"));
		}
	}
}

void AShinobiDropLog::TryReset()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bPlayerNear = Player
		&& (FVector::Dist(Player->GetActorLocation(), GetActorLocation()) < 1000.f
			|| FVector::Dist(Player->GetActorLocation(), GetInteractLocation()) < 1000.f);
	if (bPlayerNear)
	{
		GetWorldTimerManager().SetTimer(ResetTimer, this, &AShinobiDropLog::TryReset, 5.f, false);
		return;
	}

	bDown = false;
	LogMesh->SetRelativeTransform(GetRampTransform());
	UpdateNavigation();
	SetPrompt(TEXT("[E] Push the log down"));
	UE_LOG(LogWhiteMountain, Log, TEXT("%s leaned back up"), *GetName());
}

void AShinobiDropLog::UpdateNavigation()
{
	// The navmesh is set to rebuild at runtime (DefaultEngine.ini); tell it the ramp moved.
	FNavigationSystem::UpdateComponentData(*LogMesh);
}
