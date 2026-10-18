#pragma once

#include "CoreMinimal.h"

class ACharacter;
class UStaticMeshComponent;

/** Helpers so the prototype is playable with zero art: grey-box shapes, or the UE mannequin if it is installed. */
namespace ShinobiVisuals
{
	/** Uses the Third Person mannequin + its anim blueprint if that content pack exists. Returns true on success. */
	bool TryApplyMannequin(ACharacter* Character);

	/** Tints a BasicShapes mesh through its "Color" material parameter. */
	void TintMesh(UStaticMeshComponent* Mesh, const FLinearColor& Color);

	/** Grey-box parts never block anything or change the navmesh. */
	void MakeCosmetic(UStaticMeshComponent* Mesh);
}
