// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLAutoSkills.h"

#include "DrawDebugHelpers.h"
#include "Enemy/MLEnemySubsystem.h"
#include "Enemy/MLEnemyActor.h"
#include "Engine/World.h"
#include "Skills/MLSkillVisualActor.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "Combat/MLProjectileSubsystem.h"
#include "GAS/MLAttributeSet.h"

// ======================= 회전 칼날 =======================

UMLSkill_Blades::UMLSkill_Blades()
{
	CooldownByLevel = { 5.f, 5.f, 5.f };
}

void UMLSkill_Blades::OnAcquired()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	if (!World || !Hero || !PivotClass) return;

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Pivot = World->SpawnActor<AMLSkillVisualActor>(PivotClass, Hero->GetActorLocation(), FRotator::ZeroRotator, SpawnInfo);
	if (!Pivot) return;

	Pivot->AttachToActor(Hero, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Pivot->GetRootComponent()->SetUsingAbsoluteRotation(true); // 영웅이 회전해도 궤도가 흔들리지 않게

	// 쿨타임이 없으면 상시형(진화 칼날 폭풍). 칼날은 이어지는 OnLevelChanged에서 생성됨
	if (GetEffectiveCooldown() <= 0.f)
	{
		SetBladesActive(true);
	}

	if (PulseInterval > 0.f)
	{
		World->GetTimerManager().SetTimer(PulseTimerHandle, this, &UMLSkill_Blades::StartPulse, PulseInterval, true);
	}
	if (ShotInterval > 0.f)
	{
		World->GetTimerManager().SetTimer(ShotTimerHandle, this, &UMLSkill_Blades::FireShots, ShotInterval, true);
	}
}

void UMLSkill_Blades::StartPulse()
{
	UWorld* World = GetWorld();
	const UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!World || !bBladesActive || IsHeroDead() || !EnemySub || EnemySub->GetActiveCount() == 0) return;

	PulseElapsed = 0.f;
	World->GetTimerManager().SetTimer(PulseStepHandle, this, &UMLSkill_Blades::StepPulse, PulseStepSeconds, true);
}

void UMLSkill_Blades::FireShots()
{
	UWorld* World = GetWorld();
	const UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	UMLProjectileSubsystem* ProjectileSub = World ? World->GetSubsystem<UMLProjectileSubsystem>() : nullptr;
	if (!Pivot || !ProjectileSub || !bBladesActive || IsHeroDead() || !EnemySub || EnemySub->GetActiveCount() == 0) return;

	FMLProjectileLaunch Shot;
	Shot.ProjectileClass = ShotClass;
	Shot.Damage = RollDamage(ShotDamageMult);
	Shot.HitRadius = ShotHitRadius;
	Shot.MaxRange = ShotRange;
	Shot.Source = GetDamageSource();

	int32 Launched = 0;
	const FVector Center = Pivot->GetActorLocation();
	for (const TObjectPtr<AMLSkillVisualActor>& Blade : Blades)
	{
		if (!Blade) continue;
		Shot.Start = Blade->GetActorLocation();
		Shot.Velocity = (Shot.Start - Center).GetSafeNormal2D() * ShotSpeed; // 칼날 위치에서 바깥 방향
		if (ProjectileSub->Launch(Shot)) ++Launched;
	}

	// MLVerify(M9-1c): M9-2에서 제거 — 6번마다 요약
	if (++VerifyShotFires % 6 == 1)
	{
		UE_LOG(LogTemp, Display, TEXT("[MLVerify] Blade shots: %s launched=%d interval=%.2f"), *GetSkillName().ToString(), Launched, ShotInterval);
	}
}

void UMLSkill_Blades::StepPulse()
{
	PulseElapsed += PulseStepSeconds;
	const float Alpha = FMath::Clamp(PulseElapsed / FMath::Max(0.1f, PulseSeconds), 0.f, 1.f);
	// 0 → 1 → 0으로 넓어졌다 돌아오는 곡선
	SetOrbitRadius(FMath::Lerp(OrbitRadius, PulseRadius, FMath::Sin(PI * Alpha)));

	if (Alpha >= 1.f)
	{
		SetOrbitRadius(OrbitRadius);
		if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(PulseStepHandle);
	}
}

