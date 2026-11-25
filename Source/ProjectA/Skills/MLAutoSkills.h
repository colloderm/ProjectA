// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "MLAutoSkill.h"
#include "MLAutoSkills.generated.h"

class AMLSkillVisualActor;
class AMLProjectileActor;
class AMLEnemyActor;
struct FOnAttributeChangeData;

// ---------- 회전 칼날: 쿨타임마다 지속시간 동안 영웅 주위를 도는 칼날 (CooldownByLevel이 비면 상시형 = 진화 칼날 폭풍) ----------
UCLASS()
class PROJECTA_API UMLSkill_Blades : public UMLAutoSkill
{
	GENERATED_BODY()

public:
	UMLSkill_Blades();
	virtual void Release() override;

protected:
	virtual void OnAcquired() override;
	virtual void OnLevelChanged() override;
	virtual void Fire() override;

private:
	void ApplyBladeDamage();
	void SetBladesActive(bool bActive);
	void HandleBladesEnd();
	void StartPulse();
	void StepPulse();
	void SetOrbitRadius(float Radius);

	// 칼날 소용돌이 (2단계 칼날 폭풍, M9-1b): 0이면 끔. 이 주기(초)마다 궤도가 PulseRadius까지 넓어졌다 돌아옴
	UPROPERTY(EditDefaultsOnly, Category = "Blades|Pulse")
	float PulseInterval = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Blades|Pulse")
	float PulseRadius = 500.f;

	// 넓어졌다 돌아오는 데 걸리는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Blades|Pulse")
	float PulseSeconds = 1.f;

	FTimerHandle PulseTimerHandle;
	FTimerHandle PulseStepHandle;
	float PulseElapsed = 0.f;
	static constexpr float PulseStepSeconds = 0.03f;

	void FireShots();

	// 천 개의 칼날 (3단계, M9-1c): 0이면 끔. 이 주기(초)마다 칼날마다 바깥 방향으로 칼날 탄 1발
	UPROPERTY(EditDefaultsOnly, Category = "Blades|Shots")
	float ShotInterval = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Blades|Shots")
	TSubclassOf<AMLProjectileActor> ShotClass;

	UPROPERTY(EditDefaultsOnly, Category = "Blades|Shots")
	float ShotDamageMult = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Blades|Shots")
	float ShotSpeed = 1100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Blades|Shots")
	float ShotRange = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Blades|Shots")
	float ShotHitRadius = 50.f;

	FTimerHandle ShotTimerHandle;

	// RotatingMovementComponent가 있는 BP (yaw 속도 = 궤도 속도), 메시는 필요 없음
	UPROPERTY(EditDefaultsOnly, Category = "Blades")
	TSubclassOf<AMLSkillVisualActor> PivotClass;

	UPROPERTY(EditDefaultsOnly, Category = "Blades")
	TSubclassOf<AMLSkillVisualActor> BladeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Blades")
	TArray<int32> BladeCountByLevel = { 2, 3, 4 };

	UPROPERTY(EditDefaultsOnly, Category = "Blades")
	TArray<float> DamageMultByLevel = { 0.8f, 1.f, 1.2f };

	// 한 번 나타났을 때 도는 시간(초). 주기는 CooldownByLevel(나타나는 시점부터 다음 등장까지)
	UPROPERTY(EditDefaultsOnly, Category = "Blades")
	TArray<float> DurationByLevel = { 2.f, 3.f, 4.f };

	UPROPERTY(EditDefaultsOnly, Category = "Blades")
	float OrbitRadius = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "Blades")
	float HitRadius = 60.f;

	// shortcut: 적별 피격 쿨타임 없음; 칼날에 닿은 적은 초당 약 1/HitInterval번 맞음 — 칼날이 너무 강하면 적별 타이머 추가
	UPROPERTY(EditDefaultsOnly, Category = "Blades")
	float HitInterval = 0.3f;

	UPROPERTY(Transient)
	TObjectPtr<AMLSkillVisualActor> Pivot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AMLSkillVisualActor>> Blades;

	FTimerHandle DamageTimerHandle;
	FTimerHandle ActiveTimerHandle;
	bool bBladesActive = false;
};

// ---------- 번개 사슬: 가장 가까운 적에서 시작해 다음으로 가까운 적에게 튐 ----------
UCLASS()
class PROJECTA_API UMLSkill_ChainLightning : public UMLAutoSkill
{
	GENERATED_BODY()

public:
	UMLSkill_ChainLightning();

protected:
	virtual void Fire() override;

	// From에서 FirstSearchRange 안의 가장 가까운 적부터 최대 MaxTargets명까지 연쇄. HitIndices에 있는 적은 건너뜀
	void ChainFrom(FVector From, float FirstSearchRange, int32 MaxTargets, float Damage, TArray<int32>& HitIndices);

