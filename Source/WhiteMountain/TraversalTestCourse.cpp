#include "TraversalTestCourse.h"
#include "WhiteMountain.h"
#include "ShinobiVisuals.h"
#include "ShinobiRope.h"
#include "ShinobiGrapplePoint.h"
#include "ShinobiTightrope.h"
#include "ShinobiTrainer.h"
#include "ShinobiEnemy.h"
#include "ShinobiEscapeRoute.h"
#include "ShinobiOilTrap.h"
#include "ShinobiDropLog.h"
#include "ShinobiAlarmObjective.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName ClimbableTag(TEXT("Climbable"));
	const FName NoClimbTag(TEXT("NoClimb"));

	const FLinearColor FloorColor(0.22f, 0.25f, 0.18f);
	const FLinearColor ClimbColor(0.55f, 0.42f, 0.28f);	// warm wood = climbable
	const FLinearColor PlainColor(0.6f, 0.6f, 0.58f);		// plaster
	const FLinearColor SmoothColor(0.35f, 0.35f, 0.4f);	// smooth stone = no climbing
}

template <typename T>
T* ATraversalTestCourse::SpawnLocal(const FVector& LocalLocation, const FRotator& LocalRotation)
{
	const FTransform World = FTransform(LocalRotation, LocalLocation) * GetActorTransform();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<T>(T::StaticClass(), World, Params);
}

template <typename T>
T* ATraversalTestCourse::SpawnLocalWith(const FVector& LocalLocation, const FRotator& LocalRotation, TFunctionRef<void(T*)> Setup)
{
	// Deferred so Setup runs before the actor's construction script and BeginPlay.
	const FTransform World = FTransform(LocalRotation, LocalLocation) * GetActorTransform();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;
	T* Actor = GetWorld()->SpawnActor<T>(T::StaticClass(), World, Params);
	if (Actor)
	{
		Setup(Actor);
		UGameplayStatics::FinishSpawningActor(Actor, World);
	}
	return Actor;
}

ATraversalTestCourse::ATraversalTestCourse()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;

	GuardClass = AShinobiEnemy::StaticClass();
}

