// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MLEnemySubsystem.generated.h"

class AMLEnemyActor;
class AMLHeroCharacter;

DECLARE_MULTICAST_DELEGATE_TwoParams(FMLOnEnemyKilled, const AMLEnemyActor* /*Killed*/, bool /*bElite*/);

// design §6.5 엘리트 특성. None = 일반 적.
enum class EMLEliteTrait : uint8
{
	None,
	Swift,  // 이동 속도 x1.5
	Shield, // 최대 HP의 50%를 체력보다 먼저 흡수
	Regen,  // 초당 최대 HP의 3% 회복
	Count
};

USTRUCT()
struct FMLEnemyEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AMLEnemyActor> Actor = nullptr;

	float Health = 0.f;
	float MoveSpeed = 0.f;
	float ContactDamage = 0.f;
	float MaxHealth = 0.f;
	float ShieldLeft = 0.f;
	EMLEliteTrait Trait = EMLEliteTrait::None;
	float SlowFactor = 1.f;
	float SlowTimeLeft = 0.f;
	float GoldMult = 1.f;
	float BodyRadius = 0.f;
	FVector ForcedVelocity = FVector::ZeroVector; // 보스 돌진: 직선 이동, 추적·둔화·밀어내기 없음
	float ForcedTimeLeft = 0.f;
	bool bRooted = false;                         // 보스 예고: 제자리에 서서 방향만 유지
	bool bFlees = false;
	bool bActive = false;
};

UCLASS()
class PROJECTA_API UMLEnemySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// 풀 한도에 도달하면 false 반환. HealthMult: 구역 배율, SpeedMult: 탐욕 (design §6.2.2), GoldMult: 구역 금화 가치.
	bool SpawnEnemy(TSubclassOf<AMLEnemyActor> EnemyClass, const FVector& Location, float HealthMult = 1.f, float SpeedMult = 1.f, float GoldMult = 1.f,
		EMLEliteTrait EliteTrait = EMLEliteTrait::None, int32* OutIndex = nullptr);
	int32 GetActiveCount() const { return ActiveCount; }
	
	// MaxRange(2D) 안에 활성 적이 없으면 INDEX_NONE. ExcludeIndices: 제외할 인덱스 (연쇄 대상).
	int32 FindNearestEnemy(const FVector& Origin, float MaxRange, const TArray<int32>* ExcludeIndices = nullptr) const;
	FVector GetEnemyLocation(int32 Index) const;
	// 황금 슬라임 조회: 도망치는 첫 활성 적, 없으면 INDEX_NONE.
	int32 FindFleeingEnemy() const;
	// 정예 방 화살표: 가장 가까운 활성 엘리트, 없으면 INDEX_NONE.
	int32 FindNearestElite(const FVector& Origin) const;

	// Source: 피해 리포트 키 (기술 클래스 이름 / "Basic"), design §6 HUD (M9-0)
	void DamageEnemy(int32 Index, float Damage, FName Source);
	// 맞힌 적 수를 반환.
	int32 DamageInRadius(const FVector& Center, float Radius, float Damage, FName Source);
	void DamageInArc(const FVector& Origin, const FVector& Forward, float Range, float HalfAngleDeg, float Damage, FName Source);

	// 범위(2D, 가장자리 기준) 안의 활성 적 인덱스를 OutIndices에 채움 (M9-1b 낙뢰 대상 고르기)
	void GetEnemiesInRange(const FVector& Origin, float Range, TArray<int32>& OutIndices) const;

	// 범위 안 적을 한 번에 둔화. SpeedMultiplier 0 = 결빙. 둔화한 수를 반환 (M9-1b 서리 폭풍)
	int32 SlowInRadius(const FVector& Center, float Radius, float SpeedMultiplier, float Duration);

	// 소스별 표시 이름; 기술을 팔거나 진화해도 유지
	void RegisterDamageSource(FName Source, const FText& Name);
	FText GetDamageSourceName(FName Source) const;
	// 마지막 호출 이후 준 피해 (오버킬 제외); 집계를 초기화
	TMap<FName, float> TakeRoomDamage();
	// SpeedMultiplier 0.6 = 40% 느려짐. 가장 강한 둔화, 더 긴 지속시간이 우선.
	void SlowEnemy(int32 Index, float SpeedMultiplier, float Duration);
	// 방 종료: 활성 적을 모두 숨김, 드롭 없음. 정리한 수를 반환.
	int32 ClearAll();
	
	// 보스 제어 (design §6.5)
	bool IsEnemyActive(int32 Index) const { return Enemies.IsValidIndex(Index) && Enemies[Index].bActive; }
	AMLEnemyActor* GetEnemyActor(int32 Index) const { return Enemies.IsValidIndex(Index) ? Enemies[Index].Actor.Get() : nullptr; }
	float GetEnemyHealthPercent(int32 Index) const;
	void SetEnemyRooted(int32 Index, bool bRooted);
	void LaunchEnemy(int32 Index, const FVector& Velocity, float Duration);
	void SetEnemyMoveSpeed(int32 Index, float Speed);
	void DamageHero(float Damage);

	FMLOnEnemyKilled OnEnemyKilled;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void ApplyContactDamage(AMLHeroCharacter* Hero, float Damage) const;
	
	void KillEnemy(FMLEnemyEntry& Entry);

	UPROPERTY()
	TArray<FMLEnemyEntry> Enemies;

	TWeakObjectPtr<AMLHeroCharacter> CachedHero;
	TMap<FName, float> RoomDamage;
	TMap<FName, FText> DamageSourceNames;
	int32 ActiveCount = 0;
	float HeroContactCooldown = 0.f; // 주기마다 접촉 타격 1회, 모든 적이 공유

	static constexpr int32 MaxEnemies = 200;
	static constexpr float ContactRadius = 80.f;
	static constexpr float SeparationRadius = 70.f;
	static constexpr float SeparationStrength = 10.f;
	static constexpr float ContactInterval = 0.5f;
	static constexpr float GoldDropMinOffset = 50.f; // 금화는 경험치 보석 주위 링에 떨어짐, 보석 위에는 안 떨어짐
	static constexpr float GoldScatterRadius = 120.f;
	// design §6.5 엘리트
	static constexpr float EliteHealthMult = 6.f;
	static constexpr float EliteContactMult = 2.f;
	static constexpr float SwiftSpeedMult = 1.5f;
	static constexpr float ShieldHealthPercent = 0.5f;
	static constexpr float RegenPercentPerSecond = 0.03f;
	static constexpr int32 EliteCoinCount = 5;
	static constexpr float EliteXPMult = 5.f;
};