void UMLSkill_Blades::SetOrbitRadius(float Radius)
{
	const int32 BladeCount = Blades.Num();
	for (int32 i = 0; i < BladeCount; ++i)
	{
		if (!Blades[i]) continue;
		const FRotator Angle(0.f, 360.f * i / BladeCount, 0.f);
		Blades[i]->SetActorRelativeLocation(Angle.RotateVector(FVector(Radius, 0.f, 0.f)));
	}
}

void UMLSkill_Blades::Fire()
{
	UWorld* World = GetWorld();
	const float Duration = LevelValue(DurationByLevel);
	if (!World || Duration <= 0.f) return;

	SetBladesActive(true);
	// 이미 돌고 있으면(쿨타임 감소로 주기 < 지속시간) 종료 시점만 뒤로 밀림
	World->GetTimerManager().SetTimer(ActiveTimerHandle, this, &UMLSkill_Blades::HandleBladesEnd, Duration, false);
}

void UMLSkill_Blades::HandleBladesEnd()
{
	SetBladesActive(false);
}

void UMLSkill_Blades::SetBladesActive(bool bActive)
{
	UWorld* World = GetWorld();
	if (!World || bActive == bBladesActive) return;

	bBladesActive = bActive;
	for (const TObjectPtr<AMLSkillVisualActor>& Blade : Blades)
	{
		if (Blade) Blade->SetActorHiddenInGame(!bActive);
	}

	if (bActive)
	{
		World->GetTimerManager().SetTimer(DamageTimerHandle, this, &UMLSkill_Blades::ApplyBladeDamage, HitInterval, true);
	}
	else
	{
		World->GetTimerManager().ClearTimer(DamageTimerHandle);
	}
}

void UMLSkill_Blades::OnLevelChanged()
{
	UWorld* World = GetWorld();
	if (!World || !Pivot || !BladeClass) return;

	for (const TObjectPtr<AMLSkillVisualActor>& Blade : Blades)
	{
		if (Blade) Blade->Destroy();
	}
	Blades.Reset();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const int32 BladeCount = FMath::Max(1, LevelValue(BladeCountByLevel));
	for (int32 i = 0; i < BladeCount; ++i)
	{
		const FRotator Angle(0.f, 360.f * i / BladeCount, 0.f);
		AMLSkillVisualActor* Blade = World->SpawnActor<AMLSkillVisualActor>(BladeClass, Pivot->GetActorLocation(), FRotator::ZeroRotator, SpawnInfo);
		if (!Blade) continue;

		Blade->AttachToActor(Pivot, FAttachmentTransformRules::KeepWorldTransform);
		Blade->SetActorRelativeLocation(Angle.RotateVector(FVector(OrbitRadius, 0.f, 0.f)));
		Blade->SetActorRelativeRotation(Angle);
		Blade->SetActorHiddenInGame(!bBladesActive); // 돌고 있는 중 레벨업하면 새 칼날도 바로 보임
		Blades.Add(Blade);
	}
}

void UMLSkill_Blades::ApplyBladeDamage()
{
	if (IsHeroDead()) return;

	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!EnemySub) return;

	const float Damage = RollDamage(LevelValue(DamageMultByLevel));
	for (const TObjectPtr<AMLSkillVisualActor>& Blade : Blades)
	{
		if (Blade)
		{
			EnemySub->DamageInRadius(Blade->GetActorLocation(), HitRadius, Damage, GetDamageSource());
		}
	}
}

void UMLSkill_Blades::Release()
{
	for (const TObjectPtr<AMLSkillVisualActor>& Blade : Blades)
	{
		if (Blade) Blade->Destroy();
	}
	Blades.Reset();
	if (Pivot)
	{
		Pivot->Destroy();
		Pivot = nullptr;
	}
	Super::Release();
}

// ======================= 번개 사슬 =======================

UMLSkill_ChainLightning::UMLSkill_ChainLightning()
{
	CooldownByLevel = { 2.5f, 2.5f, 2.5f };
}

void UMLSkill_ChainLightning::Fire()
{
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	AActor* Hero = GetHero();
	if (!EnemySub || !Hero) return;

	TArray<int32> HitIndices;
	ChainFrom(Hero->GetActorLocation(), FirstRange, LevelValue(TargetCountByLevel), RollDamage(LevelValue(DamageMultByLevel)), HitIndices);
}

