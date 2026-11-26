// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLEnemySubsystem.h"

#include "AbilitySystemComponent.h"
#include "MLPickupActor.h"
#include "Enemy/MLEnemyActor.h"
#include "GAS/Effects/MLGE_Damage.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/MLPickupSubsystem.h"

namespace
{
	// 적 중심이 아닌 가장자리(BodyRadius)까지의 거리, 제곱값.
	float EdgeDistSq(const FVector& From, const FMLEnemyEntry& Entry)
	{
		const float DistSq = FVector::DistSquared2D(From, Entry.Actor->GetActorLocation());
		return Entry.BodyRadius > 0.f ? FMath::Square(FMath::Max(0.f, FMath::Sqrt(DistSq) - Entry.BodyRadius)) : DistSq;
	}
}

bool UMLEnemySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UMLEnemySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMLEnemySubsystem, STATGROUP_Tickables);
}

bool UMLEnemySubsystem::SpawnEnemy(TSubclassOf<AMLEnemyActor> EnemyClass, const FVector& Location, float HealthMult, float SpeedMult, float GoldMult,
	EMLEliteTrait EliteTrait, int32* OutIndex)
{
	if (!EnemyClass) return false;

	// 같은 클래스의 비활성 액터를 먼저 재사용.
	FMLEnemyEntry* Slot = Enemies.FindByPredicate([&EnemyClass](const FMLEnemyEntry& E)
	{
		return !E.bActive && E.Actor && E.Actor->GetClass() == EnemyClass;
	});

	if (!Slot)
	{
		// 풀이 가득 차면 다른 클래스의 비활성 칸을 교체: 구역마다 적 종류가 달라도 이전 구역 칸이 스폰을 막지 않게
		if (Enemies.Num() >= MaxEnemies)
		{
			Slot = Enemies.FindByPredicate([](const FMLEnemyEntry& E) { return !E.bActive; });
			if (!Slot) return false;
		}

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMLEnemyActor* NewEnemy = GetWorld()->SpawnActor<AMLEnemyActor>(EnemyClass, Location, FRotator::ZeroRotator, SpawnInfo);
		if (!NewEnemy) return false; // 실패하면 기존 칸은 그대로 둠

		if (Slot)
		{
			if (Slot->Actor) Slot->Actor->Destroy();
		}
		else
		{
			Slot = &Enemies.AddDefaulted_GetRef();
		}
		Slot->Actor = NewEnemy;
	}

	const AMLEnemyActor* Defaults = EnemyClass->GetDefaultObject<AMLEnemyActor>();
	const bool bElite = EliteTrait != EMLEliteTrait::None;
	Slot->MaxHealth = Defaults->MaxHealth * HealthMult * (bElite ? EliteHealthMult : 1.f);
	Slot->Health = Slot->MaxHealth;
	Slot->MoveSpeed = Defaults->MoveSpeed * SpeedMult * (EliteTrait == EMLEliteTrait::Swift ? SwiftSpeedMult : 1.f);
	Slot->ContactDamage = Defaults->ContactDamage * (bElite ? EliteContactMult : 1.f);
	Slot->ShieldLeft = EliteTrait == EMLEliteTrait::Shield ? Slot->MaxHealth * ShieldHealthPercent : 0.f;
	Slot->BodyRadius = Defaults->BodyRadius * (bElite ? Defaults->EliteScale : 1.f);
	Slot->ForcedTimeLeft = 0.f;
	Slot->bRooted = false;
	Slot->Trait = EliteTrait;
	Slot->SlowFactor = 1.f;
	Slot->SlowTimeLeft = 0.f;
	Slot->GoldMult = GoldMult;
	Slot->bFlees = Defaults->bFleesFromHero;
	Slot->bActive = true;
	Slot->Actor->SetActorLocation(Location);
	Slot->Actor->SetEliteLook(bElite);
	Slot->Actor->SetActorHiddenInGame(false);
	++ActiveCount;

	if (OutIndex) *OutIndex = static_cast<int32>(Slot - Enemies.GetData());
	return true;
}

void UMLEnemySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HeroContactCooldown = FMath::Max(0.f, HeroContactCooldown - DeltaTime);
	HeroSkillCooldown = FMath::Max(0.f, HeroSkillCooldown - DeltaTime);
	if (ActiveCount == 0) return;

	AMLHeroCharacter* Hero = CachedHero.Get();
	if (!Hero)
	{
		Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		CachedHero = Hero;
	}
	if (!Hero || Hero->IsDead()) return;

	const FVector HeroLoc = Hero->GetActorLocation();
	const float SeparationRadiusSq = FMath::Square(SeparationRadius);
	float StrongestContact = 0.f;

	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		FMLEnemyEntry& Entry = Enemies[i];
		if (!Entry.bActive || !Entry.Actor) continue;

		if (Entry.Trait == EMLEliteTrait::Regen && Entry.Health < Entry.MaxHealth)
		{
			Entry.Health = FMath::Min(Entry.MaxHealth, Entry.Health + Entry.MaxHealth * RegenPercentPerSecond * DeltaTime);
		}

		FVector Loc = Entry.Actor->GetActorLocation();
		FVector ToHero = HeroLoc - Loc;
		ToHero.Z = 0.f;
		const float DistSq = ToHero.SizeSquared();

		if (Entry.ForcedTimeLeft > 0.f) // 보스 돌진
		{
			Entry.ForcedTimeLeft -= DeltaTime;
			Loc += Entry.ForcedVelocity * DeltaTime;
			Loc.Z = HeroLoc.Z;
			Entry.Actor->SetActorLocationAndRotation(Loc, Entry.ForcedVelocity.Rotation());
		}
		else if (!Entry.bRooted) // 고정됨 (보스 예고): 제자리에서 방향만 유지
		{
			// shortcut: O(n^2) 밀어내기, ~200마리까지는 괜찮음; stat unit에서 문제가 보이면 공간 격자로 교체
			FVector Push = FVector::ZeroVector;
			for (int32 j = 0; j < Enemies.Num(); ++j)
			{
				const FMLEnemyEntry& Other = Enemies[j];
				if (j == i || !Other.bActive || !Other.Actor) continue;

				FVector Away = Loc - Other.Actor->GetActorLocation();
				Away.Z = 0.f;
				const float AwaySq = Away.SizeSquared();
				if (AwaySq < SeparationRadiusSq && AwaySq > KINDA_SMALL_NUMBER)
				{
					const float AwayLen = FMath::Sqrt(AwaySq);
					Push += (Away / AwayLen) * (SeparationRadius - AwayLen);
				}
			}

			float SpeedMul = 1.f;
			if (Entry.SlowTimeLeft > 0.f)
			{
				Entry.SlowTimeLeft -= DeltaTime;
				SpeedMul = Entry.SlowFactor;
			}
			const FVector MoveDir = Entry.bFlees ? -ToHero.GetSafeNormal() : ToHero.GetSafeNormal();
			const FVector Velocity = MoveDir * Entry.MoveSpeed * SpeedMul + Push * SeparationStrength;
			Loc += Velocity * DeltaTime;
			Loc.Z = HeroLoc.Z; // shortcut: 평평한 맵 전용; 경기장에 높낮이가 생기면 지면 트레이스 추가

			Entry.Actor->SetActorLocationAndRotation(Loc, MoveDir.Rotation());
		}

		if (Entry.ContactDamage > 0.f && DistSq < FMath::Square(ContactRadius + Entry.BodyRadius))
		{
			StrongestContact = FMath::Max(StrongestContact, Entry.ContactDamage);
		}
	}

	// 영웅에 닿은 적 중 가장 강한 적이 주기마다 한 번만 타격: 무리가 접촉 피해를 중첩시킬 수 없음.
	if (StrongestContact > 0.f && HeroContactCooldown <= 0.f)
	{
		ApplyContactDamage(Hero, StrongestContact);
		HeroContactCooldown = ContactInterval;
	}
}

