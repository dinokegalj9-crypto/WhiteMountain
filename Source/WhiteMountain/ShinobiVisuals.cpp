#include "ShinobiVisuals.h"
#include "WhiteMountain.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	// Paths used by the Third Person content pack in recent UE5 versions. The first one that exists wins.
	const TCHAR* MannequinMeshPaths[] =
	{
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"),
	};

	const TCHAR* MannequinAnimPaths[] =
	{
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C"),
	};

	bool PackageExists(const FSoftObjectPath& Path)
	{
		return FPackageName::DoesPackageExist(Path.GetLongPackageName());
	}
}

bool ShinobiVisuals::TryApplyMannequin(ACharacter* Character)
{
	if (!Character || !Character->GetMesh())
	{
		return false;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (MeshComp->GetSkeletalMeshAsset())
	{
		// A Blueprint child already assigned its own mesh.
		return true;
	}

	USkeletalMesh* FoundMesh = nullptr;
	for (const TCHAR* Path : MannequinMeshPaths)
	{
		const FSoftObjectPath SoftPath(Path);
		if (PackageExists(SoftPath))
		{
			FoundMesh = Cast<USkeletalMesh>(SoftPath.TryLoad());
			if (FoundMesh)
			{
				break;
			}
		}
	}
	if (!FoundMesh)
	{
		return false;
	}

	MeshComp->SetSkeletalMeshAsset(FoundMesh);

	for (const TCHAR* Path : MannequinAnimPaths)
	{
		const FSoftClassPath ClassPath(Path);
		if (PackageExists(ClassPath))
		{
			if (UClass* AnimClass = ClassPath.TryLoadClass<UAnimInstance>())
			{
				MeshComp->SetAnimInstanceClass(AnimClass);
				break;
			}
		}
	}

	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	MeshComp->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -HalfHeight), FRotator(0.f, -90.f, 0.f));
	UE_LOG(LogWhiteMountain, Log, TEXT("%s uses mannequin %s"), *Character->GetName(), *FoundMesh->GetName());
	return true;
}

void ShinobiVisuals::TintMesh(UStaticMeshComponent* Mesh, const FLinearColor& Color)
{
	if (!Mesh || !Mesh->GetStaticMesh())
	{
		return;
	}
	if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ShinobiVisuals::MakeCosmetic(UStaticMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetGenerateOverlapEvents(false);
}
