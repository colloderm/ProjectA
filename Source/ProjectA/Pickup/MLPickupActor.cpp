// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLPickupActor.h"

#include "Components/StaticMeshComponent.h"

AMLPickupActor::AMLPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PickupRoot"));
	RootComponent = PickupRoot;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(PickupRoot);
	PickupMesh->SetCollisionProfileName(TEXT("NoCollision"));
	PickupMesh->SetGenerateOverlapEvents(false);
	PickupMesh->SetCanEverAffectNavigation(false);
}

