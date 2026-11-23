// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLEnemyActor.h"

#include "Components/StaticMeshComponent.h"

AMLEnemyActor::AMLEnemyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	EnemyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EnemyRoot"));
	RootComponent = EnemyRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(EnemyRoot);
	VisualMesh->SetCollisionProfileName(TEXT("NoCollision")); // 명중은 서브시스템의 거리 판정으로 처리
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCanEverAffectNavigation(false);
}

void AMLEnemyActor::SetEliteLook(bool bElite)
{
	SetActorScale3D(FVector(bElite ? EliteScale : 1.f));
	VisualMesh->SetOverlayMaterial(bElite ? EliteOverlayMaterial.Get() : nullptr);
}