	UPROPERTY(EditDefaultsOnly, Category = "Chain")
	float FirstRange = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Chain")
	float ChainRange = 350.f;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Chain")
	TArray<int32> TargetCountByLevel = { 3, 5, 7 };

	UPROPERTY(EditDefaultsOnly, Category = "Chain")
	TArray<float> DamageMultByLevel = { 1.0f, 1.4f, 1.8f };
};

// ---------- 천둥 심판 (2단계, M9-1b): 범위 안 무작위 적 머리 위에 낙뢰 + 맞은 지점에서 연쇄 ----------
UCLASS()
class PROJECTA_API UMLSkill_ThunderStrike : public UMLSkill_ChainLightning
{
	GENERATED_BODY()

public:
	UMLSkill_ThunderStrike();

protected:
	virtual void OnAcquired() override;
	virtual void Fire() override;

private:
	// 대상 적 머리 위에 낙뢰 1발 + 그 자리에서 연쇄. 범위 피해로 맞힌 수를 반환
	int32 StrikeAt(int32 TargetIndex, float StrikeDamage, float ChainDamage);
	void StartStorm();
	void StormStrike();

	// 뇌신강림 (3단계, M9-1c): 0이면 끔. 이 주기(초)마다 StormSeconds 동안 StormRange 안에 낙뢰 StormStrikes발 (항상 치명타)
	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Storm")
	float StormInterval = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Storm")
	int32 StormStrikes = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Storm")
	float StormSeconds = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Storm")
	float StormRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Storm")
	float StormDamageMult = 1.6f;

	FTimerHandle StormTimerHandle;
	FTimerHandle StormStrikeHandle;
	int32 StormLeft = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	TArray<int32> StrikeCountByLevel = { 3, 4, 5 };

	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	TArray<float> StrikeDamageMultByLevel = { 1.2f, 1.4f, 1.6f };

	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	TArray<float> ChainDamageMultByLevel = { 0.8f, 0.9f, 1.05f };

	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	float StrikeRange = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	float StrikeRadius = 150.f;

	// 낙뢰 하나마다 근처로 튀는 연쇄 수
	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	int32 ChainPerStrike = 3;
};

// ---------- 화염 장판: 영웅 근처 무작위 위치에 피해 장판 ----------
USTRUCT()
struct FMLFireFieldEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AMLSkillVisualActor> Actor = nullptr;

	FVector Center = FVector::ZeroVector;
	float Radius = 0.f;
	float TimeLeft = 0.f;
	int32 TotalHits = 0;
	bool bActive = false;
};

UCLASS()
class PROJECTA_API UMLSkill_FireField : public UMLAutoSkill
{
	GENERATED_BODY()

public:
	UMLSkill_FireField();
	virtual void Release() override;

protected:
	virtual void OnAcquired() override;
	virtual void Fire() override;

private:
	void TickFields();

	UPROPERTY(EditDefaultsOnly, Category = "FireField")
	TSubclassOf<AMLSkillVisualActor> FieldClass;

	// 액터 스케일 1에서 장판 메시 반경 (엔진 Cylinder = 50)
	UPROPERTY(EditDefaultsOnly, Category = "FireField")
	float VisualBaseRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "FireField")
	TArray<float> RadiusByLevel = { 150.f, 190.f, 230.f };

	UPROPERTY(EditDefaultsOnly, Category = "FireField")
	TArray<float> DurationByLevel = { 3.f, 4.f, 5.f };

	UPROPERTY(EditDefaultsOnly, Category = "FireField")
	float TickDamageMult = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "FireField")
	float TickInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "FireField")
	float SpawnRange = 600.f;

	UPROPERTY(Transient)
	TArray<FMLFireFieldEntry> Fields;

	FTimerHandle FieldTimerHandle;
};

// ---------- 수호 방패: 주기적 쉴드 = 최대 HP의 % (덮어쓰기, 중첩 없음) ----------
UCLASS()
class PROJECTA_API UMLSkill_GuardShield : public UMLAutoSkill
{
	GENERATED_BODY()

public:
	UMLSkill_GuardShield();
	virtual void Release() override;

protected:
	virtual void OnAcquired() override;
	virtual void Fire() override;

	// 쉴드가 0보다 크다가 0이 되는 순간 (M9-1b 눈보라의 파편 폭발)
	virtual void OnShieldBroken() {}

private:
	void HandleShieldChanged(const FOnAttributeChangeData& ChangeData);

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	TSubclassOf<AMLSkillVisualActor> ShieldVisualClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	TArray<float> ShieldPercentByLevel = { 0.15f, 0.25f, 0.35f };

