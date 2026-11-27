// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLAutoSkill.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/MLCombat.h"
#include "Combat/MLProjectileSubsystem.h"
#include "Enemy/MLEnemySubsystem.h"
#include "Engine/World.h"
#include "GAS/MLAttributeSet.h"
#include "GAS/MLGameplayTags.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Skills/MLSkillVisualActor.h"
#include "TimerManager.h"

UWorld* UMLAutoSkill::GetWorld() const
{
	return HeroActor ? HeroActor->GetWorld() : nullptr; // CDO에는 영웅이 없음 -> 에디터가 기대하는 대로 nullptr
}

void UMLAutoSkill::Acquire(AActor* InHero)
{
	HeroActor = InHero;
	SkillLevel = 1;

	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (EnemySub && !bHostile) // 망령의 피해는 피해 리포트에 넣지 않음
	{
		EnemySub->RegisterDamageSource(GetDamageSource(), SkillName);
	}
	OnAcquired();
	OnLevelChanged();

	if (GetEffectiveCooldown() > 0.f)
	{
		ScheduleFire(FirstFireDelay);
	}
}

void UMLAutoSkill::AcquireHostile(AActor* InOwner, TSubclassOf<AMLSkillVisualActor> InTelegraphClass, UMaterialInterface* InOverlay, float InTelegraphZOffset)
{
	bHostile = true; // Acquire 안의 OnAcquired가 이미 적대 모드를 보도록 먼저
	HostileTelegraphClass = InTelegraphClass;
	HostileOverlay = InOverlay;
	HostileTelegraphZOffset = InTelegraphZOffset;
	Acquire(InOwner);
}

void UMLAutoSkill::LevelUp()
{
	if (SkillLevel >= MaxSkillLevel) return;

	++SkillLevel;
	OnLevelChanged();
}

void UMLAutoSkill::Release()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this); // Fire + 하위 클래스 타이머 (칼날, 장판)
	}
	HeroActor = nullptr;
}

float UMLAutoSkill::GetEffectiveCooldown() const
{
	const float BaseCooldown = LevelValue(CooldownByLevel);
	if (BaseCooldown <= 0.f) return 0.f;

	const UAbilitySystemComponent* ASC = GetHeroASC();
	const float CDR = ASC ? ASC->GetNumericAttribute(UMLAttributeSet::GetSkillCooldownReductionAttribute()) : 0.f;
	return FMath::Max(0.05f, BaseCooldown * (1.f - CDR) * (bHostile ? HostileCooldownMult : 1.f));
}

void UMLAutoSkill::ScheduleFire(float Delay)
{
	UWorld* World = GetWorld();
	if (!World || Delay <= 0.f) return;

	// 반복하지 않고 Fire마다 다시 예약하므로 쿨타임 패시브가 바로 적용됨.
	World->GetTimerManager().SetTimer(FireTimerHandle, this, &UMLAutoSkill::HandleFire, Delay, false);
}

void UMLAutoSkill::HandleFire()
{
	if (IsHeroDead()) return; // 완전히 멈춤

	// 맞힐 대상 없음 (방 클리어 / 방 사이): 시전을 건너뛰고 잠시 후 다시 확인.
	if (!HasTargets())
	{
		ScheduleFire(IdleRetryDelay);
		return;
	}

	Fire();
	ScheduleFire(GetEffectiveCooldown());
}

UAbilitySystemComponent* UMLAutoSkill::GetHeroASC() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HeroActor);
}

UMLEnemySubsystem* UMLAutoSkill::GetEnemySubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UMLEnemySubsystem>() : nullptr;
}

bool UMLAutoSkill::IsHeroDead() const
{
	const UAbilitySystemComponent* ASC = GetHeroASC();
	return !ASC || ASC->HasMatchingGameplayTag(MLTags::State_Dead.GetTag());
}

float UMLAutoSkill::RollDamage(float DamageMult, bool bForceCrit) const
{
	const UAbilitySystemComponent* ASC = GetHeroASC();
	return ASC ? MLCombat::RollHeroDamage(*ASC, DamageMult, bForceCrit) : 0.f;
}

AMLHeroCharacter* UMLAutoSkill::GetTargetHero() const
{
	AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(HeroActor, 0));
	return Hero && !Hero->IsDead() ? Hero : nullptr;
}

