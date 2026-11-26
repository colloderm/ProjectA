// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "MLAutoSkill.generated.h"

class UAbilitySystemComponent;
class UMLEnemySubsystem;
class UTexture2D;
class UMaterialInterface;
class AMLSkillVisualActor;
class AMLHeroCharacter;
struct FMLProjectileLaunch;

UCLASS(Abstract, Blueprintable)
class PROJECTA_API UMLAutoSkill : public UObject
{
	GENERATED_BODY()

public:
	void Acquire(AActor* InHero);
	// design §6.2.3 강적: 망령(InOwner)이 이 기술을 영웅에게 씀 (Lv1 고정). 대상 헬퍼가 영웅 하나를 가리킴
	void AcquireHostile(AActor* InOwner, TSubclassOf<AMLSkillVisualActor> InTelegraphClass, UMaterialInterface* InOverlay, float InTelegraphZOffset);
	bool IsHostile() const { return bHostile; }
	void LevelUp();
	
	// 오브젝트를 버리기 전에 타이머를 멈추고 시각 효과를 제거 (진화, M6-3b의 상점 제거).
	virtual void Release();

	bool IsMaxLevel() const { return SkillLevel >= MaxSkillLevel; }
	TSubclassOf<UMLAutoSkill> GetEvolvedClass() const { return EvolvedClass; }
	FName GetEvolvePassiveRow() const { return EvolvePassive.RowName; }
	int32 GetSkillTier() const { return SkillTier; }
	const TArray<TSubclassOf<UMLAutoSkill>>& GetRecipeInputs() const { return RecipeInputs; }

	int32 GetSkillLevel() const { return SkillLevel; }
	int32 GetMaxSkillLevel() const { return MaxSkillLevel; }
	const FText& GetSkillName() const { return SkillName; }
	const FText& GetSkillDescription() const { return SkillDescription; }
	UTexture2D* GetSkillIcon() const { return SkillIcon; }

	virtual UWorld* GetWorld() const override;

protected:
	virtual void OnAcquired() {}
	virtual void OnLevelChanged() {}
	virtual void Fire() {}

	AActor* GetHero() const { return HeroActor; }
	UAbilitySystemComponent* GetHeroASC() const;
	UMLEnemySubsystem* GetEnemySubsystem() const;
	bool IsHeroDead() const;
	float RollDamage(float DamageMult, bool bForceCrit = false) const;
	float GetEffectiveCooldown() const; // 0 = 발동 안 함
	
	// 피해 리포트 키: 진화 기술은 별도 클래스라서 별도 줄로 표시 (design §6 HUD, M9-0)
	FName GetDamageSource() const { return GetClass()->GetFName(); }

	// --- 대상 헬퍼: 영웅의 기술 = 적 서브시스템 그대로, 망령의 기술 = 영웅 하나 (HeroTarget) ---
	static constexpr int32 HeroTarget = -2;
	bool HasTargets() const;
	int32 FindTarget(const FVector& Origin, float Range) const;
	FVector GetTargetLocation(int32 Target) const;
	int32 DamageArea(const FVector& Center, float Radius, float Damage); // 맞힌 수
	bool LaunchShot(FMLProjectileLaunch Shot) const;                     // 망령이면 적대 탄 + 유도 1/4
	AMLSkillVisualActor* SpawnVisual(TSubclassOf<AMLSkillVisualActor> VisualClass, const FVector& Location) const; // 망령이면 오버레이

	// 망령 전용 (design §6.2.3): Center에 예고 원을 HostileStrikeDelay초 보여 준 뒤 Radius 안이면 피해 (+ 둔화)
	void HostileStrike(const FVector& Center, float Radius, float Damage, float SlowMultiplier = 1.f, float SlowSeconds = 0.f);
	AMLHeroCharacter* GetTargetHero() const; // 살아 있는 영웅, 없으면 nullptr

	static constexpr float HostileStrikeDelay = 0.6f;
	static constexpr float HostileStrikeRadius = 150.f; // 번개 사슬처럼 원래 범위가 없는 즉시 명중
	static constexpr float HostileCooldownMult = 1.5f;
	static constexpr float HostileTurnRateMult = 0.25f;

	// PerLevel[Level-1], 마지막 항목으로 제한
	template <typename T>
	T LevelValue(const TArray<T>& PerLevel) const
	{
		return PerLevel.Num() > 0 ? PerLevel[FMath::Clamp(SkillLevel - 1, 0, PerLevel.Num() - 1)] : T();
	}

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	FText SkillName;

	UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (MultiLine = true))
	FText SkillDescription;
	
	// 선택; 비면 HUD 슬롯에 색 사각형 + 첫 글자
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TObjectPtr<UTexture2D> SkillIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (ClampMin = "1"))
	int32 MaxSkillLevel = 3;

	// 기술 단계 (design §6.4.1, M9-1): 1 = 기본, 2 = 조합, 3 = 진화. 판매가와 진화 조건에 사용
	UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (ClampMin = "1", ClampMax = "3"))
	int32 SkillTier = 1;

	// 레벨별 초. 비면 주기적 Fire가 없는 상시형 기술.
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	TArray<float> CooldownByLevel;
	
	// 2단계 전용: 두 재료 기술을 모두 최대 레벨로 보유하면 레벨업 카드로 이 기술에 조합 (design §6.4.1, M9-1)
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Combo")
	TArray<TSubclassOf<UMLAutoSkill>> RecipeInputs;

	// design §6.4.1: 2단계 최대 레벨 + 이 패시브 보유 -> 휴식터에서 EvolvedClass(3단계)로 진화 가능 (같은 슬롯)
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Evolution")
	TSubclassOf<UMLAutoSkill> EvolvedClass;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Evolution", meta = (RowType = "/Script/ProjectA.MLPassiveRow"))
	FDataTableRowHandle EvolvePassive;

private:
	void ScheduleFire(float Delay);
	void HandleFire();

	UPROPERTY(Transient)
	TObjectPtr<AActor> HeroActor;

	UPROPERTY(Transient)
	TSubclassOf<AMLSkillVisualActor> HostileTelegraphClass;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> HostileOverlay;

	float HostileTelegraphZOffset = 0.f;
	bool bHostile = false;

	FTimerHandle FireTimerHandle;
	int32 SkillLevel = 0;

	static constexpr float FirstFireDelay = 0.2f;
	
	static constexpr float IdleRetryDelay = 0.2f; // 적이 없을 때: 이 주기로 다시 확인
};
