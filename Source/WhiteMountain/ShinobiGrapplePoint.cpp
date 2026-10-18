#include "ShinobiGrapplePoint.h"
#include "ShinobiVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AShinobiGrapplePoint::AShinobiGrapplePoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Root);
	Marker->SetStaticMesh(SphereMesh.Object);
	Marker->SetRelativeScale3D(FVector(0.18f));
	ShinobiVisuals::MakeCosmetic(Marker);
}
