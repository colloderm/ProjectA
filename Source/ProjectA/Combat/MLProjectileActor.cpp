// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLProjectileActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

AMLProjectileActor::AMLProjectileActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	RootComponent = ProjectileMesh;
	ProjectileMesh->SetCollisionProfileName(TEXT("NoCollision"));
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetCanEverAffectNavigation(false);
}

void AMLProjectileActor::SetOverlay(UMaterialInterface* Overlay)
{
	ProjectileMesh->SetOverlayMaterial(Overlay);
}