void UMLSkill_ChainLightning::ChainFrom(FVector From, float FirstSearchRange, int32 MaxTargets, float Damage, TArray<int32>& HitIndices)
{
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!EnemySub) return;

	int32 Target = EnemySub->FindNearestEnemy(From, FirstSearchRange, &HitIndices);
	int32 Hits = 0;
	while (Target != INDEX_NONE && Hits < MaxTargets)
	{
		const FVector To = EnemySub->GetEnemyLocation(Target);
#if ENABLE_DRAW_DEBUG
		// shortcut: 디버그 선을 빔 시각 효과로 사용, M10에서 Niagara 빔으로 교체
		DrawDebugLine(GetWorld(), From, To, FColor(120, 180, 255), false, 0.2f, 0, 4.f);
#endif
		HitIndices.Add(Target);
		++Hits;
		EnemySub->DamageEnemy(Target, Damage, GetDamageSource());

		From = To;
		Target = EnemySub->FindNearestEnemy(From, ChainRange, &HitIndices);
	}
}

// ======================= 천둥 심판 (2단계 낙뢰) =======================

UMLSkill_ThunderStrike::UMLSkill_ThunderStrike()
{
	CooldownByLevel = { 1.5f, 1.5f, 1.5f };
}

void UMLSkill_ThunderStrike::Fire()
{
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	AActor* Hero = GetHero();
	if (!EnemySub || !Hero) return;

	TArray<int32> Candidates;
	EnemySub->GetEnemiesInRange(Hero->GetActorLocation(), StrikeRange, Candidates);
	if (Candidates.Num() == 0) return;

	const int32 StrikeCount = FMath::Min(LevelValue(StrikeCountByLevel), Candidates.Num());
	const float StrikeDamage = RollDamage(LevelValue(StrikeDamageMultByLevel));
	const float ChainDamage = RollDamage(LevelValue(ChainDamageMultByLevel));

	for (int32 s = 0; s < StrikeCount && Candidates.Num() > 0; ++s)
	{
		const int32 Pick = FMath::RandRange(0, Candidates.Num() - 1);
		StrikeAt(Candidates[Pick], StrikeDamage, ChainDamage);
		Candidates.RemoveAtSwap(Pick);
	}
}

int32 UMLSkill_ThunderStrike::StrikeAt(int32 TargetIndex, float StrikeDamage, float ChainDamage)
{
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!EnemySub || !EnemySub->IsEnemyActive(TargetIndex)) return 0; // 앞선 낙뢰에 이미 죽음

	const FVector Ground = EnemySub->GetEnemyLocation(TargetIndex);
#if ENABLE_DRAW_DEBUG
	// shortcut: 디버그 선·원을 낙뢰 시각 효과로 사용, M10에서 Niagara로 교체
	DrawDebugLine(GetWorld(), Ground + FVector(0.f, 0.f, 700.f), Ground, FColor(255, 240, 120), false, 0.25f, 0, 10.f);
	DrawDebugCircle(GetWorld(), Ground, StrikeRadius, 24, FColor(255, 240, 120), false, 0.25f, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
#endif
	const int32 Hits = EnemySub->DamageInRadius(Ground, StrikeRadius, StrikeDamage, GetDamageSource());

	// 맞은 지점에서 근처 적에게 연쇄 (낙뢰 중심의 적은 건너뜀)
	TArray<int32> Chained;
	Chained.Add(TargetIndex);
	ChainFrom(Ground, ChainRange, ChainPerStrike, ChainDamage, Chained);
	return Hits;
}

void UMLSkill_ThunderStrike::OnAcquired()
{
	Super::OnAcquired();
	if (StormInterval > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(StormTimerHandle, this, &UMLSkill_ThunderStrike::StartStorm, StormInterval, true);
		}
	}
}

void UMLSkill_ThunderStrike::StartStorm()
{
	UWorld* World = GetWorld();
	const UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!World || IsHeroDead() || !EnemySub || EnemySub->GetActiveCount() == 0 || StormStrikes <= 0) return;

	StormLeft = StormStrikes;
	World->GetTimerManager().SetTimer(StormStrikeHandle, this, &UMLSkill_ThunderStrike::StormStrike, FMath::Max(0.05f, StormSeconds / StormStrikes), true);

	// MLVerify(M9-1c): M9-2에서 제거
	UE_LOG(LogTemp, Display, TEXT("[MLVerify] Thunder storm: strikes=%d over %.1fs range=%.0f"), StormStrikes, StormSeconds, StormRange);
}

