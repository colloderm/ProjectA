// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGhostActor.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemy/MLEnemySubsystem.h"
#include "GAS/MLAttributeSet.h"
#include "GAS/Effects/MLGE_Passive.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Run/MLPassiveTypes.h"
#include "Run/MLRunProgressComponent.h"
#include "Skills/MLAutoSkill.h"
#include "Skills/MLSkillVisualActor.h"
#include "TimerManager.h"

AMLGhostActor::AMLGhostActor()
{
	GhostASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("GhostASC"));
	GhostAttributes = CreateDefaultSubobject<UMLAttributeSet>(TEXT("GhostAttributes"));

	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f)); // 액터 Z = 영웅 캡슐 중심, 메시는 +Y를 봄
	BodyMesh->SetCollisionProfileName(TEXT("NoCollision")); // 명중은 서브시스템의 거리 판정으로 처리
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCanEverAffectNavigation(false);
}

void AMLGhostActor::BeginPlay()
{
	Super::BeginPlay();
	GhostASC->InitAbilityActorInfo(this, this);
	if (MoveBlendSpace) BodyMesh->PlayAnimation(MoveBlendSpace, true);
}

void AMLGhostActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopGhost(); // 기술의 타이머와 시각 효과 정리
	Super::EndPlay(EndPlayReason);
}

void AMLGhostActor::SetMoving(bool bMoving)
{
	if (UAnimSingleNodeInstance* Anim = BodyMesh->GetSingleNodeInstance())
	{
		Anim->SetBlendSpacePosition(FVector(0.f, bMoving ? MoveSpeed : 0.f, 0.f));
	}
}

void AMLGhostActor::StartGhost(int32 InEnemyIndex, EMLAttackType InWeaponType, const TArray<FSoftClassPath>& InSkills, const TArray<FName>& InPassives)
{
	EnemyIndex = InEnemyIndex;
	WeaponType = InWeaponType;
	BodyMesh->SetOverlayMaterial(EliteOverlayMaterial); // 액터 크기는 그대로: 키우면 발이 바닥에 묻힘 (크기는 BP의 BodyMesh에서)
	SetMoving(true);

	// design §6.2.3: 공격력 = BP 값, 최대 체력 = 적 서브시스템 값(구역·등급 배율 포함). 그 위에 패시브 Lv1
	UMLEnemySubsystem* EnemySub = GetEnemySubsystem();
	GhostASC->SetNumericAttributeBase(UMLAttributeSet::GetMaxHealthAttribute(), EnemySub ? EnemySub->GetEnemyMaxHealth(EnemyIndex) : 1.f);
	GhostASC->SetNumericAttributeBase(UMLAttributeSet::GetAttackAttribute(), AttackDamage);
	ApplyPassives(InPassives);
	if (EnemySub) EnemySub->SetEnemyMaxHealth(EnemyIndex, GhostASC->GetNumericAttribute(UMLAttributeSet::GetMaxHealthAttribute()));

	// 수호 방패: 망령 ASC의 보호막을 적 서브시스템이 흡수하고, 깨지면 되돌려 알림
	GhostASC->GetGameplayAttributeValueChangeDelegate(UMLAttributeSet::GetShieldAttribute()).AddUObject(this, &AMLGhostActor::HandleShieldChanged);
	if (EnemySub) EnemySub->OnEnemyShieldBroken.AddUObject(this, &AMLGhostActor::HandleShieldBroken);

	for (const FSoftClassPath& Path : InSkills)
	{
		UClass* SkillClass = Path.TryLoadClass<UMLAutoSkill>(); // 지워진 기술 BP는 건너뜀
		if (!SkillClass || !SkillClass->IsChildOf(UMLAutoSkill::StaticClass()) || SkillClass->HasAnyClassFlags(CLASS_Abstract)) continue;

		UMLAutoSkill* Skill = NewObject<UMLAutoSkill>(this, SkillClass);
		Skills.Add(Skill);
		Skill->AcquireHostile(this, CircleTelegraphClass, SkillOverlayMaterial, TelegraphZOffset);
	}

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AMLGhostActor::BeginAttack, AttackGap, false);
}

void AMLGhostActor::ApplyPassives(const TArray<FName>& Passives)
{
	// DT_Passives는 영웅의 RunProgress가 들고 있음. 적에게 없는 능력치(방어·재생·획득 반경·경험치)는 효과 없음
	const AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const UMLRunProgressComponent* RunProgress = Hero ? Hero->GetRunProgress() : nullptr;
	if (!RunProgress) return;

	for (const FName& RowName : Passives)
	{
		const FMLPassiveRow* Row = RunProgress->FindPassiveRow(RowName);
		if (!Row) continue;

		const FGameplayEffectSpecHandle Spec = UMLGE_Passive::MakeSpec(*GhostASC, Row->Stat, Row->ValuePerLevel); // Lv1 = 1번
		if (Spec.IsValid()) GhostASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void AMLGhostActor::HandleShieldChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.NewValue <= ChangeData.OldValue) return; // 줄어드는 쪽은 적 서브시스템이 먼저 처리함
	if (UMLEnemySubsystem* EnemySub = GetEnemySubsystem()) EnemySub->SetEnemyShield(EnemyIndex, ChangeData.NewValue);
}

void AMLGhostActor::HandleShieldBroken(int32 BrokenIndex)
{
	if (BrokenIndex != EnemyIndex) return;

	// 피해 처리 도중: 다음 틱에 보호막을 0으로 → 수호 방패 표시가 꺼지고 눈보라 파편이 나감
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		GhostASC->SetNumericAttributeBase(UMLAttributeSet::GetShieldAttribute(), 0.f);
	}));
}

void AMLGhostActor::StopGhost()
{
	// 풀에 남는 액터라 죽어도 사라지지 않음: 타이머와 예고, 기술을 직접 정리
	GetWorldTimerManager().ClearAllTimersForObject(this);
	HideTelegraph();

	GhostASC->GetGameplayAttributeValueChangeDelegate(UMLAttributeSet::GetShieldAttribute()).RemoveAll(this);
	if (UMLEnemySubsystem* EnemySub = GetEnemySubsystem()) EnemySub->OnEnemyShieldBroken.RemoveAll(this);

	for (const TObjectPtr<UMLAutoSkill>& Skill : Skills)
	{
		if (Skill) Skill->Release();
	}
	Skills.Reset();
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
	SetMoving(false);
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
	SetMoving(true);
	FVector HeroLoc;
	if (!GetHeroLocation(HeroLoc)) return;

	// 공격력·공격 속도 패시브 반영 (패시브가 없으면 BP의 AttackDamage, AttackGap 그대로)
	const float Damage = GhostASC->GetNumericAttribute(UMLAttributeSet::GetAttackAttribute());
	const float Gap = AttackGap / FMath::Max(0.1f, 1.f + GhostASC->GetNumericAttribute(UMLAttributeSet::GetAttackSpeedAttribute()) / 100.f);
	const bool bHit = IsHit(HeroLoc);
	if (bHit) EnemySub->DamageHero(Damage);

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AMLGhostActor::BeginAttack, Gap, false);
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