void UMLEnemySubsystem::ApplyContactDamage(AMLHeroCharacter* Hero, float Damage) const
{
	UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent();
	if (!ASC) return;

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UMLGE_Damage::StaticClass(), 1.f, ASC->MakeEffectContext());
	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetSetByCallerMagnitude(UMLGE_Damage::DamageName, Damage * HeroDamageMult);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

int32 UMLEnemySubsystem::FindNearestEnemy(const FVector& Origin, float MaxRange, const TArray<int32>* ExcludeIndices) const
{
	int32 BestIndex = INDEX_NONE;
	float BestDistSq = FMath::Square(MaxRange);

	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		const FMLEnemyEntry& Entry = Enemies[i];
		if (!Entry.bActive || !Entry.Actor) continue;
		if (ExcludeIndices && ExcludeIndices->Contains(i)) continue;

		const float DistSq = EdgeDistSq(Origin, Entry);
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}
	return BestIndex;
}

FVector UMLEnemySubsystem::GetEnemyLocation(int32 Index) const
{
	return Enemies.IsValidIndex(Index) && Enemies[Index].Actor
		? Enemies[Index].Actor->GetActorLocation()
		: FVector::ZeroVector;
}

int32 UMLEnemySubsystem::FindFleeingEnemy() const
{
	return Enemies.IndexOfByPredicate([](const FMLEnemyEntry& E) { return E.bActive && E.bFlees && E.Actor; });
}

int32 UMLEnemySubsystem::FindNearestElite(const FVector& Origin) const
{
	int32 BestIndex = INDEX_NONE;
	float BestDistSq = MAX_flt;
	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		const FMLEnemyEntry& Entry = Enemies[i];
		if (!Entry.bActive || !Entry.Actor || Entry.Trait == EMLEliteTrait::None) continue;

		const float DistSq = FVector::DistSquared2D(Origin, Entry.Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}
	return BestIndex;
}

void UMLEnemySubsystem::DamageEnemy(int32 Index, float Damage, FName Source)
{
	if (!Enemies.IsValidIndex(Index)) return;

	FMLEnemyEntry& Entry = Enemies[Index];
	if (!Entry.bActive) return;

	// design §6 피해 리포트: 적이 아직 받을 수 있는 만큼만 집계 (쉴드 엘리트의 쉴드 포함)
	RoomDamage.FindOrAdd(Source) += FMath::Min(Damage, Entry.ShieldLeft + FMath::Max(0.f, Entry.Health));

	if (Entry.ShieldLeft > 0.f) // 쉴드 엘리트 / 망령의 수호 방패: 체력보다 먼저 흡수
	{
		const float Absorbed = FMath::Min(Entry.ShieldLeft, Damage);
		Entry.ShieldLeft -= Absorbed;
		Damage -= Absorbed;
		if (Entry.ShieldLeft <= 0.f) OnEnemyShieldBroken.Broadcast(Index);
	}

	Entry.Health -= Damage;
	if (Entry.Health <= 0.f)
	{
		KillEnemy(Entry);
	}
}

int32 UMLEnemySubsystem::DamageInRadius(const FVector& Center, float Radius, float Damage, FName Source)
{
	const float RadiusSq = FMath::Square(Radius);
	int32 HitCount = 0;

	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		const FMLEnemyEntry& Entry = Enemies[i];
		if (Entry.bActive && Entry.Actor && EdgeDistSq(Center, Entry) <= RadiusSq)
		{
			DamageEnemy(i, Damage, Source);
			++HitCount;
		}
	}
	return HitCount;
}