void UMLSkill_ThunderStrike::StormStrike()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!World || !Hero || !EnemySub || IsHeroDead() || --StormLeft < 0)
	{
		if (World) World->GetTimerManager().ClearTimer(StormStrikeHandle);
		return;
	}

	TArray<int32> Candidates;
	EnemySub->GetEnemiesInRange(Hero->GetActorLocation(), StormRange, Candidates);
	if (Candidates.Num() == 0) return;

	// 뇌신강림 낙뢰는 항상 치명타
	StrikeAt(Candidates[FMath::RandRange(0, Candidates.Num() - 1)], RollDamage(StormDamageMult, true), RollDamage(LevelValue(ChainDamageMultByLevel), true));
}

// ======================= 화염 장판 =======================

UMLSkill_FireField::UMLSkill_FireField()
{
	CooldownByLevel = { 4.f, 4.f, 4.f };
}

void UMLSkill_FireField::OnAcquired()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FieldTimerHandle, this, &UMLSkill_FireField::TickFields, TickInterval, true);
	}
}

void UMLSkill_FireField::Fire()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	if (!World || !Hero) return;

	FMLFireFieldEntry* Field = Fields.FindByPredicate([](const FMLFireFieldEntry& F) { return !F.bActive; });
	if (!Field)
	{
		Field = &Fields.AddDefaulted_GetRef();
		if (FieldClass)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Field->Actor = World->SpawnActor<AMLSkillVisualActor>(FieldClass, Hero->GetActorLocation(), FRotator::ZeroRotator, SpawnInfo);
		}
	}

	const FVector2D Offset = FMath::RandPointInCircle(SpawnRange);
	Field->Center = Hero->GetActorLocation() + FVector(Offset.X, Offset.Y, 0.f);
	Field->Radius = LevelValue(RadiusByLevel);
	Field->TimeLeft = LevelValue(DurationByLevel);
	Field->TotalHits = 0;
	Field->bActive = true;

	if (Field->Actor)
	{
		const float VisualScale = Field->Radius / FMath::Max(1.f, VisualBaseRadius);
		Field->Actor->SetActorLocation(Field->Center);
		Field->Actor->SetActorScale3D(FVector(VisualScale, VisualScale, 1.f));
		Field->Actor->SetActorHiddenInGame(false);
	}
}

void UMLSkill_FireField::TickFields()
{
	if (IsHeroDead()) return;

	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	const float Damage = RollDamage(TickDamageMult);

	for (FMLFireFieldEntry& Field : Fields)
	{
		if (!Field.bActive) continue;

		if (EnemySub)
		{
			Field.TotalHits += EnemySub->DamageInRadius(Field.Center, Field.Radius, Damage, GetDamageSource());
		}

		Field.TimeLeft -= TickInterval;
		if (Field.TimeLeft <= 0.f)
		{
			Field.bActive = false;
			if (Field.Actor) Field.Actor->SetActorHiddenInGame(true);
		}
	}
}

void UMLSkill_FireField::Release()
{
	for (const FMLFireFieldEntry& Field : Fields)
	{
		if (Field.Actor) Field.Actor->Destroy();
	}
	Fields.Reset();
	Super::Release();
}

// ======================= 수호 방패 =======================

UMLSkill_GuardShield::UMLSkill_GuardShield()
{
	CooldownByLevel = { 10.f, 8.f, 6.f };
}

void UMLSkill_GuardShield::OnAcquired()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	UAbilitySystemComponent* ASC = GetHeroASC();
	if (!World || !Hero || !ASC) return;

	ASC->GetGameplayAttributeValueChangeDelegate(UMLAttributeSet::GetShieldAttribute()).AddUObject(this, &UMLSkill_GuardShield::HandleShieldChanged);

	if (ShieldVisualClass)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ShieldVisual = World->SpawnActor<AMLSkillVisualActor>(ShieldVisualClass, Hero->GetActorLocation(), FRotator::ZeroRotator, SpawnInfo);
		if (ShieldVisual)
		{
			ShieldVisual->AttachToActor(Hero, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			ShieldVisual->SetActorHiddenInGame(true);
		}
	}
}

void UMLSkill_GuardShield::Fire()
{
	UAbilitySystemComponent* ASC = GetHeroASC();
	if (!ASC) return;

	const float MaxHP = ASC->GetNumericAttribute(UMLAttributeSet::GetMaxHealthAttribute());
	const float ShieldAmount = MaxHP * LevelValue(ShieldPercentByLevel);
	ASC->SetNumericAttributeBase(UMLAttributeSet::GetShieldAttribute(), ShieldAmount);
}

