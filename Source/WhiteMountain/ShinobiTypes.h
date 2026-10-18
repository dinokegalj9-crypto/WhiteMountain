#pragma once

#include "CoreMinimal.h"
#include "ShinobiTypes.generated.h"

/** What the shinobi's body is doing outside of normal walking / jumping. */
UENUM(BlueprintType)
enum class EShinobiTraversalState : uint8
{
	None,
	Climbing,
	LedgeHang,
	Mantling,
	OnRope,
	Balancing,	// walking on a tightrope
	Deploying	// throwing the kaginawa or tying a rope ladder
};

/** Traversal moves the escape score remembers. Keep NumShinobiMoves in sync. */
UENUM(BlueprintType)
enum class EShinobiMove : uint8
{
	Climb,
	LedgeGrab,
	Mantle,
	WallJump,
	Rope,
	RopeLadder,
	Kaginawa,
	Tightrope
};
static constexpr int32 NumShinobiMoves = 8;

/** Why a pursuer gave up. */
UENUM(BlueprintType)
enum class EEscapeReason : uint8
{
	Unreachable,	// the shinobi is somewhere the pursuer has no route to
	Detour,			// a route exists but it is far too long to be worth it
	LostTrail		// the pursuer lost sight and searched in vain
};

/** Alert level of an enemy. */
UENUM(BlueprintType)
enum class EEnemyAlert : uint8
{
	Patrol,
	Chase,
	Search,
	Frustrated,	// standing below the shinobi's route, unable to follow
	Return
};

/** What a village master teaches. Only Traversal has content in the prototype. */
UENUM(BlueprintType)
enum class EMasterSkill : uint8
{
	Traversal,	// hunter: climbing and ropes
	Weapons,	// smith
	Herbs		// herbalist
};

/** Guard types from the design doc. Each has different limits and is worth different escape points. */
UENUM(BlueprintType)
enum class EGuardType : uint8
{
	Spearman,	// cannot climb walls or jump between roofs; few points
	Samurai,	// armoured: slow, cannot climb at all; medium points
	Archer,		// slow chaser, but escaping his sight is worth more
	Dog			// fast, follows scent on the ground for a long time; many points
};