	UPROPERTY(Transient)
	TObjectPtr<AMLSkillVisualActor> ShieldVisual;
};

// ---------- 눈보라 (2단계, M9-1b): 주기마다 냉기 폭발(피해 + 결빙) + 쉴드가 없으면 얼음 쉴드, 쉴드가 깨지면 파편 폭발 ----------
UCLASS()
class PROJECTA_API UMLSkill_FrostNova : public UMLSkill_GuardShield
{
	GENERATED_BODY()

public:
	UMLSkill_FrostNova();
	virtual void Release() override;

protected:
	virtual void OnAcquired() override;
	virtual void Fire() override;
	virtual void OnShieldBroken() override;

private:
	void StepNovaVisual();
	void CastGlacier();

	// 절대 영도 (3단계, M9-1c): 0이면 끔. 이 주기(초)마다 GlacierRadius 안 모든 적을 결빙 + 큰 피해
	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Glacier")
	float GlacierInterval = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Glacier")
	float GlacierRadius = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Glacier")
	float GlacierFreezeSeconds = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Glacier")
	float GlacierDamageMult = 2.f;

	// 화면 전체 원판 (0.5초 표시). 액터 스케일 1에서 반경 = NovaVisualBaseRadius
	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Glacier")
	TSubclassOf<AMLSkillVisualActor> GlacierVisualClass;

	UPROPERTY(Transient)
	TObjectPtr<AMLSkillVisualActor> GlacierVisual;

	FTimerHandle GlacierTimerHandle;
	FTimerHandle GlacierHideHandle;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova")
	TArray<float> NovaRadiusByLevel = { 350.f, 380.f, 420.f };

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova")
	TArray<float> NovaDamageMultByLevel = { 1.f, 1.15f, 1.3f };

	// 결빙(이동 0) 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "FrostNova")
	float FreezeSeconds = 1.f;

	// 퍼져 나가는 원판. 액터 스케일 1에서 반경 = NovaVisualBaseRadius
	UPROPERTY(EditDefaultsOnly, Category = "FrostNova")
	TSubclassOf<AMLSkillVisualActor> NovaVisualClass;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova")
	float NovaVisualBaseRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova")
	float NovaGrowSeconds = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Shards")
	TSubclassOf<AMLProjectileActor> ShardClass;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Shards")
	int32 BreakShardCount = 24;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Shards")
	float ShardDamageMult = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Shards")
	float ShardSlow = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Shards")
	float ShardSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "FrostNova|Shards")
	float ShardRange = 700.f;

	UPROPERTY(Transient)
	TObjectPtr<AMLSkillVisualActor> NovaVisual;

	FTimerHandle NovaGrowHandle;
	float NovaElapsed = 0.f;
	float NovaTargetRadius = 0.f;
};

// 얼음 파편 한 레벨의 발사 모양 (M9-1b)
USTRUCT()
struct FMLShardPattern
{
	GENERATED_BODY()

	FMLShardPattern() = default;
	FMLShardPattern(int32 InCount, float InArcDeg, bool bInMirror) : Count(InCount), ArcDeg(InArcDeg), bMirror(bInMirror) {}

	UPROPERTY(EditAnywhere)
	int32 Count = 8;

	// 펼침 각도. 360 이상이면 원형
	UPROPERTY(EditAnywhere)
	float ArcDeg = 360.f;

	// 반대 방향에도 같은 부채꼴을 한 번 더
	UPROPERTY(EditAnywhere)
	bool bMirror = false;
};

// ---------- 얼음 파편: 가장 가까운 적 방향으로 레벨별 모양(부채꼴 → 앞뒤 → 원형)으로 쏘고 맞히면 둔화 ----------
UCLASS()
class PROJECTA_API UMLSkill_IceShards : public UMLAutoSkill
{
	GENERATED_BODY()

public:
	UMLSkill_IceShards();

protected:
	virtual void Fire() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "IceShards")
	TSubclassOf<AMLProjectileActor> ShardClass;

	// Lv1 부채꼴 3발 / Lv2 앞뒤 3+3발 / Lv3 원형 12발
	UPROPERTY(EditDefaultsOnly, Category = "IceShards")
	TArray<FMLShardPattern> PatternByLevel = { FMLShardPattern(3, 40.f, false), FMLShardPattern(3, 40.f, true), FMLShardPattern(12, 360.f, false) };

	// 줄어드는 이동 속도 비율 (0.3 = 30% 느려짐)
	UPROPERTY(EditDefaultsOnly, Category = "IceShards")
	TArray<float> SlowByLevel = { 0.3f, 0.4f, 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "IceShards")
	float SlowDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "IceShards")
	float DamageMult = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "IceShards")
	float ShardSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "IceShards")
	float ShardRange = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "IceShards")
	float ShardHitRadius = 40.f;
};