void UMLEnemySubsystem::GetEnemiesInRange(const FVector& Origin, float Range, TArray<int32>& OutIndices) const
{
	OutIndices.Reset();
	const float RangeSq = FMath::Square(Range);
	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		const FMLEnemyEntry& Entry = Enemies[i];
		if (Entry.bActive && Entry.Actor && EdgeDistSq(Origin, Entry) <= RangeSq)
		{
			OutIndices.Add(i);
		}
	}
}

int32 UMLEnemySubsystem::SlowInRadius(const FVector& Center, float Radius, float SpeedMultiplier, float Duration)
{
	TArray<int32> Indices;
	GetEnemiesInRange(Center, Radius, Indices);
	for (const int32 Index : Indices)
	{
		SlowEnemy(Index, SpeedMultiplier, Duration);
	}
	return Indices.Num();
}

void UMLEnemySubsystem::DamageInArc(const FVector& Origin, const FVector& Forward, float Range, float HalfAngleDeg, float Damage, FName Source)
{
	const FVector Fwd2D = Forward.GetSafeNormal2D();
	const float RangeSq = FMath::Square(Range);
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));

	for (int32 i = 0; i < Enemies.Num(); ++i)
	{
		const FMLEnemyEntry& Entry = Enemies[i];
		if (!Entry.bActive || !Entry.Actor) continue;
		if (EdgeDistSq(Origin, Entry) > RangeSq) continue;

		FVector ToEnemy = Entry.Actor->GetActorLocation() - Origin;
		ToEnemy.Z = 0.f;
		const float DistSq = ToEnemy.SizeSquared();

		// 영웅 위에 있는 적(또는 큰 몸체와 겹친 적)은 항상 호 안으로 판정.
		if (DistSq < FMath::Square(Entry.BodyRadius) + KINDA_SMALL_NUMBER || FVector::DotProduct(ToEnemy / FMath::Sqrt(DistSq), Fwd2D) >= CosHalf)
		{
			DamageEnemy(i, Damage, Source);
		}
	}
}

void UMLEnemySubsystem::RegisterDamageSource(FName Source, const FText& Name)
{
	DamageSourceNames.Add(Source, Name);
}

FText UMLEnemySubsystem::GetDamageSourceName(FName Source) const
{
	const FText* Name = DamageSourceNames.Find(Source);
	return Name ? *Name : NSLOCTEXT("ML", "DamageSourceOther", "기타");
}

TMap<FName, float> UMLEnemySubsystem::TakeRoomDamage()
{
	TMap<FName, float> Taken = MoveTemp(RoomDamage);
	RoomDamage.Reset();
	return Taken;
}

void UMLEnemySubsystem::KillEnemy(FMLEnemyEntry& Entry)
{
	const AMLEnemyActor* Enemy = Entry.Actor;
	const FVector Loc = Enemy->GetActorLocation();
	const bool bElite = Entry.Trait != EMLEliteTrait::None;
	const float XP = Enemy->XPValue * (bElite ? EliteXPMult : 1.f);
	// design §6.5: 엘리트는 항상 금화 묶음 드롭; 일반 적은 GoldChance 판정
	const int32 CoinCount = bElite ? EliteCoinCount : (FMath::FRand() < Enemy->GoldChance ? Enemy->GoldDropCount : 0);
	const int32 CoinValue = FMath::Max(1, FMath::RoundToInt(Enemy->GoldValue * Entry.GoldMult));

	if (UMLPickupSubsystem* PickupSub = GetWorld()->GetSubsystem<UMLPickupSubsystem>())
	{
		PickupSub->SpawnGem(Enemy->XPGemClass, Loc, XP);

		if (Enemy->GoldCoinClass)
		{
			for (int32 i = 0; i < CoinCount; ++i)
			{
				const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
				const float Dist = FMath::FRandRange(GoldDropMinOffset, GoldScatterRadius);
				PickupSub->SpawnGem(Enemy->GoldCoinClass, Loc + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Dist, CoinValue);
			}
		}
	}

	Entry.bActive = false;
	Entry.Actor->SetActorHiddenInGame(true);
	--ActiveCount;
	OnEnemyKilled.Broadcast(Enemy, bElite); // --ActiveCount 다음에: 리스너가 새 숫자를 보도록
}

