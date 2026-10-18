#include "ShinobiCharacter.h"
#include "WhiteMountain.h"
#include "ShinobiVisuals.h"
#include "ShinobiTraversalComponent.h"
#include "EscapeScoreComponent.h"
#include "ShinobiTrainer.h"
#include "ShinobiInteractable.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AShinobiCharacter::AShinobiCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 92.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 600.f, 0.f);
	MoveComp->JumpZVelocity = 560.f;
	MoveComp->AirControl = 0.35f;
	MoveComp->MaxWalkSpeed = RunSpeed;
	MoveComp->MaxWalkSpeedCrouched = 220.f;
	MoveComp->MinAnalogWalkSpeed = 20.f;
	MoveComp->BrakingDecelerationWalking = 2000.f;
	MoveComp->BrakingDecelerationFalling = 1500.f;
	MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 380.f;
	CameraBoom->SocketOffset = FVector(0.f, 45.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Traversal = CreateDefaultSubobject<UShinobiTraversalComponent>(TEXT("Traversal"));
	EscapeScore = CreateDefaultSubobject<UEscapeScoreComponent>(TEXT("EscapeScore"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetStaticMesh(CylinderMesh.Object);
	PlaceholderBody->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.84f));
	ShinobiVisuals::MakeCosmetic(PlaceholderBody);

	PlaceholderFace = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderFace"));
	PlaceholderFace->SetupAttachment(GetCapsuleComponent());
	PlaceholderFace->SetStaticMesh(CubeMesh.Object);
	PlaceholderFace->SetRelativeLocation(FVector(38.f, 0.f, 60.f));
	PlaceholderFace->SetRelativeScale3D(FVector(0.15f, 0.5f, 0.12f));
	ShinobiVisuals::MakeCosmetic(PlaceholderFace);
}

void AShinobiCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

	if (ShinobiVisuals::TryApplyMannequin(this))
	{
		PlaceholderBody->SetVisibility(false);
		PlaceholderFace->SetVisibility(false);
	}
	else
	{
		ShinobiVisuals::TintMesh(PlaceholderBody, FLinearColor(0.04f, 0.04f, 0.09f));
		ShinobiVisuals::TintMesh(PlaceholderFace, FLinearColor(0.7f, 0.1f, 0.1f));
	}
}

void AShinobiCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp->IsFalling())
	{
		FallApexZ = FMath::Max(FallApexZ, GetActorLocation().Z);
	}

	// Sprinting is loud and costs stamina (design doc).
	const bool bMoving = MoveComp->IsMovingOnGround() && MoveComp->Velocity.SizeSquared2D() > FMath::Square(100.f);
	const bool bSprinting = bWantsSprint && bMoving && !bIsCrouched && !Traversal->IsTraversing()
		&& Traversal->ConsumeSprintStamina(DeltaSeconds);
	MoveComp->MaxWalkSpeed = (bSprinting ? SprintSpeed : RunSpeed) * (IsWounded() ? ArrowSlowFactor : 1.f);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void AShinobiCharacter::CreateFallbackInput()
{
	auto MakeAction = [this](TObjectPtr<UInputAction>& Slot, const TCHAR* Name, EInputActionValueType Type)
	{
		if (!Slot)
		{
			Slot = NewObject<UInputAction>(this, Name);
			Slot->ValueType = Type;
		}
	};
	MakeAction(MoveAction, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	MakeAction(LookAction, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	MakeAction(JumpAction, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	MakeAction(SprintAction, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	MakeAction(DropAction, TEXT("IA_Drop"), EInputActionValueType::Boolean);
	MakeAction(InteractAction, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	MakeAction(RopeLadderAction, TEXT("IA_RopeLadder"), EInputActionValueType::Boolean);
	MakeAction(KaginawaAction, TEXT("IA_Kaginawa"), EInputActionValueType::Boolean);

	if (DefaultMappingContext)
	{
		return;	// a designer supplied their own mapping asset
	}

	UInputMappingContext* Context = NewObject<UInputMappingContext>(this, TEXT("IMC_ShinobiFallback"));

	// WASD into a 2D axis: X = right, Y = forward.
	auto MapMoveKey = [this, Context](const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
	};
	MapMoveKey(EKeys::W, true, false);
	MapMoveKey(EKeys::S, true, true);
	MapMoveKey(EKeys::D, false, false);
	MapMoveKey(EKeys::A, false, true);
	Context->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	// Mouse / right stick: invert Y so pushing up looks up.
	auto MapLookKey = [this, Context](const FKey& Key)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(LookAction, Key);
		UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(Context);
		NegateY->bX = false;
		NegateY->bY = true;
		NegateY->bZ = false;
		Mapping.Modifiers.Add(NegateY);
	};
	MapLookKey(EKeys::Mouse2D);
	MapLookKey(EKeys::Gamepad_Right2D);

	Context->MapKey(JumpAction, EKeys::SpaceBar);
	Context->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	Context->MapKey(SprintAction, EKeys::LeftShift);
	Context->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	Context->MapKey(DropAction, EKeys::C);
	Context->MapKey(DropAction, EKeys::Gamepad_FaceButton_Right);
	Context->MapKey(InteractAction, EKeys::E);
	Context->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
	Context->MapKey(RopeLadderAction, EKeys::R);
	Context->MapKey(RopeLadderAction, EKeys::Gamepad_FaceButton_Top);
	Context->MapKey(KaginawaAction, EKeys::Q);
	Context->MapKey(KaginawaAction, EKeys::Gamepad_RightShoulder);

	DefaultMappingContext = Context;
}

void AShinobiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	CreateFallbackInput();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogWhiteMountain, Error, TEXT("Enhanced Input is not the default input component. Check Config/DefaultInput.ini."));
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShinobiCharacter::Move);
	Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AShinobiCharacter::MoveCompleted);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShinobiCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AShinobiCharacter::JumpPressed);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AShinobiCharacter::JumpReleased);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AShinobiCharacter::SprintPressed);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AShinobiCharacter::SprintReleased);
	Input->BindAction(DropAction, ETriggerEvent::Started, this, &AShinobiCharacter::DropPressed);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AShinobiCharacter::InteractPressed);
	Input->BindAction(RopeLadderAction, ETriggerEvent::Started, this, &AShinobiCharacter::RopeLadderPressed);
	Input->BindAction(KaginawaAction, ETriggerEvent::Started, this, &AShinobiCharacter::KaginawaPressed);
}

void AShinobiCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	Traversal->SetMoveInput(Input);

	if (bStunned || Traversal->IsTraversing() || !Controller)
	{
		return;	// traversal moves the body itself
	}
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Input.X);
}

void AShinobiCharacter::MoveCompleted(const FInputActionValue& Value)
{
	Traversal->SetMoveInput(FVector2D::ZeroVector);
}

void AShinobiCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

void AShinobiCharacter::JumpPressed()
{
	if (bStunned || Traversal->HandleJumpPressed())
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	Jump();
}

void AShinobiCharacter::JumpReleased()
{
	StopJumping();
}

void AShinobiCharacter::SprintPressed()
{
	bWantsSprint = true;
	if (bIsCrouched)
	{
		UnCrouch();
	}
}

void AShinobiCharacter::SprintReleased()
{
	bWantsSprint = false;
}

void AShinobiCharacter::DropPressed()
{
	LastDropPressTime = GetWorld()->GetTimeSeconds();	// also the ukemi timing
	if (bStunned || Traversal->HandleDropPressed())
	{
		return;
	}
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		if (bIsCrouched)
		{
			UnCrouch();
		}
		else
		{
			Crouch();
		}
	}
}

void AShinobiCharacter::InteractPressed()
{
	if (bStunned || Traversal->HandleInteractPressed())
	{
		return;
	}

	// Closest usable thing in reach: oil jar, log, ledger...
	AShinobiInteractable* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<AShinobiInteractable> It(GetWorld()); It; ++It)
	{
		if (It->CanInteract(this))
		{
			const float Distance = FVector::Dist(GetActorLocation(), It->GetInteractLocation());
			if (Distance < BestDistance)
			{
				Best = *It;
				BestDistance = Distance;
			}
		}
	}
	if (Best)
	{
		Best->Interact(this);
		return;
	}

	// Nearby village trainer: spend escape points on a new move.
	for (TActorIterator<AShinobiTrainer> It(GetWorld()); It; ++It)
	{
		if (It->CanTrain(this))
		{
			It->Train(this);
			return;
		}
	}
}

void AShinobiCharacter::RopeLadderPressed()
{
	if (!bStunned)
	{
		Traversal->HandleRopeLadderPressed();
	}
}

void AShinobiCharacter::KaginawaPressed()
{
	if (!bStunned)
	{
		Traversal->HandleKaginawaPressed();
	}
}

// ---------------------------------------------------------------------------
// Falls, ukemi, stun
// ---------------------------------------------------------------------------

void AShinobiCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	if (GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		FallApexZ = GetActorLocation().Z;
	}
}

void AShinobiCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	const float FallHeight = FallApexZ - GetActorLocation().Z;
	if (FallHeight < HardFallHeight)
	{
		return;
	}

	const bool bRolled = GetWorld()->GetTimeSeconds() - LastDropPressTime <= UkemiWindowSeconds;
	if (bRolled && FallHeight <= UkemiMaxHeight)
	{
		// Ukemi: roll forward and keep running.
		LaunchCharacter(GetActorForwardVector() * 300.f, true, false);
		EscapeScore->PushMessage(FString::Printf(TEXT("Ukemi! %.1f m fall, no injury."), FallHeight / 100.f));
		return;
	}

	const float Seconds = HardFallStunSeconds * FMath::Clamp(FallHeight / HardFallHeight, 1.f, 2.5f);
	Stun(Seconds, FString::Printf(TEXT("Hard fall from %.1f m! (press C just before landing = ukemi)"), FallHeight / 100.f));
}

void AShinobiCharacter::ReceiveCatch(const FVector& FromLocation)
{
	const FVector Away = (GetActorLocation() - FromLocation).GetSafeNormal2D();
	LaunchCharacter(Away * 500.f + FVector(0.f, 0.f, 250.f), true, true);
}

void AShinobiCharacter::ReceiveArrowHit(const FVector& FromLocation)
{
	WoundedUntil = GetWorld()->GetTimeSeconds() + ArrowSlowSeconds;
	EscapeScore->AwardBonus(-ArrowHitPenalty, TEXT("Hit by an arrow!"));

	// A hit on the ground staggers you; on a wall or rope you just hold on.
	if (!Traversal->IsTraversing() && GetCharacterMovement()->IsMovingOnGround())
	{
		const FVector Away = (GetActorLocation() - FromLocation).GetSafeNormal2D();
		LaunchCharacter(Away * 250.f + FVector(0.f, 0.f, 80.f), true, false);
	}
}

bool AShinobiCharacter::IsWounded() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < WoundedUntil;
}

void AShinobiCharacter::Stun(float Seconds, const FString& Reason)
{
	bStunned = true;
	GetCharacterMovement()->DisableMovement();
	EscapeScore->PushMessage(Reason);
	GetWorldTimerManager().SetTimer(StunTimer, this, &AShinobiCharacter::EndStun, Seconds, false);
}

void AShinobiCharacter::EndStun()
{
	bStunned = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
