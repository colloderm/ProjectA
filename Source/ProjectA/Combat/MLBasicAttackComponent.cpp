// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLBasicAttackComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/MLCombat.h"
#include "Combat/MLProjectileSubsystem.h"
#include "Combat/MLWeaponTypes.h"
#include "DrawDebugHelpers.h"
#include "Enemy/MLEnemySubsystem.h"
#include "GAS/MLAttributeSet.h"
#include "GAS/MLGameplayTags.h"
#include "TimerManager.h"

namespace
{
	const FName BasicAttackSource(TEXT("Basic")); // 피해 리포트 키 (design §6 HUD, M9-0)
}

UMLBasicAttackComponent::UMLBasicAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMLBasicAttackComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>())
	{
		EnemySub->RegisterDamageSource(BasicAttackSource, NSLOCTEXT("ML", "DamageSourceBasic", "기본 공격"));
	}
	ScheduleNextAttack();
}

const FMLWeaponRow* UMLBasicAttackComponent::GetWeaponRow() const
{
	return WeaponRowHandle.GetRow<FMLWeaponRow>(TEXT("MLBasicAttack"));
}

void UMLBasicAttackComponent::ScheduleNextAttack()
{
	const FMLWeaponRow* Weapon = GetWeaponRow();
	if (!Weapon || Weapon->BaseInterval <= 0.f) return;

	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	const float SpeedStat = ASC ? ASC->GetNumericAttribute(UMLAttributeSet::GetAttackSpeedAttribute()) : 0.f;
	const float Interval = Weapon->BaseInterval / FMath::Max(0.1f, 1.f + SpeedStat / 100.f);

	GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &UMLBasicAttackComponent::PerformAttack, Interval, false);
}

void UMLBasicAttackComponent::PerformAttack()
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC)
	{
		ScheduleNextAttack();
		return;
	}
	if (ASC->HasMatchingGameplayTag(MLTags::State_Dead.GetTag()))
	{
		return; // 공격을 완전히 멈춤
	}

	const FMLWeaponRow* Weapon = GetWeaponRow();
	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	if (!Weapon || !EnemySub) return;

	const FVector Origin = GetOwner()->GetActorLocation();
	const int32 TargetIndex = EnemySub->FindNearestEnemy(Origin, Weapon->Range);
	if (TargetIndex != INDEX_NONE)
	{
		const FVector TargetLoc = EnemySub->GetEnemyLocation(TargetIndex);
		const FVector AimDir = (TargetLoc - Origin).GetSafeNormal2D();
		const float Damage = MLCombat::RollHeroDamage(*ASC, Weapon->DamageMult * DamageScale);

		switch (Weapon->AttackType)
		{
		case EMLAttackType::Dagger:
			EnemySub->DamageEnemy(TargetIndex, Damage, BasicAttackSource);
#if ENABLE_DRAW_DEBUG
			DrawDebugLine(GetWorld(), Origin, TargetLoc, FColor::White, false, 0.1f, 0, 3.f);
#endif
			break;

		case EMLAttackType::Sword:
			EnemySub->DamageInArc(Origin, AimDir, Weapon->Range, Weapon->ArcAngle * 0.5f, Damage, BasicAttackSource);
#if ENABLE_DRAW_DEBUG
			DrawDebugLine(GetWorld(), Origin, Origin + AimDir.RotateAngleAxis(Weapon->ArcAngle * 0.5f, FVector::UpVector) * Weapon->Range, FColor::Yellow, false, 0.15f, 0, 3.f);
			DrawDebugLine(GetWorld(), Origin, Origin + AimDir.RotateAngleAxis(-Weapon->ArcAngle * 0.5f, FVector::UpVector) * Weapon->Range, FColor::Yellow, false, 0.15f, 0, 3.f);
#endif
			break;

		case EMLAttackType::Bow:
			if (UMLProjectileSubsystem* ProjectileSub = GetWorld()->GetSubsystem<UMLProjectileSubsystem>())
			{
				FMLProjectileLaunch Shot;
				Shot.ProjectileClass = Weapon->ProjectileClass;
				Shot.Start = Origin;
				Shot.Velocity = AimDir * Weapon->ProjectileSpeed;
				Shot.Damage = Damage;
				Shot.HitRadius = Weapon->ProjectileHitRadius;
				Shot.MaxRange = Weapon->Range;
				Shot.Source = BasicAttackSource;
				ProjectileSub->Launch(Shot);
			}
			break;

		case EMLAttackType::Staff:
			EnemySub->DamageInRadius(TargetLoc, Weapon->AoERadius, Damage, BasicAttackSource);
#if ENABLE_DRAW_DEBUG
			DrawDebugCircle(GetWorld(), TargetLoc, Weapon->AoERadius, 24, FColor::Cyan, false, 0.2f, 0, 3.f,
				FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
#endif
			break;
		}
	}

	ScheduleNextAttack();
}
