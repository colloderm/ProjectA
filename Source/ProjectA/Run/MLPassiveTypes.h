// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MLPassiveTypes.generated.h"

class UMLAutoSkill;
class UTexture2D;

// 순서는 MLGE_Passive.cpp의 switch와 같아야 함.
UENUM(BlueprintType)
enum class EMLPassiveStat : uint8
{
	Attack,          // 퍼센트 (0.1 = +10%)
	AttackSpeed,     // 고정값
	MaxHealth,       // 퍼센트
	Defense,         // 고정값
	CritChance,      // 고정값
	SkillCooldown,   // 고정값 (0.06 = -6%)
	PickupRadius,    // 퍼센트
	XPGain,          // 고정값
	HealthRegen,     // 고정값 (0.4 = 초당 최대 HP의 +0.4%)
	Count UMETA(Hidden)
};

// DT_Passives 행
USTRUCT(BlueprintType)
struct FMLPassiveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	EMLPassiveStat Stat = EMLPassiveStat::Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	float ValuePerLevel = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (ClampMin = "1"))
	int32 MaxLevel = 5;
	
	// 선택; 비면 HUD 슬롯에 색 사각형 + 첫 글자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	TObjectPtr<UTexture2D> Icon;
};

// design §6.2.1 휴식터 모닥불. None = 일반 레벨업 선택지.
enum class EMLRestAction : uint8
{
	None,
	Heal,    // HP 50%
	Evolve,  // SkillClass -> 그 EvolvedClass, 같은 슬롯
	Upgrade, // SkillClass +1 레벨
	TakeItem, // 묘비 (design §6.2.3): ItemId를 미확보 장비로
};

// 레벨업 선택지 하나: SkillClass가 있으면 자동 기술, 아니면 PassiveRow가 있으면 패시브, 아니면 회복 대체.
struct FMLLevelUpChoice
{
	FName PassiveRow;
	int32 NextLevel = 0;
	TSubclassOf<UMLAutoSkill> SkillClass;
	EMLRestAction RestAction = EMLRestAction::None;
	bool bEnabled = true; // 휴식터: 조건이 안 되면 회색으로 표시
	bool bCombo = false;  // 조합 카드: SkillClass = 2단계 기술, 재료 두 개를 소모 (M9-1)
	FName ItemId;         // TakeItem: DT_Items 행
};

// design §6.2.1 상점 행
enum class EMLShopItemKind : uint8
{
	Offer,       // Target 구매 (NextLevel의 기술 또는 패시브)
	Potion,      // 30% 회복
	SellSkill,   // Target.SkillClass = 보유 기술 (NextLevel = 현재 레벨, 표시용)
	SellPassive, // Target.PassiveRow = 보유 패시브 (NextLevel = 현재 레벨, 표시용)
};

struct FMLShopItem
{
	EMLShopItemKind Kind = EMLShopItemKind::Offer;
	FMLLevelUpChoice Target;
	int32 Price = 0;
	bool bSold = false;
};
