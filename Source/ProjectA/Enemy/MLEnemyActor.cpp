// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLEnemyActor.h"

#include "Components/StaticMeshComponent.h"
#include "Enemy/MLEnemySubsystem.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Skills/MLSkillVisualActor.h"

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

void AMLEnemyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideTelegraph();
	Super::EndPlay(EndPlayReason);
}

void AMLEnemyActor::ShowTelegraph(TSubclassOf<AMLSkillVisualActor> TelegraphClass, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	HideTelegraph();
	if (!TelegraphClass) return;

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Telegraph = GetWorld()->SpawnActor<AMLSkillVisualActor>(TelegraphClass, Location, Rotation, SpawnInfo);
	if (Telegraph)
	{
		Telegraph->SetActorScale3D(Scale);
	}
}

void AMLEnemyActor::HideTelegraph()
{
	if (Telegraph)
	{
		Telegraph->Destroy();
		Telegraph = nullptr;
	}
}

UMLEnemySubsystem* AMLEnemyActor::GetEnemySubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UMLEnemySubsystem>() : nullptr;
}

bool AMLEnemyActor::GetHeroLocation(FVector& OutLocation) const
{
	const AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Hero || Hero->IsDead()) return false;

	OutLocation = Hero->GetActorLocation();
	return true;
}
