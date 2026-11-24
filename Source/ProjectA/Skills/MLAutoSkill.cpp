// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLAutoSkill.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/MLCombat.h"
#include "Enemy/MLEnemySubsystem.h"
#include "Engine/World.h"
#include "GAS/MLAttributeSet.h"
#include "GAS/MLGameplayTags.h"
#include "TimerManager.h"

UWorld* UMLAutoSkill::GetWorld() const
{
	return HeroActor ? HeroActor->GetWorld() : nullptr; // CDO에는 영웅이 없음 -> 에디터가 기대하는 대로 nullptr
}

void UMLAutoSkill::Acquire(AActor* InHero)
{
	HeroActor = InHero;
	SkillLevel = 1;

	if (UMLEnemySubsystem* EnemySub = GetEnemySubsystem())
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
	return FMath::Max(0.05f, BaseCooldown * (1.f - CDR));
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
	const UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	const bool bIdle = !EnemySub || EnemySub->GetActiveCount() == 0;

	if (bIdle)
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
