// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGhostActor.h"

#include "Enemy/MLEnemySubsystem.h"
#include "Skills/MLSkillVisualActor.h"
#include "TimerManager.h"

void AMLGhostActor::StartGhost(int32 InEnemyIndex, EMLAttackType InWeaponType, float InDamageScale)
{
	EnemyIndex = InEnemyIndex;
	WeaponType = InWeaponType;
	DamageScale = InDamageScale;
	SetEliteLook(true); // 크기 + 오버레이만; 특성은 없음
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AMLGhostActor::BeginAttack, AttackGap, false);
}

void AMLGhostActor::StopGhost()
{
	// 풀에 남는 액터라 죽어도 사라지지 않음: 타이머와 예고를 직접 정리
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	HideTelegraph();
}

void AMLGhostActor::BeginAttack()
{
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	FVector HeroLoc;
	if (!EnemySub || !EnemySub->IsEnemyActive(EnemyIndex) || EnemySub->GetEnemyActor(EnemyIndex) != this || !GetHeroLocation(HeroLoc)) return;

	const bool bBow = WeaponType == EMLAttackType::Bow;
	const bool bStaff = WeaponType == EMLAttackType::Staff;
	const float Reach = bBow ? BowLength : (bStaff ? StaffRange : SwordRadius);
	if (FVector::Dist2D(HeroLoc, GetActorLocation()) > Reach)
	{
		GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AMLGhostActor::BeginAttack, RetrySeconds, false);
		return;
	}

	EnemySub->SetEnemyRooted(EnemyIndex, true);
	AttackOrigin = bStaff ? HeroLoc : GetActorLocation();
	const FVector FloorOffset(0.f, 0.f, TelegraphZOffset);
	if (bBow)
	{
		AttackDir = (HeroLoc - AttackOrigin).GetSafeNormal2D();
		if (AttackDir.IsNearlyZero()) AttackDir = GetActorForwardVector().GetSafeNormal2D();
		ShowTelegraph(LineTelegraphClass, AttackOrigin + FloorOffset + AttackDir * (BowLength * 0.5f), AttackDir.Rotation(),
			FVector(BowLength / 100.f, BowWidth / 100.f, 1.f)); // 엔진 평면 = 100 × 100
	}
	else
	{
		const float Diameter = (bStaff ? StaffRadius : SwordRadius) * 2.f / 100.f;
		ShowTelegraph(CircleTelegraphClass, AttackOrigin + FloorOffset, FRotator::ZeroRotator, FVector(Diameter, Diameter, 1.f));
	}

	const float Seconds = bBow ? BowSeconds : (bStaff ? StaffSeconds : SwordSeconds);
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AMLGhostActor::ExecuteAttack, Seconds, false);
}

void AMLGhostActor::ExecuteAttack()
{
	HideTelegraph();
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	if (!EnemySub || !EnemySub->IsEnemyActive(EnemyIndex) || EnemySub->GetEnemyActor(EnemyIndex) != this) return;

	EnemySub->SetEnemyRooted(EnemyIndex, false);
	FVector HeroLoc;
	if (!GetHeroLocation(HeroLoc)) return;

	const bool bHit = IsHit(HeroLoc);
	if (bHit) EnemySub->DamageHero(AttackDamage * DamageScale);

	// MLVerify(M9-3): 다음 마일스톤에서 제거
	UE_LOG(LogTemp, Display, TEXT("[MLVerify] Ghost attack: weapon=%s hit=%d dist=%.0f damage=%.1f"),
		*StaticEnum<EMLAttackType>()->GetNameStringByValue(static_cast<int64>(WeaponType)), bHit ? 1 : 0,
		FVector::Dist2D(HeroLoc, AttackOrigin), AttackDamage * DamageScale);

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AMLGhostActor::BeginAttack, AttackGap, false);
}

bool AMLGhostActor::IsHit(const FVector& HeroLoc) const
{
	const FVector Offset = (HeroLoc - AttackOrigin) * FVector(1.f, 1.f, 0.f);
	if (WeaponType == EMLAttackType::Bow)
	{
		const float Along = FVector::DotProduct(Offset, AttackDir);
		const float Side = (Offset - AttackDir * Along).Size();
		return Along >= 0.f && Along <= BowLength && Side <= BowWidth * 0.5f;
	}
	return Offset.Size() <= (WeaponType == EMLAttackType::Staff ? StaffRadius : SwordRadius);
}