void UMLSkill_GuardShield::HandleShieldChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ShieldVisual)
	{
		ShieldVisual->SetActorHiddenInGame(ChangeData.NewValue <= 0.f);
	}
	if (ChangeData.NewValue <= 0.f && ChangeData.OldValue > 0.f)
	{
		OnShieldBroken();
	}
}

void UMLSkill_GuardShield::Release()
{
	if (UAbilitySystemComponent* ASC = GetHeroASC())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UMLAttributeSet::GetShieldAttribute()).RemoveAll(this);
		// Shield는 이 기술이 소유한 원시 어트리뷰트; 남겨 두면 계속 피해를 흡수하고 영웅 바도 계속 보임.
		ASC->SetNumericAttributeBase(UMLAttributeSet::GetShieldAttribute(), 0.f);
	}
	if (ShieldVisual)
	{
		ShieldVisual->Destroy();
		ShieldVisual = nullptr;
	}
	Super::Release();
}

// ======================= 눈보라 (2단계 서리 폭풍) =======================

UMLSkill_FrostNova::UMLSkill_FrostNova()
{
	CooldownByLevel = { 3.f, 3.f, 3.f };
}

void UMLSkill_FrostNova::Fire()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	UAbilitySystemComponent* ASC = GetHeroASC();
	if (!World || !Hero || !EnemySub) return;

	// 쉴드는 깨졌을 때(0)만 새로 줌: 깨짐 → 파편 폭발 → 다음 폭발에 다시 쉴드
	if (ASC && ASC->GetNumericAttribute(UMLAttributeSet::GetShieldAttribute()) <= 0.f)
	{
		Super::Fire();
	}

	const FVector Center = Hero->GetActorLocation();
	NovaTargetRadius = LevelValue(NovaRadiusByLevel);
	EnemySub->DamageInRadius(Center, NovaTargetRadius, RollDamage(LevelValue(NovaDamageMultByLevel)), GetDamageSource());
	EnemySub->SlowInRadius(Center, NovaTargetRadius, 0.f, FreezeSeconds);

	if (NovaVisualClass)
	{
		if (!NovaVisual)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			NovaVisual = World->SpawnActor<AMLSkillVisualActor>(NovaVisualClass, Center, FRotator::ZeroRotator, SpawnInfo);
		}
		if (NovaVisual)
		{
			NovaVisual->SetActorLocation(Center);
			NovaVisual->SetActorHiddenInGame(false);
			NovaElapsed = 0.f;
			StepNovaVisual();
			World->GetTimerManager().SetTimer(NovaGrowHandle, this, &UMLSkill_FrostNova::StepNovaVisual, 0.03f, true);
		}
	}

}

void UMLSkill_FrostNova::OnAcquired()
{
	Super::OnAcquired();
	if (GlacierInterval > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(GlacierTimerHandle, this, &UMLSkill_FrostNova::CastGlacier, GlacierInterval, true);
		}
	}
}

void UMLSkill_FrostNova::CastGlacier()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!World || !Hero || !EnemySub || IsHeroDead() || EnemySub->GetActiveCount() == 0) return;

	// 결빙 먼저: 피해로 죽지 않은 적이 확실히 멈추도록
	const FVector Center = Hero->GetActorLocation();
	const int32 Frozen = EnemySub->SlowInRadius(Center, GlacierRadius, 0.f, GlacierFreezeSeconds);
	const int32 Hits = EnemySub->DamageInRadius(Center, GlacierRadius, RollDamage(GlacierDamageMult), GetDamageSource());

	if (GlacierVisualClass)
	{
		if (!GlacierVisual)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GlacierVisual = World->SpawnActor<AMLSkillVisualActor>(GlacierVisualClass, Center, FRotator::ZeroRotator, SpawnInfo);
		}
		if (GlacierVisual)
		{
			const float Scale = GlacierRadius / FMath::Max(1.f, NovaVisualBaseRadius);
			GlacierVisual->SetActorLocation(Center);
			GlacierVisual->SetActorScale3D(FVector(Scale, Scale, 1.f));
			GlacierVisual->SetActorHiddenInGame(false);
			World->GetTimerManager().SetTimer(GlacierHideHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (GlacierVisual) GlacierVisual->SetActorHiddenInGame(true);
			}), 0.5f, false);
		}
	}

	// MLVerify(M9-1c): M9-2에서 제거
	UE_LOG(LogTemp, Display, TEXT("[MLVerify] Glacier: radius=%.0f frozen=%d hits=%d"), GlacierRadius, Frozen, Hits);
}

