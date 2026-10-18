#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShinobiCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UShinobiTraversalComponent;
class UEscapeScoreComponent;
class AShinobiEnemy;
struct FInputActionValue;

/**
 * The player shinobi: third-person camera, run / sprint / crouch, traversal and escape score.
 * Works out of the box: if no input assets are assigned, it builds its own key bindings at runtime.
 */
UCLASS()
class WHITEMOUNTAIN_API AShinobiCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AShinobiCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	UShinobiTraversalComponent* GetTraversal() const { return Traversal; }
	UEscapeScoreComponent* GetEscapeScore() const { return EscapeScore; }
	bool IsStunned() const { return bStunned; }

	/** Knock the shinobi back (used when a guard catches him). */
	void ReceiveCatch(const FVector& FromLocation);

	/** An arrow hit: lose points and limp for a moment. No blood, the design doc keeps it low. */
	void ReceiveArrowHit(const FVector& FromLocation);

	bool IsWounded() const;

	// --- Takedowns: E = Silent Sleep (slow, silent), F = Pressure Strike (fast, he groans) ---
	/** The guard the shinobi could take down right now, or null. */
	AShinobiEnemy* FindTakedownTarget() const;

	bool IsInTakedown() const { return bInTakedown; }

	/** 0..1 while a takedown is under way. */
	float GetTakedownProgress() const;

	/** In-game name of the takedown under way. */
	const TCHAR* GetTakedownName() const;

	/** Hadaka jime (rear choke). Slow and completely silent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Takedown")
	float SilentSleepSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Takedown")
	float SilentSleepKnockoutSeconds = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Takedown")
	int32 SilentSleepPoints = 40;

	/** Shikan ken into Dokko (behind the ear). Quick, but he groans and nearby guards hear it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Takedown")
	float PressureStrikeSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Takedown")
	float PressureStrikeKnockoutSeconds = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Takedown")
	int32 PressureStrikePoints = 20;

	/** Guards this close hear the groan and come to look. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Takedown")
	float PressureStrikeNoiseRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Arrows")
	float ArrowSlowSeconds = 2.5f;

	/** Run and sprint speed are multiplied by this while wounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Arrows")
	float ArrowSlowFactor = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Arrows")
	int32 ArrowHitPenalty = 20;

	// --- Movement tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Movement")
	float RunSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Movement")
	float SprintSpeed = 700.f;

	/** Falls higher than this hurt (stun) unless you roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Fall")
	float HardFallHeight = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Fall")
	float HardFallStunSeconds = 1.6f;

	/** Press Drop this long before landing to roll (ukemi) and take no stun. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Fall")
	float UkemiWindowSeconds = 0.45f;

	/** Above this even ukemi cannot save you from a stun. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shinobi|Fall")
	float UkemiMaxHeight = 1000.f;

	// --- Optional input assets (leave empty to use the built-in bindings) ---
	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> DropAction;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> RopeLadderAction;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> KaginawaAction;

	UPROPERTY(EditAnywhere, Category = "Shinobi|Input")
	TObjectPtr<UInputAction> StrikeAction;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shinobi")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shinobi")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shinobi")
	TObjectPtr<UShinobiTraversalComponent> Traversal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shinobi")
	TObjectPtr<UEscapeScoreComponent> EscapeScore;

	/** Grey-box body, hidden automatically when the UE mannequin is available. */
	UPROPERTY(VisibleAnywhere, Category = "Shinobi")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, Category = "Shinobi")
	TObjectPtr<UStaticMeshComponent> PlaceholderFace;

private:
	void CreateFallbackInput();

	void Move(const FInputActionValue& Value);
	void MoveCompleted(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void JumpPressed();
	void JumpReleased();
	void SprintPressed();
	void SprintReleased();
	void DropPressed();
	void InteractPressed();
	void RopeLadderPressed();
	void KaginawaPressed();
	void StrikePressed();

	/** Stunned or busy with a takedown: no other input. */
	bool IsBusy() const { return bStunned || IsInTakedown(); }

	void StartTakedown(AShinobiEnemy* Target, bool bSilent);
	void TickTakedown(float DeltaSeconds);
	void CompleteTakedown();
	/** Let go before the end (caught, shot, fell): the guard tears free and raises the alarm. */
	void CancelTakedown();

	void Stun(float Seconds, const FString& Reason);
	void EndStun();

	bool bWantsSprint = false;
	float WoundedUntil = -1.f;
	bool bStunned = false;
	float FallApexZ = 0.f;
	float LastDropPressTime = -1000.f;
	FTimerHandle StunTimer;

	TWeakObjectPtr<AShinobiEnemy> TakedownTarget;
	bool bInTakedown = false;
	bool bTakedownSilent = true;
	float TakedownTime = 0.f;
};
