// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLBossActor.h"

#include "Enemy/MLEnemySubsystem.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Skills/MLSkillVisualActor.h"
#include "TimerManager.h"

void AMLBossActor::StartBoss(int32 InEnemyIndex)
{
	EnemyIndex = InEnemyIndex;
	EnrageStacks = 0;
	LastPattern = EMLBossPattern::None;

	GetWorldTimerManager().SetTimer(EnrageTimerHandle, this, &AMLBossActor::AddEnrage, EnrageInterval, true, EnrageStartSeconds);
	ScheduleNextPattern();
}

void AMLBossActor::StopBoss()
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	HideTelegraph();
}

void AMLBossActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideTelegraph();
	Super::EndPlay(EndPlayReason);
}

void AMLBossActor::ScheduleNextPattern()
{
	const float Gap = PatternGap * FMath::Max(0.2f, 1.f - EnrageGapStep * EnrageStacks);
	GetWorldTimerManager().SetTimer(PatternTimerHandle, this, &AMLBossActor::BeginPattern, Gap, false);
}

void AMLBossActor::BeginPattern()
{
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	FVector HeroLoc;
	if (!EnemySub || !EnemySub->IsEnemyActive(EnemyIndex) || !GetHeroLocation(HeroLoc)) return;

	const float HealthPercent = EnemySub->GetEnemyHealthPercent(EnemyIndex);
	TArray<EMLBossPattern> Options;
	Options.Add(EMLBossPattern::Charge);
	Options.Add(EMLBossPattern::Roar);
	if (SummonClass && HealthPercent <= SummonHealthPercent) Options.Add(EMLBossPattern::Summon);
	if (Options.Num() > 1) Options.Remove(LastPattern); // 같은 패턴을 연속으로 쓰지 않음

	const EMLBossPattern Pattern = Options[FMath::RandRange(0, Options.Num() - 1)];
	LastPattern = Pattern;

	const FVector Floor = GetActorLocation() + FVector(0.f, 0.f, TelegraphZOffset);
	switch (Pattern)
	{
	case EMLBossPattern::Charge:
		{
			ChargeDir = (HeroLoc - GetActorLocation()).GetSafeNormal2D();
			if (ChargeDir.IsNearlyZero()) ChargeDir = GetActorForwardVector().GetSafeNormal2D();
			EnemySub->SetEnemyRooted(EnemyIndex, true);

			const float Length = ChargeSpeed * ChargeSeconds;
			ShowTelegraph(LineTelegraphClass, Floor + ChargeDir * (Length * 0.5f), ChargeDir.Rotation(),
				FVector(Length / 100.f, ChargeTelegraphWidth / 100.f, 1.f)); // 엔진 평면 = 100 × 100
			GetWorldTimerManager().SetTimer(PatternTimerHandle, this, &AMLBossActor::ExecuteCharge, ChargeTelegraphSeconds, false);
			break;
		}
	case EMLBossPattern::Roar:
		EnemySub->SetEnemyRooted(EnemyIndex, true);
		ShowTelegraph(RingTelegraphClass, Floor, FRotator::ZeroRotator, FVector(RoarRadius * 2.f / 100.f, RoarRadius * 2.f / 100.f, 1.f));
		GetWorldTimerManager().SetTimer(PatternTimerHandle, this, &AMLBossActor::ExecuteRoar, RoarTelegraphSeconds, false);
		break;
	default:
		DoSummon();
		ScheduleNextPattern();
		break;
	}
}

void AMLBossActor::ExecuteCharge()
{
	HideTelegraph();
	if (UMLEnemySubsystem* EnemySub = GetEnemySubsystem())
	{
		EnemySub->LaunchEnemy(EnemyIndex, ChargeDir * ChargeSpeed, ChargeSeconds); // 고정(root)도 함께 해제
	}
	GetWorldTimerManager().SetTimer(PatternTimerHandle, this, &AMLBossActor::ScheduleNextPattern, ChargeSeconds, false);
}

void AMLBossActor::ExecuteRoar()
{
	HideTelegraph();
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!EnemySub) return;

	EnemySub->SetEnemyRooted(EnemyIndex, false);
	FVector HeroLoc;
	if (GetHeroLocation(HeroLoc))
	{
		const float Dist = FVector::Dist2D(HeroLoc, GetActorLocation());
		if (Dist > RoarSafeRadius && Dist <= RoarRadius) EnemySub->DamageHero(RoarDamage);
	}
	ScheduleNextPattern();
}

void AMLBossActor::DoSummon()
{
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!EnemySub) return;

	for (int32 i = 0; i < SummonCount; ++i)
	{
		const float Angle = UE_TWO_PI * i / SummonCount;
		EnemySub->SpawnEnemy(SummonClass, GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * SummonRadius);
	}
}

void AMLBossActor::AddEnrage()
{
	if (EnrageStacks >= EnrageMaxStacks)
	{
		GetWorldTimerManager().ClearTimer(EnrageTimerHandle);
		return;
	}
	++EnrageStacks;

	const float NewSpeed = MoveSpeed * (1.f + EnrageSpeedStep * EnrageStacks);
	if (UMLEnemySubsystem* EnemySub = GetEnemySubsystem())
	{
		EnemySub->SetEnemyMoveSpeed(EnemyIndex, NewSpeed);
	}
}

void AMLBossActor::ShowTelegraph(TSubclassOf<AMLSkillVisualActor> TelegraphClass, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	HideTelegraph();
	if (!TelegraphClass) return; // 경고 표시 없이 패턴만 실행

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Telegraph = GetWorld()->SpawnActor<AMLSkillVisualActor>(TelegraphClass, Location, Rotation, SpawnInfo);
	if (Telegraph)
	{
		Telegraph->SetActorScale3D(Scale);
	}
}

void AMLBossActor::HideTelegraph()
{
	if (Telegraph)
	{
		Telegraph->Destroy();
		Telegraph = nullptr;
	}
}

UMLEnemySubsystem* AMLBossActor::GetEnemySubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UMLEnemySubsystem>() : nullptr;
}

bool AMLBossActor::GetHeroLocation(FVector& OutLocation) const
{
	const AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Hero || Hero->IsDead()) return false;

	OutLocation = Hero->GetActorLocation();
	return true;
}
