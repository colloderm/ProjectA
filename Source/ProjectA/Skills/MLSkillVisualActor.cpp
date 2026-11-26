// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLSkillVisualActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

AMLSkillVisualActor::AMLSkillVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	RootComponent = VisualRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(VisualRoot);
	VisualMesh->SetCollisionProfileName(TEXT("NoCollision"));
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCanEverAffectNavigation(false);
}

void AMLSkillVisualActor::SetOverlay(UMaterialInterface* Overlay)
{
	ForEachComponent<UMeshComponent>(false, [Overlay](UMeshComponent* Mesh) { Mesh->SetOverlayMaterial(Overlay); });
}