void ATraversalTestCourse::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	for (USceneComponent* Old : Generated)
	{
		if (IsValid(Old))
		{
			Old->DestroyComponent();
		}
	}
	Generated.Reset();

	const FRotator Flat = FRotator::ZeroRotator;

	if (bBuildFloor)
	{
		AddBlock(TEXT("Floor"), FVector(0.f, 0.f, -10.f), FVector(10000.f, 10000.f, 20.f), Flat, FloorColor, NAME_None);
	}

	// 0. Vault: low crate, Space while running at it.
	AddBlock(TEXT("VaultCrate"), FVector(0.f, -1000.f, 60.f), FVector(200.f, 200.f, 120.f), Flat, ClimbColor, NAME_None);
	AddLabel(TEXT("Label0"), FVector(-150.f, -1000.f, 200.f), TEXT("0. VAULT  (Space while running)"));

	// 1. Climbing wall: marked (wood) block, no way up for guards.
	AddBlock(TEXT("ClimbBlock"), FVector(1800.f, 0.f, 225.f), FVector(800.f, 1200.f, 450.f), Flat, ClimbColor, ClimbableTag);
	AddLabel(TEXT("Label1"), FVector(1350.f, 0.f, 520.f), TEXT("1. CLIMB  (Space at the wooden wall, W up)"));

	// 2. Ledge wall: plaster, cannot be climbed, but its top edge can be grabbed.
	AddBlock(TEXT("LedgeWall"), FVector(600.f, 1800.f, 140.f), FVector(60.f, 1000.f, 280.f), Flat, PlainColor, NAME_None);
	AddLabel(TEXT("Label2"), FVector(500.f, 1800.f, 340.f), TEXT("2. LEDGE  (jump and hold W, A/D along the edge)"));

	// 3. Kaginawa tower: smooth stone, only the hook gets you up.
	AddBlock(TEXT("Tower"), FVector(1800.f, 2600.f, 450.f), FVector(600.f, 600.f, 900.f), Flat, SmoothColor, NoClimbTag);
	AddLabel(TEXT("Label3"), FVector(1400.f, 2600.f, 980.f), TEXT("3. KAGINAWA  (look at the ball, Q)"));

	// 4. Rope wall.
	AddBlock(TEXT("RopeBlock"), FVector(1800.f, -2600.f, 300.f), FVector(600.f, 600.f, 600.f), Flat, SmoothColor, NoClimbTag);
	AddLabel(TEXT("Label4"), FVector(1400.f, -2600.f, 680.f), TEXT("4. ROPE  (E, then W)"));

	// 5. Cliff with a long ramp: guards can walk round, the ladder is the short way down.
	AddBlock(TEXT("Cliff"), FVector(-3200.f, -2000.f, 350.f), FVector(1200.f, 1200.f, 700.f), Flat, SmoothColor, NoClimbTag);
	const float RampRise = 700.f;
	const float RampRun = 3500.f;
	const float RampLength = FMath::Sqrt(RampRise * RampRise + RampRun * RampRun);
	const float RampPitch = -FMath::RadiansToDegrees(FMath::Atan2(RampRise, RampRun));
	AddBlock(TEXT("Ramp"), FVector(-2600.f + RampRun * 0.5f, -2000.f, RampRise * 0.5f - 20.f), FVector(RampLength, 400.f, 40.f), FRotator(RampPitch, 0.f, 0.f), PlainColor, NAME_None);
	AddLabel(TEXT("Label5"), FVector(-3200.f, -2000.f, 800.f), TEXT("5. ROPE LADDER  (R at the cliff edge)"));

	// 6. Tightrope between two pillars; the far pillar is a dead end only the shinobi can reach.
	AddBlock(TEXT("PillarA"), FVector(-2600.f, 2600.f, 250.f), FVector(300.f, 300.f, 500.f), Flat, ClimbColor, ClimbableTag);
	AddBlock(TEXT("PillarB"), FVector(-1000.f, 2600.f, 250.f), FVector(300.f, 300.f, 500.f), Flat, SmoothColor, NoClimbTag);
	AddLabel(TEXT("Label6"), FVector(-1800.f, 2600.f, 650.f), TEXT("6. TIGHTROPE  (E at the pillar edge)"));

	AddLabel(TEXT("LabelMaster"), FVector(-900.f, -400.f, 280.f), TEXT("Spend escape points with the village master"));

	// 7. Town escape: walled courtyard with the magistrate's ledger, a back gate, an oil jar on
	// the stones and a log leaned against a storehouse roof (the log itself is spawned at play).
	const float WallHeight = 400.f;
	const float WallZ = WallHeight * 0.5f;
	AddBlock(TEXT("TownWallW1"), FVector(3400.f, -450.f, WallZ), FVector(60.f, 500.f, WallHeight), Flat, PlainColor, NoClimbTag);
	AddBlock(TEXT("TownWallW2"), FVector(3400.f, 450.f, WallZ), FVector(60.f, 500.f, WallHeight), Flat, PlainColor, NoClimbTag);
	AddBlock(TEXT("TownWallE"), FVector(4400.f, 0.f, WallZ), FVector(60.f, 1460.f, WallHeight), Flat, PlainColor, NoClimbTag);
	AddBlock(TEXT("TownWallS"), FVector(3900.f, -700.f, WallZ), FVector(1060.f, 60.f, WallHeight), Flat, PlainColor, NoClimbTag);
	AddBlock(TEXT("TownWallN1"), FVector(3675.f, 700.f, WallZ), FVector(550.f, 60.f, WallHeight), Flat, PlainColor, NoClimbTag);
	AddBlock(TEXT("TownWallN2"), FVector(4325.f, 700.f, WallZ), FVector(150.f, 60.f, WallHeight), Flat, PlainColor, NoClimbTag);
	AddBlock(TEXT("Storehouse"), FVector(4100.f, 2300.f, 225.f), FVector(800.f, 600.f, 450.f), Flat, PlainColor, NoClimbTag);
	AddLabel(TEXT("Label7"), FVector(3250.f, 0.f, 480.f), TEXT("7. TOWN ESCAPE  (steal the ledger, run the route)"));
	AddLabel(TEXT("Label7b"), FVector(3250.f, 0.f, 430.f), TEXT("Rehearse it calmly first: rehearsed escapes pay more"));
}