bool UMLAutoSkill::HasTargets() const
{
	if (bHostile) return GetTargetHero() != nullptr;
	const UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	return EnemySub && EnemySub->GetActiveCount() > 0;
}

int32 UMLAutoSkill::FindTarget(const FVector& Origin, float Range) const
{
	if (bHostile)
	{
		const AMLHeroCharacter* Hero = GetTargetHero();
		return Hero && FVector::Dist2D(Origin, Hero->GetActorLocation()) <= Range ? HeroTarget : INDEX_NONE;
	}
	const UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	return EnemySub ? EnemySub->FindNearestEnemy(Origin, Range) : INDEX_NONE;
}

FVector UMLAutoSkill::GetTargetLocation(int32 Target) const
{
	if (Target == HeroTarget)
	{
		const AMLHeroCharacter* Hero = GetTargetHero();
		return Hero ? Hero->GetActorLocation() : FVector::ZeroVector;
	}
	const UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	return EnemySub ? EnemySub->GetEnemyLocation(Target) : FVector::ZeroVector;
}

int32 UMLAutoSkill::DamageArea(const FVector& Center, float Radius, float Damage)
{
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!EnemySub) return 0;
	if (!bHostile) return EnemySub->DamageInRadius(Center, Radius, Damage, GetDamageSource());

	const AMLHeroCharacter* Hero = GetTargetHero();
	if (!Hero || FVector::Dist2D(Center, Hero->GetActorLocation()) > Radius) return 0;
	EnemySub->DamageHeroBySkill(Damage);
	return 1;
}

bool UMLAutoSkill::LaunchShot(FMLProjectileLaunch Shot) const
{
	const UWorld* World = GetWorld();
	UMLProjectileSubsystem* ProjectileSub = World ? World->GetSubsystem<UMLProjectileSubsystem>() : nullptr;
	if (!ProjectileSub) return false;

	if (bHostile)
	{
		Shot.bHostile = true;
		Shot.Overlay = HostileOverlay;
		Shot.HomingTurnRate *= HostileTurnRateMult;
	}
	return ProjectileSub->Launch(Shot);
}

AMLSkillVisualActor* UMLAutoSkill::SpawnVisual(TSubclassOf<AMLSkillVisualActor> VisualClass, const FVector& Location) const
{
	UWorld* World = GetWorld();
	if (!World || !VisualClass) return nullptr;

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMLSkillVisualActor* Visual = World->SpawnActor<AMLSkillVisualActor>(VisualClass, Location, FRotator::ZeroRotator, SpawnInfo);
	if (Visual && bHostile) Visual->SetOverlay(HostileOverlay); // 영웅의 같은 기술과 구분
	return Visual;
}

void UMLAutoSkill::HostileStrike(const FVector& Center, float Radius, float Damage, float SlowMultiplier, float SlowSeconds)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (HostileTelegraphClass)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector Floor = Center + FVector(0.f, 0.f, HostileTelegraphZOffset);
		if (AActor* Telegraph = World->SpawnActor<AMLSkillVisualActor>(HostileTelegraphClass, Floor, FRotator::ZeroRotator, SpawnInfo))
		{
			Telegraph->SetActorScale3D(FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, 1.f)); // 엔진 평면 = 100 × 100
			Telegraph->SetLifeSpan(HostileStrikeDelay);
		}
	}

	// 약한 람다: Release(ClearAllTimersForObject) 뒤에는 실행되지 않음
	FTimerHandle StrikeHandle;
	World->GetTimerManager().SetTimer(StrikeHandle, FTimerDelegate::CreateWeakLambda(this, [this, Center, Radius, Damage, SlowMultiplier, SlowSeconds]()
	{
		AMLHeroCharacter* Hero = GetTargetHero();
		UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
		const bool bHit = Hero && EnemySub && FVector::Dist2D(Center, Hero->GetActorLocation()) <= Radius;
		if (bHit)
		{
			EnemySub->DamageHeroBySkill(Damage);
			if (SlowSeconds > 0.f) Hero->ApplySlow(SlowMultiplier, SlowSeconds);
		}
	}), HostileStrikeDelay, false);
}