void UMLSkill_FrostNova::StepNovaVisual()
{
	if (!NovaVisual) return;

	// 작은 원판에서 반경까지 퍼진 뒤 사라짐 (표시 전용, 피해는 Fire에서 한 번에)
	const float Alpha = FMath::Clamp(NovaElapsed / FMath::Max(0.05f, NovaGrowSeconds), 0.f, 1.f);
	const float Scale = FMath::Lerp(0.2f, 1.f, Alpha) * NovaTargetRadius / FMath::Max(1.f, NovaVisualBaseRadius);
	NovaVisual->SetActorScale3D(FVector(Scale, Scale, 1.f));
	NovaElapsed += 0.03f;

	if (Alpha >= 1.f)
	{
		NovaVisual->SetActorHiddenInGame(true);
		if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(NovaGrowHandle);
	}
}

void UMLSkill_FrostNova::OnShieldBroken()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	UMLProjectileSubsystem* ProjectileSub = World ? World->GetSubsystem<UMLProjectileSubsystem>() : nullptr;
	if (!Hero || !ProjectileSub || IsHeroDead()) return;

	const int32 Count = FMath::Max(1, BreakShardCount);
	const float AngleOffset = FMath::FRandRange(0.f, 360.f / Count);

	FMLProjectileLaunch Shot;
	Shot.ProjectileClass = ShardClass;
	Shot.Start = Hero->GetActorLocation();
	Shot.Damage = RollDamage(ShardDamageMult);
	Shot.HitRadius = 40.f;
	Shot.MaxRange = ShardRange;
	Shot.SlowFactor = 1.f - ShardSlow;
	Shot.SlowDuration = 2.f;
	Shot.Source = GetDamageSource();

	for (int32 i = 0; i < Count; ++i)
	{
		Shot.Velocity = FRotator(0.f, AngleOffset + 360.f * i / Count, 0.f).Vector() * ShardSpeed;
		ProjectileSub->Launch(Shot);
	}
}

void UMLSkill_FrostNova::Release()
{
	if (NovaVisual)
	{
		NovaVisual->Destroy();
		NovaVisual = nullptr;
	}
	if (GlacierVisual)
	{
		GlacierVisual->Destroy();
		GlacierVisual = nullptr;
	}
	Super::Release();
}

// ======================= 얼음 파편 =======================

UMLSkill_IceShards::UMLSkill_IceShards()
{
	CooldownByLevel = { 3.f, 3.f, 3.f };
}

void UMLSkill_IceShards::Fire()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	UMLProjectileSubsystem* ProjectileSub = World ? World->GetSubsystem<UMLProjectileSubsystem>() : nullptr;
	if (!Hero || !ProjectileSub) return;

	const FMLShardPattern Pattern = LevelValue(PatternByLevel);
	const int32 ShardCount = FMath::Max(1, Pattern.Count);
	const bool bRing = Pattern.ArcDeg >= 360.f;

	// 기준 방향: 가장 가까운 적, 없으면 무작위
	float BaseYaw = FMath::FRandRange(0.f, 360.f);
	if (const UMLEnemySubsystem* EnemySub = GetEnemySubsystem())
	{
		const int32 Nearest = EnemySub->FindNearestEnemy(Hero->GetActorLocation(), ShardRange);
		if (Nearest != INDEX_NONE)
		{
			BaseYaw = (EnemySub->GetEnemyLocation(Nearest) - Hero->GetActorLocation()).Rotation().Yaw;
		}
	}
	const float Step = bRing ? 360.f / ShardCount : (ShardCount > 1 ? Pattern.ArcDeg / (ShardCount - 1) : 0.f);
	const float StartYaw = bRing ? BaseYaw : BaseYaw - Pattern.ArcDeg * 0.5f;
	const int32 Sides = Pattern.bMirror && !bRing ? 2 : 1;

	FMLProjectileLaunch Shot;
	Shot.ProjectileClass = ShardClass;
	Shot.Start = Hero->GetActorLocation();
	Shot.Damage = RollDamage(DamageMult);
	Shot.HitRadius = ShardHitRadius;
	Shot.MaxRange = ShardRange;
	Shot.SlowFactor = 1.f - LevelValue(SlowByLevel);
	Shot.SlowDuration = SlowDuration;
	Shot.Source = GetDamageSource();

	for (int32 Side = 0; Side < Sides; ++Side)
	{
		for (int32 i = 0; i < ShardCount; ++i)
		{
			Shot.Velocity = FRotator(0.f, StartYaw + Step * i + 180.f * Side, 0.f).Vector() * ShardSpeed;
			ProjectileSub->Launch(Shot);
		}
	}
}

