// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLTestEnemySpawner.h"

#include "Enemy/MLEnemyActor.h"
#include "Enemy/MLEnemySubsystem.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AMLTestEnemySpawner::AMLTestEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMLTestEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AMLTestEnemySpawner::SpawnBatch, SpawnInterval, true);
}

void AMLTestEnemySpawner::SpawnBatch()
{
	UMLEnemySubsystem* EnemySubsystem = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	const AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!EnemySubsystem || !Hero || !EnemyClass) return;

	if (Hero->IsDead())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	const FVector HeroLoc = Hero->GetActorLocation();
	for (int32 k = 0; k < SpawnPerTick && EnemySubsystem->GetActiveCount() < MaxAlive; ++k)
	{
		const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
		const FVector SpawnLoc = HeroLoc + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * SpawnRadius;
		if (!EnemySubsystem->SpawnEnemy(EnemyClass, SpawnLoc)) break;
	}
}
