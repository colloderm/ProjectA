// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/MLEnemyActor.h"
#include "MLBossActor.generated.h"

class AMLSkillVisualActor;
class UMLEnemySubsystem;

enum class EMLBossPattern : uint8
{
	None,
	Charge,
	Roar,
	Summon
};

// design §6.5 보스. 다른 적처럼 UMLEnemySubsystem에 들어 있음 (모든 기술이 평소처럼 맞힘);
// 이 액터는 타이머로 패턴 루프만 돌림.
UCLASS()
class PROJECTA_API AMLBossActor : public AMLEnemyActor
{
	GENERATED_BODY()

public:
	void StartBoss(int32 InEnemyIndex);
	void StopBoss();
	int32 GetEnemyIndex() const { return EnemyIndex; }
	const FText& GetBossName() const { return BossName; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	FText BossName;

	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0.5"))
	float PatternGap = 3.f;

	// 영웅 액터 Z는 캡슐 중심; 예고 표시는 그보다 이만큼 아래, 바닥에 놓임
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	float TelegraphZOffset = -86.f;

	// ① 돌진: 빨간 선 후 직선 돌진 (길이 = 속도 × 초)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge")
	TSubclassOf<AMLSkillVisualActor> LineTelegraphClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge")
	float ChargeTelegraphSeconds = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge")
	float ChargeSpeed = 1600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge")
	float ChargeSeconds = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge")
	float ChargeTelegraphWidth = 200.f;

	// ② 포효: 링 예고 후 SafeRadius~Radius 사이에 피해 (보스 바로 옆은 안전)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Roar")
	TSubclassOf<AMLSkillVisualActor> RingTelegraphClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Roar")
	float RoarTelegraphSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Roar")
	float RoarSafeRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Roar")
	float RoarRadius = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Roar")
	float RoarDamage = 25.f;

	// ③ 소환: HP 비율이 이 값 이하가 되면 순환에 합류
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon")
	TSubclassOf<AMLEnemyActor> SummonClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (ClampMin = "0", ClampMax = "1"))
	float SummonHealthPercent = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (ClampMin = "1"))
	int32 SummonCount = 8;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon")
	float SummonRadius = 300.f;

	// 약한 광폭화: EnrageStartSeconds부터 EnrageInterval마다 → 이동 속도 +step, 패턴 간격 −step
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	float EnrageStartSeconds = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	float EnrageInterval = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	float EnrageSpeedStep = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	float EnrageGapStep = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage", meta = (ClampMin = "0"))
	int32 EnrageMaxStacks = 5;

private:
	void ScheduleNextPattern();
	void BeginPattern();
	void ExecuteCharge();
	void ExecuteRoar();
	void DoSummon();
	void AddEnrage();
	void ShowTelegraph(TSubclassOf<AMLSkillVisualActor> TelegraphClass, const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	void HideTelegraph();
	UMLEnemySubsystem* GetEnemySubsystem() const;
	bool GetHeroLocation(FVector& OutLocation) const;

	UPROPERTY(Transient)
	TObjectPtr<AMLSkillVisualActor> Telegraph;

	FTimerHandle PatternTimerHandle;
	FTimerHandle EnrageTimerHandle;

	FVector ChargeDir = FVector::ForwardVector;
	EMLBossPattern LastPattern = EMLBossPattern::None;
	int32 EnemyIndex = INDEX_NONE;
	int32 EnrageStacks = 0;
};