// ======================= 유도 마탄 =======================

UMLSkill_HomingBolt::UMLSkill_HomingBolt()
{
	CooldownByLevel = { 2.f, 2.f, 2.f };
}

void UMLSkill_HomingBolt::Fire()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	UMLProjectileSubsystem* ProjectileSub = World ? World->GetSubsystem<UMLProjectileSubsystem>() : nullptr;
	if (!Hero || !EnemySub || !ProjectileSub) return;

	const FVector Origin = Hero->GetActorLocation();
	const int32 TargetIndex = EnemySub->FindNearestEnemy(Origin, BoltRange);
	if (TargetIndex == INDEX_NONE) return; // 쫓을 대상 없음: 이번 시전 건너뜀

	const FVector AimDir = (EnemySub->GetEnemyLocation(TargetIndex) - Origin).GetSafeNormal2D();
	const int32 BoltCount = FMath::Max(1, LevelValue(BoltCountByLevel));

	FMLProjectileLaunch Shot;
	Shot.ProjectileClass = BoltClass;
	Shot.Start = Origin;
	Shot.Damage = RollDamage(LevelValue(DamageMultByLevel));
	Shot.HitRadius = BoltHitRadius;
	Shot.MaxRange = BoltRange;
	Shot.HomingTurnRate = TurnRateDeg;
	Shot.Source = GetDamageSource();

	int32 Launched = 0;
	for (int32 i = 0; i < BoltCount; ++i)
	{
		const float Spread = (i - (BoltCount - 1) * 0.5f) * SpreadAngle;
		Shot.Velocity = AimDir.RotateAngleAxis(Spread, FVector::UpVector) * BoltSpeed;
		if (ProjectileSub->Launch(Shot)) ++Launched;
	}
}

// ======================= 오라 / 지옥불 =======================

void UMLSkill_Aura::OnAcquired()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	if (!World || !Hero) return;

	if (AuraVisualClass)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AuraVisual = World->SpawnActor<AMLSkillVisualActor>(AuraVisualClass, Hero->GetActorLocation(), FRotator::ZeroRotator, SpawnInfo);
		if (AuraVisual)
		{
			AuraVisual->AttachToActor(Hero, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}

	World->GetTimerManager().SetTimer(AuraTimerHandle, this, &UMLSkill_Aura::TickAura, TickInterval, true);
	if (FlameInterval > 0.f)
	{
		World->GetTimerManager().SetTimer(FlameTimerHandle, this, &UMLSkill_Aura::DropFlame, FlameInterval, true);
	}
	if (bDeathBurst)
	{
		if (UMLEnemySubsystem* EnemySub = GetEnemySubsystem())
		{
			EnemySub->OnEnemyKilled.AddUObject(this, &UMLSkill_Aura::HandleEnemyKilled);
		}
	}
}

void UMLSkill_Aura::OnLevelChanged()
{
	if (AuraVisual)
	{
		const float Scale = LevelValue(RadiusByLevel) / FMath::Max(1.f, VisualBaseRadius);
		AuraVisual->SetActorRelativeScale3D(FVector(Scale, Scale, 1.f));
	}
}

void UMLSkill_Aura::TickAura()
{
	AActor* Hero = GetHero();
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!Hero || !EnemySub || IsHeroDead()) return;

	EnemySub->DamageInRadius(Hero->GetActorLocation(), LevelValue(RadiusByLevel), RollDamage(LevelValue(TickDamageMultByLevel)), GetDamageSource());

	const float FlameDamage = RollDamage(FlameTickDamageMult);
	for (FMLFlamePatch& Flame : Flames)
	{
		if (Flame.TimeLeft <= 0.f) continue;

		EnemySub->DamageInRadius(Flame.Center, FlameRadius, FlameDamage, GetDamageSource());
		Flame.TimeLeft -= TickInterval;
		if (Flame.TimeLeft <= 0.f && Flame.Actor)
		{
			Flame.Actor->SetActorHiddenInGame(true);
		}
	}
}