void ATraversalTestCourse::BeginPlay()
{
	Super::BeginPlay();

	// Rope hanging on station 4.
	if (AShinobiRope* Rope = SpawnLocal<AShinobiRope>(FVector(1485.f, -2600.f, 600.f), FRotator::ZeroRotator))
	{
		Rope->SetupRope(600.f, false, true);
	}

	// Kaginawa anchor on the tower's front edge, facing onto the roof.
	SpawnLocal<AShinobiGrapplePoint>(FVector(1505.f, 2600.f, 915.f), FRotator::ZeroRotator);

	// Tightrope from pillar A to pillar B.
	if (AShinobiTightrope* Tightrope = SpawnLocal<AShinobiTightrope>(FVector(-2455.f, 2600.f, 500.f), FRotator::ZeroRotator))
	{
		Tightrope->SetEnds(GetActorTransform().TransformPosition(FVector(-2455.f, 2600.f, 500.f)),
			GetActorTransform().TransformPosition(FVector(-1145.f, 2600.f, 500.f)));
	}

	// Village master (hunter: climbing and ropes).
	SpawnLocal<AShinobiTrainer>(FVector(-900.f, -400.f, 0.f), FRotator(0.f, 180.f, 0.f));

	// Station 7: the escape route out of the town and its two traps.
	SpawnLocalWith<AShinobiEscapeRoute>(FVector::ZeroVector, FRotator::ZeroRotator, [](AShinobiEscapeRoute* Route)
	{
		Route->RouteName = TEXT("Storehouse roof");
		Route->Checkpoints = {
			FVector(4100.f, 780.f, 0.f),	// back gate
			FVector(4100.f, 1100.f, 0.f),	// oiled stones
			FVector(4100.f, 1350.f, 0.f),	// foot of the log
			FVector(4100.f, 2450.f, 450.f)	// far side of the storehouse roof
		};
	});
	SpawnLocalWith<AShinobiOilTrap>(FVector(3850.f, 1000.f, 0.f), FRotator::ZeroRotator, [](AShinobiOilTrap* Oil)
	{
		Oil->SlickOffset = FVector(250.f, 100.f, 0.f);
		Oil->SlickSize = FVector2D(400.f, 300.f);
	});
	SpawnLocalWith<AShinobiDropLog>(FVector(4100.f, 1300.f, 0.f), FRotator::ZeroRotator, [](AShinobiDropLog* Log)
	{
		Log->TopPoint = FVector(0.f, 700.f, 450.f);
	});
	SpawnLocal<AShinobiAlarmObjective>(FVector(4200.f, -450.f, 0.f), FRotator::ZeroRotator);

	if (bSpawnGuards && GuardClass)
	{
		SpawnGuard(EGuardType::Spearman, FVector(700.f, 700.f, 100.f));
		SpawnGuard(EGuardType::Samurai, FVector(700.f, -700.f, 100.f));
		SpawnGuard(EGuardType::Dog, FVector(-1500.f, 700.f, 60.f));
		SpawnGuard(EGuardType::Archer, FVector(-1200.f, -1300.f, 100.f));

		// Town guards.
		SpawnGuard(EGuardType::Spearman, FVector(3800.f, -300.f, 100.f));
		SpawnGuard(EGuardType::Spearman, FVector(4000.f, 300.f, 100.f));
		SpawnGuard(EGuardType::Samurai, FVector(3200.f, 0.f, 100.f));
	}

	if (bMovePlayerToStart)
	{
		// The player pawn may not exist yet on the first frame.
		GetWorldTimerManager().SetTimer(StartTimer, this, &ATraversalTestCourse::MovePlayerToStart, 0.2f, false);
	}
}

void ATraversalTestCourse::SpawnGuard(EGuardType Type, const FVector& LocalLocation)
{
	const FTransform World = FTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), LocalLocation) * GetActorTransform();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.bDeferConstruction = true;
	if (AShinobiEnemy* Guard = GetWorld()->SpawnActor<AShinobiEnemy>(GuardClass, World, Params))
	{
		Guard->GuardType = Type;
		UGameplayStatics::FinishSpawningActor(Guard, World);
	}
}

void ATraversalTestCourse::MovePlayerToStart()
{
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const FVector Start = GetActorTransform().TransformPosition(PlayerStart);
		Player->TeleportTo(Start, GetActorRotation());
		if (AController* Controller = Player->GetController())
		{
			Controller->SetControlRotation(GetActorRotation());
		}
	}
}

UStaticMeshComponent* ATraversalTestCourse::AddBlock(const TCHAR* Name, const FVector& Center, const FVector& Size, const FRotator& Rotation, const FLinearColor& Color, FName Tag)
{
	UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this, Name);
	Block->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Block->SetupAttachment(Root);
	Block->SetStaticMesh(CubeMesh);
	Block->SetRelativeLocationAndRotation(Center, Rotation);
	Block->SetRelativeScale3D(Size / 100.f);
	Block->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	if (!Tag.IsNone())
	{
		Block->ComponentTags.Add(Tag);
	}
	Block->RegisterComponent();
	ShinobiVisuals::TintMesh(Block, Color);
	Generated.Add(Block);
	return Block;
}

void ATraversalTestCourse::AddLabel(const TCHAR* Name, const FVector& Location, const FString& Text)
{
	UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this, Name);
	Label->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Label->SetupAttachment(Root);
	Label->SetRelativeLocationAndRotation(Location, FRotator(0.f, 180.f, 0.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(45.f);
	Label->SetTextRenderColor(FColor(255, 240, 200));
	Label->SetText(FText::FromString(Text));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->RegisterComponent();
	Generated.Add(Label);
}
