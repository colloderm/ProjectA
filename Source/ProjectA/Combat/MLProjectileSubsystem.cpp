// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLProjectileSubsystem.h"

#include "Combat/MLProjectileActor.h"
#include "Enemy/MLEnemySubsystem.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"

bool UMLProjectileSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UMLProjectileSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMLProjectileSubsystem, STATGROUP_Tickables);
}

bool UMLProjectileSubsystem::Launch(const FMLProjectileLaunch& LaunchInfo)
{
	const TSubclassOf<AMLProjectileActor> ProjectileClass = LaunchInfo.ProjectileClass;
	if (!ProjectileClass) return false;

	FMLProjectileEntry* Slot = Projectiles.FindByPredicate([&ProjectileClass](const FMLProjectileEntry& P)
	{
		return !P.bActive && P.Actor && P.Actor->GetClass() == ProjectileClass;
	});

	if (!Slot)
	{
		if (Projectiles.Num() >= MaxProjectiles) return false;

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMLProjectileActor* NewProjectile = GetWorld()->SpawnActor<AMLProjectileActor>(ProjectileClass, LaunchInfo.Start, LaunchInfo.Velocity.Rotation(), SpawnInfo);
		if (!NewProjectile) return false;

		Slot = &Projectiles.AddDefaulted_GetRef();
		Slot->Actor = NewProjectile;
	}

	Slot->Velocity = LaunchInfo.Velocity;
	Slot->Damage = LaunchInfo.Damage;
	Slot->HitRadius = LaunchInfo.HitRadius;
	Slot->RangeLeft = LaunchInfo.MaxRange;
	Slot->SlowFactor = LaunchInfo.SlowFactor;
	Slot->SlowDuration = LaunchInfo.SlowDuration;
	Slot->HomingTurnRate = LaunchInfo.HomingTurnRate;
	Slot->Source = LaunchInfo.Source;
	Slot->bHostile = LaunchInfo.bHostile;
	Slot->bActive = true;
	Slot->Actor->SetOverlay(LaunchInfo.Overlay); // 풀에서 다시 쓰므로 매번 설정 (nullptr = 원래 모습)
	Slot->Actor->SetActorLocationAndRotation(LaunchInfo.Start, LaunchInfo.Velocity.Rotation());
	Slot->Actor->SetActorHiddenInGame(false);
	++ActiveCount;
	return true;
}

void UMLProjectileSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ActiveCount == 0) return;

	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const bool bHeroAlive = Hero && !Hero->IsDead();
	const FVector HeroLoc = bHeroAlive ? Hero->GetActorLocation() : FVector::ZeroVector;

	// design §6.2.3 망령의 탄: 영웅 피해는 루프가 끝난 뒤 적용
	// (영웅 보호막이 깨지면 눈보라가 탄을 쏘아 Projectiles 배열이 바뀔 수 있음)
	float HostileDamage = 0.f;
	float HostileSlowFactor = 1.f;
	float HostileSlowSeconds = 0.f;

	for (FMLProjectileEntry& Entry : Projectiles)
	{
		if (!Entry.bActive || !Entry.Actor) continue;

		FVector Loc = Entry.Actor->GetActorLocation();

		if (Entry.bHostile)
		{
			if (Entry.HomingTurnRate > 0.f && bHeroAlive)
			{
				const FVector CurrentDir = Entry.Velocity.GetSafeNormal2D();
				const FVector DesiredDir = (HeroLoc - Loc).GetSafeNormal2D();
				const float SignedAngle = FMath::Atan2(CurrentDir.X * DesiredDir.Y - CurrentDir.Y * DesiredDir.X, FVector::DotProduct(CurrentDir, DesiredDir));
				const float MaxTurn = FMath::DegreesToRadians(Entry.HomingTurnRate) * DeltaTime;
				Entry.Velocity = CurrentDir.RotateAngleAxisRad(FMath::Clamp(SignedAngle, -MaxTurn, MaxTurn), FVector::UpVector) * Entry.Velocity.Size2D();
				Entry.Actor->SetActorRotation(Entry.Velocity.Rotation());
			}

			const FVector Step = Entry.Velocity * DeltaTime;
			Loc += Step;
			Entry.RangeLeft -= Step.Size();
			Entry.Actor->SetActorLocation(Loc);

			if (bHeroAlive && FVector::Dist2D(Loc, HeroLoc) <= Entry.HitRadius + HeroHitRadius)
			{
				HostileDamage = FMath::Max(HostileDamage, Entry.Damage); // 한 프레임에 여러 발이어도 1번 (0.5초 제한은 적 서브시스템)
				if (Entry.SlowDuration > 0.f)
				{
					HostileSlowFactor = FMath::Min(HostileSlowFactor, Entry.SlowFactor);
					HostileSlowSeconds = FMath::Max(HostileSlowSeconds, Entry.SlowDuration);
				}
				Deactivate(Entry);
			}
			else if (Entry.RangeLeft <= 0.f)
			{
				Deactivate(Entry);
			}
			continue;
		}

		if (Entry.HomingTurnRate > 0.f && EnemySub)
		{
			// shortcut: 유도탄마다 매 프레임 가장 가까운 적 탐색 (O(탄 x 적)); stat unit에서 문제가 보이면 대상 인덱스 캐시
			const int32 SeekIndex = EnemySub->FindNearestEnemy(Loc, HomingSeekRange);
			if (SeekIndex != INDEX_NONE)
			{
				const FVector CurrentDir = Entry.Velocity.GetSafeNormal2D();
				const FVector DesiredDir = (EnemySub->GetEnemyLocation(SeekIndex) - Loc).GetSafeNormal2D();
				const float SignedAngle = FMath::Atan2(CurrentDir.X * DesiredDir.Y - CurrentDir.Y * DesiredDir.X, FVector::DotProduct(CurrentDir, DesiredDir));
				const float MaxTurn = FMath::DegreesToRadians(Entry.HomingTurnRate) * DeltaTime;

				Entry.Velocity = CurrentDir.RotateAngleAxisRad(FMath::Clamp(SignedAngle, -MaxTurn, MaxTurn), FVector::UpVector) * Entry.Velocity.Size2D();
				Entry.Actor->SetActorRotation(Entry.Velocity.Rotation());
			}
		}

		// shortcut: 매 프레임 점 판정; speed*dt > HitRadius면 관통 가능 (저프레임 / 매우 빠른 탄) — 그때 선분 판정으로 교체
		const FVector Step = Entry.Velocity * DeltaTime;
		Loc += Step;
		Entry.RangeLeft -= Step.Size();
		Entry.Actor->SetActorLocation(Loc);

		const int32 HitIndex = EnemySub ? EnemySub->FindNearestEnemy(Loc, Entry.HitRadius) : INDEX_NONE;
		if (HitIndex != INDEX_NONE)
		{
			if (Entry.SlowDuration > 0.f)
			{
				EnemySub->SlowEnemy(HitIndex, Entry.SlowFactor, Entry.SlowDuration); // 피해 전에: 죽은 적은 무시
			}
			EnemySub->DamageEnemy(HitIndex, Entry.Damage, Entry.Source);
			Deactivate(Entry);
		}
		else if (Entry.RangeLeft <= 0.f)
		{
			Deactivate(Entry);
		}
	}

	if (HostileDamage > 0.f && EnemySub && bHeroAlive)
	{
		EnemySub->DamageHeroBySkill(HostileDamage);
		if (HostileSlowSeconds > 0.f) Hero->ApplySlow(HostileSlowFactor, HostileSlowSeconds);
	}
}

void UMLProjectileSubsystem::Deactivate(FMLProjectileEntry& Entry)
{
	Entry.bActive = false;
	Entry.Actor->SetActorHiddenInGame(true);
	--ActiveCount;
}