void UMLSkill_Aura::HandleEnemyKilled(const AMLEnemyActor* Killed, bool bElite)
{
	UWorld* World = GetWorld();
	const AActor* Hero = GetHero();
	if (!World || !Hero || !Killed || IsHeroDead()) return;

	// 처치 이벤트는 피해 처리 도중에 불리므로, 위치만 모아 두고 0.1초 뒤에 폭발 (재귀 방지)
	const FVector Location = Killed->GetActorLocation();
	if (FVector::DistSquared2D(Hero->GetActorLocation(), Location) > FMath::Square(LevelValue(RadiusByLevel))) return;

	PendingBursts.Add(Location);
	if (!World->GetTimerManager().IsTimerActive(BurstTimerHandle))
	{
		World->GetTimerManager().SetTimer(BurstTimerHandle, this, &UMLSkill_Aura::ProcessBursts, 0.1f, false);
	}
}

void UMLSkill_Aura::ProcessBursts()
{
	UWorld* World = GetWorld();
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!World || !EnemySub || IsHeroDead())
	{
		PendingBursts.Reset();
		return;
	}

	// 이번 단계 몫만 떼어 냄: 폭발로 죽은 적은 PendingBursts 뒤에 쌓여 다음 단계에서 연쇄
	const int32 Count = FMath::Min(PendingBursts.Num(), FMath::Max(1, MaxBurstsPerStep));
	TArray<FVector> Batch(PendingBursts.GetData(), Count);
	PendingBursts.RemoveAt(0, Count);

	const float Damage = RollDamage(BurstDamageMult);
	for (const FVector& Center : Batch)
	{
#if ENABLE_DRAW_DEBUG
		// shortcut: 디버그 원을 폭발 시각 효과로 사용, M10에서 Niagara로 교체
		DrawDebugCircle(World, Center, BurstRadius, 20, FColor(255, 90, 20), false, 0.2f, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
#endif
		EnemySub->DamageInRadius(Center, BurstRadius, Damage, GetDamageSource());
	}

	if (PendingBursts.Num() > 0 && !World->GetTimerManager().IsTimerActive(BurstTimerHandle))
	{
		World->GetTimerManager().SetTimer(BurstTimerHandle, this, &UMLSkill_Aura::ProcessBursts, 0.1f, false);
	}

	// MLVerify(M9-1c): M9-2에서 제거
	UE_LOG(LogTemp, Display, TEXT("[MLVerify] Purgatory burst: bursts=%d pending=%d"), Count, PendingBursts.Num());
}

void UMLSkill_Aura::DropFlame()
{
	UWorld* World = GetWorld();
	AActor* Hero = GetHero();
	const UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!World || !Hero || IsHeroDead() || !EnemySub || EnemySub->GetActiveCount() == 0) return; // 방 사이에는 남기지 않음

	FMLFlamePatch* Flame = Flames.FindByPredicate([](const FMLFlamePatch& F) { return F.TimeLeft <= 0.f; });
	if (!Flame)
	{
		Flame = &Flames.AddDefaulted_GetRef();
		if (FlameVisualClass)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Flame->Actor = World->SpawnActor<AMLSkillVisualActor>(FlameVisualClass, Hero->GetActorLocation(), FRotator::ZeroRotator, SpawnInfo);
		}
	}

	Flame->Center = Hero->GetActorLocation();
	Flame->TimeLeft = FlameDuration;
	if (Flame->Actor)
	{
		const float Scale = FlameRadius / FMath::Max(1.f, VisualBaseRadius);
		Flame->Actor->SetActorLocation(Flame->Center);
		Flame->Actor->SetActorScale3D(FVector(Scale, Scale, 1.f));
		Flame->Actor->SetActorHiddenInGame(false);
	}
}

void UMLSkill_Aura::Release()
{
	if (UMLEnemySubsystem* EnemySub = GetEnemySubsystem())
	{
		EnemySub->OnEnemyKilled.RemoveAll(this);
	}
	PendingBursts.Reset();
	if (AuraVisual)
	{
		AuraVisual->Destroy();
		AuraVisual = nullptr;
	}
	for (const FMLFlamePatch& Flame : Flames)
	{
		if (Flame.Actor) Flame.Actor->Destroy();
	}
	Flames.Reset();
	Super::Release();
}