// ---------- 유도 마탄: 가장 가까운 적을 향해 휘어지는 탄 ----------
UCLASS()
class PROJECTA_API UMLSkill_HomingBolt : public UMLAutoSkill
{
	GENERATED_BODY()

public:
	UMLSkill_HomingBolt();

protected:
	virtual void Fire() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Homing")
	TSubclassOf<AMLProjectileActor> BoltClass;

	UPROPERTY(EditDefaultsOnly, Category = "Homing")
	TArray<int32> BoltCountByLevel = { 1, 2, 4 };

	UPROPERTY(EditDefaultsOnly, Category = "Homing")
	TArray<float> DamageMultByLevel = { 0.8f, 1.1f, 1.4f };

	UPROPERTY(EditDefaultsOnly, Category = "Homing")
	float BoltSpeed = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "Homing")
	float TurnRateDeg = 360.f;

	UPROPERTY(EditDefaultsOnly, Category = "Homing")
	float BoltRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Homing")
	float BoltHitRadius = 50.f;

	// 발사 시 탄 사이 부채꼴 각도 (도)
	UPROPERTY(EditDefaultsOnly, Category = "Homing")
	float SpreadAngle = 20.f;
};

// 지옥불이 남기는 불길 하나
USTRUCT()
struct FMLFlamePatch
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AMLSkillVisualActor> Actor = nullptr;

	FVector Center = FVector::ZeroVector;
	float TimeLeft = 0.f;
};

// ---------- 오라 (1단계, M9-1b): 영웅 주변 상시 피해. 불길 옵션(FlameInterval > 0)을 켜면 2단계 지옥불 ----------
UCLASS()
class PROJECTA_API UMLSkill_Aura : public UMLAutoSkill
{
	GENERATED_BODY()

public:
	virtual void Release() override;

protected:
	virtual void OnAcquired() override;
	virtual void OnLevelChanged() override;

private:
	void TickAura();
	void DropFlame();
	void HandleEnemyKilled(const AMLEnemyActor* Killed, bool bElite);
	void ProcessBursts();

	// 연옥 (3단계, M9-1c): 오라 안에서 죽은 적이 폭발. 폭발로 죽은 적도 다시 폭발 (0.1초 간격 연쇄)
	UPROPERTY(EditDefaultsOnly, Category = "Aura|Burst")
	bool bDeathBurst = false;

	UPROPERTY(EditDefaultsOnly, Category = "Aura|Burst")
	float BurstRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aura|Burst")
	float BurstDamageMult = 0.8f;

	// 한 번(0.1초)에 처리하는 폭발 수 상한: 큰 무리에서 연쇄가 한 프레임에 몰리지 않게
	UPROPERTY(EditDefaultsOnly, Category = "Aura|Burst")
	int32 MaxBurstsPerStep = 20;

	TArray<FVector> PendingBursts;
	FTimerHandle BurstTimerHandle;

	// 영웅에 붙는 원판. 액터 스케일 1에서 반경 = VisualBaseRadius
	UPROPERTY(EditDefaultsOnly, Category = "Aura")
	TSubclassOf<AMLSkillVisualActor> AuraVisualClass;

	UPROPERTY(EditDefaultsOnly, Category = "Aura")
	float VisualBaseRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aura")
	TArray<float> RadiusByLevel = { 200.f, 240.f, 280.f };

	UPROPERTY(EditDefaultsOnly, Category = "Aura")
	TArray<float> TickDamageMultByLevel = { 0.25f, 0.3f, 0.35f };

	UPROPERTY(EditDefaultsOnly, Category = "Aura")
	float TickInterval = 0.5f;

	// 지옥불 전용: 0이면 끔. 이 주기(초)마다 영웅이 선 자리에 불길을 남김
	UPROPERTY(EditDefaultsOnly, Category = "Aura|Flame")
	float FlameInterval = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aura|Flame")
	TSubclassOf<AMLSkillVisualActor> FlameVisualClass;

	UPROPERTY(EditDefaultsOnly, Category = "Aura|Flame")
	float FlameRadius = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aura|Flame")
	float FlameDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aura|Flame")
	float FlameTickDamageMult = 0.3f;

	UPROPERTY(Transient)
	TObjectPtr<AMLSkillVisualActor> AuraVisual;

	UPROPERTY(Transient)
	TArray<FMLFlamePatch> Flames;

	FTimerHandle AuraTimerHandle;
	FTimerHandle FlameTimerHandle;
};