void UMLEnemySubsystem::SlowEnemy(int32 Index, float SpeedMultiplier, float Duration)
{
	if (!Enemies.IsValidIndex(Index) || !Enemies[Index].bActive) return;

	FMLEnemyEntry& Entry = Enemies[Index];
	Entry.SlowFactor = Entry.SlowTimeLeft > 0.f ? FMath::Min(Entry.SlowFactor, SpeedMultiplier) : SpeedMultiplier;
	Entry.SlowTimeLeft = FMath::Max(Entry.SlowTimeLeft, Duration);
}

int32 UMLEnemySubsystem::ClearAll()
{
	int32 Cleared = 0;
	for (FMLEnemyEntry& Entry : Enemies)
	{
		if (!Entry.bActive || !Entry.Actor) continue;
		Entry.bActive = false;
		Entry.Actor->SetActorHiddenInGame(true);
		++Cleared;
	}
	ActiveCount = 0;
	return Cleared;
}

void UMLEnemySubsystem::RemoveEnemy(int32 Index)
{
	if (!IsEnemyActive(Index)) return;

	Enemies[Index].bActive = false;
	if (Enemies[Index].Actor) Enemies[Index].Actor->SetActorHiddenInGame(true);
	--ActiveCount;
}

float UMLEnemySubsystem::GetEnemyHealthPercent(int32 Index) const
{
	if (!IsEnemyActive(Index) || Enemies[Index].MaxHealth <= 0.f) return 0.f;
	return FMath::Clamp(Enemies[Index].Health / Enemies[Index].MaxHealth, 0.f, 1.f);
}

void UMLEnemySubsystem::SetEnemyRooted(int32 Index, bool bRooted)
{
	if (IsEnemyActive(Index)) Enemies[Index].bRooted = bRooted;
}

void UMLEnemySubsystem::LaunchEnemy(int32 Index, const FVector& Velocity, float Duration)
{
	if (!IsEnemyActive(Index)) return;

	FMLEnemyEntry& Entry = Enemies[Index];
	Entry.bRooted = false;
	Entry.ForcedVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
	Entry.ForcedTimeLeft = Duration;
}

void UMLEnemySubsystem::SetEnemyMoveSpeed(int32 Index, float Speed)
{
	if (IsEnemyActive(Index)) Enemies[Index].MoveSpeed = Speed;
}

void UMLEnemySubsystem::DamageHeroBySkill(float Damage)
{
	if (HeroSkillCooldown > 0.f) return;

	AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Hero || Hero->IsDead()) return;

	ApplyContactDamage(Hero, Damage * HostileSkillDamageMult);
	HeroSkillCooldown = HostileSkillInterval;

	// MLVerify(M9-3c): 다음 마일스톤에서 제거
	UE_LOG(LogTemp, Display, TEXT("[MLVerify] Ghost skill hit: raw=%.1f final=%.1f (x%.2f x zone %.2f, before DEF)"),
		Damage, Damage * HostileSkillDamageMult * HeroDamageMult, HostileSkillDamageMult, HeroDamageMult);
}

void UMLEnemySubsystem::SetEnemyMaxHealth(int32 Index, float MaxHealth)
{
	if (!IsEnemyActive(Index)) return;
	Enemies[Index].MaxHealth = FMath::Max(1.f, MaxHealth);
	Enemies[Index].Health = Enemies[Index].MaxHealth;
}

void UMLEnemySubsystem::SetEnemyShield(int32 Index, float Shield)
{
	if (IsEnemyActive(Index)) Enemies[Index].ShieldLeft = FMath::Max(0.f, Shield);
}

void UMLEnemySubsystem::DamageHero(float Damage)
{
	AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (Hero && !Hero->IsDead()) ApplyContactDamage(Hero, Damage);
}