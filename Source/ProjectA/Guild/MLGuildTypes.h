// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Combat/MLWeaponTypes.h"
#include "MLGuildTypes.generated.h"

// design §7.1 용병 등급 (DT_Grades 행 이름 = 이 문자)
UENUM(BlueprintType)
enum class EMLGrade : uint8
{
	F, E, D, C, B, A, S,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMLTalentRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Legendary
};

UENUM(BlueprintType)
enum class EMLLeaveReason : uint8
{
	Retired,
	Sold,
	Fallen
};

// design §8.7 칭호 (M9-2): 해금 조건이 되는 길드 누적 기록
UENUM(BlueprintType)
enum class EMLTitleStat : uint8
{
	Kills,
	EliteKills,
	BossKills,
	Extractions,
	Deaths,
	GoldEarned
};

// 칭호 색 등급 (흰 / 파랑 / 보라 / 금)
UENUM(BlueprintType)
enum class EMLTitleRarity : uint8
{
	Common,
	Rare,
	Epic,
	Legend
};

// design §7.4 장비 슬롯
UENUM(BlueprintType)
enum class EMLItemSlot : uint8
{
	Weapon,
	Armor,
	Trinket,
	Count UMETA(Hidden)
};

// design §7.2 용병 스탯 (웹 db.js STATS 순서)
USTRUCT(BlueprintType)
struct FMLStatBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float HP = 0.f;
	UPROPERTY(EditAnywhere) float ATK = 0.f;
	UPROPERTY(EditAnywhere) float DEF = 0.f;
	UPROPERTY(EditAnywhere) float SPD = 0.f;
	UPROPERTY(EditAnywhere) float CRIT = 0.f;
	UPROPERTY(EditAnywhere) float DODGE = 0.f;
	UPROPERTY(EditAnywhere) float SPECIAL = 0.f;

	static constexpr int32 Num = 7;
	static bool IsPercentStat(int32 Index) { return Index >= 4; } // CRIT, DODGE, SPECIAL은 더 느리게 성장 (웹 leveledStats)

	float& operator[](int32 Index)
	{
		switch (Index)
		{
		case 0: return HP;
		case 1: return ATK;
		case 2: return DEF;
		case 3: return SPD;
		case 4: return CRIT;
		case 5: return DODGE;
		default: return SPECIAL;
		}
	}
	float operator[](int32 Index) const { return const_cast<FMLStatBlock&>(*this)[Index]; }
};

// DT_Grades 행 (웹 db.js GRADES + design §6.5 / §7.1.1 / §8.3)
USTRUCT(BlueprintType)
struct FMLGradeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float Growth = 1.f;
	UPROPERTY(EditAnywhere) int32 TalentSlots = 1;
	UPROPERTY(EditAnywhere) int32 MaxLevel = 20;
	UPROPERTY(EditAnywhere) int32 ContractGold = 0;
	UPROPERTY(EditAnywhere) int32 RetirePoints = 1;
	UPROPERTY(EditAnywhere) int32 UnlockRenown = 0;   // design §8.3: 이 명성부터 계약소에 이 등급이 나옴
	UPROPERTY(EditAnywhere) FMLStatBlock BaseMin;     // Lv1 굴림 범위 (포함)
	UPROPERTY(EditAnywhere) FMLStatBlock BaseMax;
	UPROPERTY(EditAnywhere) float TargetRuns = 4.f;   // design §7.1.1: Lv1부터 최대 레벨까지 성공 원정 수
	UPROPERTY(EditAnywhere) float EnemyScale = 1.f;   // design §6.5: 이 등급의 적 HP 배율
	// design §7.4 드롭 티어 가중치 (%): X/Y/Z/W = T1..T4, 드롭 단계별 (구역 1/2/3, 보스)
	UPROPERTY(EditAnywhere) FVector4 DropZone1 = FVector4(100, 0, 0, 0);
	UPROPERTY(EditAnywhere) FVector4 DropZone2 = FVector4(100, 0, 0, 0);
	UPROPERTY(EditAnywhere) FVector4 DropZone3 = FVector4(100, 0, 0, 0);
	UPROPERTY(EditAnywhere) FVector4 DropBoss = FVector4(100, 0, 0, 0);
};

// DT_Talents 행 (웹 db.js TALENTS)
USTRUCT(BlueprintType)
struct FMLTalentRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FText Name;
	UPROPERTY(EditAnywhere) EMLTalentRarity Rarity = EMLTalentRarity::Common;
	UPROPERTY(EditAnywhere) float Weight = 100.f;
	UPROPERTY(EditAnywhere) FMLStatBlock BonusPercent; // +18 = ×1.18, -8 = ×0.92
	UPROPERTY(EditAnywhere) FMLStatBlock Flat;
	UPROPERTY(EditAnywhere) FText Description;
};

// DT_Titles 행 (design §8.7, M9-2). 해금 = 길드 누적 기록[Stat] >= Target
USTRUCT(BlueprintType)
struct FMLTitleRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FText Name;
	UPROPERTY(EditAnywhere) EMLTitleStat Stat = EMLTitleStat::Kills;
	UPROPERTY(EditAnywhere, meta = (ClampMin = "1")) int32 Target = 1;
	UPROPERTY(EditAnywhere) EMLTitleRarity Rarity = EMLTitleRarity::Common;
	UPROPERTY(EditAnywhere) FMLStatBlock BonusPercent; // 재능과 같음: +3 = ×1.03
	UPROPERTY(EditAnywhere) FMLStatBlock Flat;
	UPROPERTY(EditAnywhere) float GoldPercent = 0.f;   // 탈출 골드
	UPROPERTY(EditAnywhere) float XPPercent = 0.f;     // 용병 경험치
};

// design §8.7 길드 누적 기록 (계정 전체, 칭호 해금 기준)
USTRUCT()
struct FMLGuildStats
{
	GENERATED_BODY()

	UPROPERTY() int32 Kills = 0;
	UPROPERTY() int32 EliteKills = 0;
	UPROPERTY() int32 BossKills = 0;
	UPROPERTY() int32 Extractions = 0;
	UPROPERTY() int32 Deaths = 0;		// 용병 전사
	UPROPERTY() int32 GoldEarned = 0;	// 가문 보너스 적용 후 탈출 골드

	int32 Get(EMLTitleStat Stat) const
	{
		switch (Stat)
		{
		case EMLTitleStat::Kills:       return Kills;
		case EMLTitleStat::EliteKills:  return EliteKills;
		case EMLTitleStat::BossKills:   return BossKills;
		case EMLTitleStat::Extractions: return Extractions;
		case EMLTitleStat::Deaths:      return Deaths;
		default:                        return GoldEarned;
		}
	}
};

// DT_FamilyUpgrades 행 (웹 db.js ACCOUNT_UPGRADES, design §8.3)
USTRUCT(BlueprintType)
struct FMLFamilyUpgradeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FText Name;
	UPROPERTY(EditAnywhere) FMLStatBlock StatPercentPerLevel; // ATK=3 → 레벨당 ATK +3%
	UPROPERTY(EditAnywhere) float GoldPercentPerLevel = 0.f;  // 탈출 골드
	UPROPERTY(EditAnywhere) float XPPercentPerLevel = 0.f;    // 용병 경험치
	UPROPERTY(EditAnywhere) int32 MaxLevel = 10;
	UPROPERTY(EditAnywhere) float Cost = 2.f;                 // 가문 포인트: round(Cost × CostGrowth^Lv)
	UPROPERTY(EditAnywhere) float CostGrowth = 1.35f;
};

// DT_Trainings 행 (웹 db.js TRAININGS, design §7.5)
USTRUCT(BlueprintType)
struct FMLTrainingRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FText Name;
	UPROPERTY(EditAnywhere) FMLStatBlock Amount;		// 구매당 고정 보너스
	UPROPERTY(EditAnywhere) float Cost = 200.f;			// 골드: round(Cost × CostGrowth^이 용병의 구매 횟수)
	UPROPERTY(EditAnywhere) float CostGrowth = 1.2f;
};

// DT_Items 행 (웹 db.js ITEMS, design §7.4)
USTRUCT(BlueprintType)
struct FMLItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FText Name;
	UPROPERTY(EditAnywhere) EMLItemSlot Slot = EMLItemSlot::Weapon;
	UPROPERTY(EditAnywhere, meta = (ClampMin = "1", ClampMax = "4")) int32 Tier = 1;
	UPROPERTY(EditAnywhere) EMLGrade RequiredGrade = EMLGrade::F;				// T1 F / T2 D / T3 B / T4 A
	UPROPERTY(EditAnywhere) FMLStatBlock Flat;
	UPROPERTY(EditAnywhere) int32 Price = 400;									// 판매가 = Price × UMLGuildSettings::ItemSellRate
	UPROPERTY(EditAnywhere) EMLAttackType WeaponType = EMLAttackType::Sword;	// Slot == Weapon일 때만
	UPROPERTY(EditAnywhere) FText Description;
};

// design §8.5.1 길드 의뢰 종류 (원정 종료 시 판정, 탈출 시에만 지급)
UENUM(BlueprintType)
enum class EMLRequestType : uint8
{
	ReachZone,   // 도달 구역 >= Target
	Kills,       // 처치한 적 >= Target
	EliteKills,  // 처치한 엘리트 >= Target
	BossKill,    // 보스 처치 (Target 1)
	GoldRooms,   // 처치한 황금 슬라임 >= Target
	EliteRooms,  // 승리한 정예 방 (엘리트 전멸, 시간 초과는 제외) >= Target
	Greed        // 종료 시 탐욕 단계 >= Target
};

// DT_Requests 행 (design §8.5.1)
USTRUCT(BlueprintType)
struct FMLRequestRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FText Name;
	UPROPERTY(EditAnywhere) EMLRequestType Type = EMLRequestType::ReachZone;
	UPROPERTY(EditAnywhere, meta = (ClampMin = "1")) int32 Target = 1;
	UPROPERTY(EditAnywhere) int32 RewardGold = 0;
	UPROPERTY(EditAnywhere) int32 RewardXP = 0;
	UPROPERTY(EditAnywhere) int32 RewardPoints = 0;	// 가문 포인트 (드묾)
	UPROPERTY(EditAnywhere) float Weight = 100.f;	// 진열 추첨 가중치
};

USTRUCT()
struct FMLMercData
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = 0;
	UPROPERTY() FString Name;
	UPROPERTY() EMLGrade Grade = EMLGrade::F;
	UPROPERTY() int32 Level = 1;
	UPROPERTY() int32 XP = 0;
	UPROPERTY() FMLStatBlock BaseStats;								// Lv1 굴림
	UPROPERTY() TArray<FName> Talents;								// DT_Talents 행 이름
	UPROPERTY() FMLStatBlock Training;								// design §7.5: 용병별 고정 보너스, 용병과 함께 사라짐
	UPROPERTY() TMap<FName, int32> TrainingBuys;					// DT_Trainings 행 → 구매 횟수
	UPROPERTY() EMLAttackType WeaponType = EMLAttackType::Dagger;	// design §7.4 주무기 (M7-3 이전 저장 = Dagger)
	UPROPERTY() TMap<EMLItemSlot, FName> Equipped;					// 슬롯별 DT_Items 행
	UPROPERTY() FName Title;										// design §8.7: 장착 칭호 (DT_Titles 행), 없으면 None

	// design §8.2 / §8.5 기록
	UPROPERTY() int32 Extractions = 0;
	UPROPERTY() int32 BossKills = 0;
	UPROPERTY() int32 BestZone = 0;
	UPROPERTY() int32 GoldEarned = 0;
};

// design §8.5 명예의 전당
USTRUCT()
struct FMLHallEntry
{
	GENERATED_BODY()

	UPROPERTY() FMLMercData Merc;
	UPROPERTY() EMLLeaveReason Reason = EMLLeaveReason::Fallen;
	UPROPERTY() FDateTime Date;
};

// design §6.2.3 전사자의 묘비 (M9-3). 원정이 끝날 때(EndRun)만 바뀜
USTRUCT()
struct FMLGrave
{
	GENERATED_BODY()

	UPROPERTY() int32 MercId = 0;									// 전사한 용병 (명예의 전당 기록의 Id)
	UPROPERTY() FString Name;
	UPROPERTY() EMLGrade Grade = EMLGrade::F;
	UPROPERTY() EMLAttackType WeaponType = EMLAttackType::Sword;	// 죽을 때의 공격 방식 (장착 무기, 없으면 주무기)
	UPROPERTY() TArray<FName> Items;								// 장착했던 DT_Items 행 (최대 3)
	UPROPERTY() int32 Zone = 1;										// 죽은 구역
	UPROPERTY() int32 RunsLeft = 0;									// 남은 원정 수
};

// 원정 → 길드, 원정 종료 시 한 번 (design §11)
struct FMLRunReport
{
	bool bExtracted = false;
	bool bBossKilled = false;
	int32 Zone = 1;
	int32 Kills = 0;
	int32 EliteKills = 0;
	int32 Gold = 0;
	float RewardMult = 1.f;			// 도달 구역의 보상 배율 (1.0 / 1.6 / 2.5)
	TArray<FName> Items;			// 이번 원정에서 주운 미확보 DT_Items 행
	int32 GoldRoomsCleared = 0;		// design §8.5.1 의뢰
	int32 EliteRoomsCleared = 0;
	int32 Greed = 0;				// 원정 종료 시점의 탐욕 단계
	int32 ClearedGraveMercId = INDEX_NONE; // design §6.2.3: 장비를 되찾은 묘비, 없으면 INDEX_NONE
};

// 길드 → 결과 화면
struct FMLRunReward
{
	bool bHasMerc = false;
	bool bFallen = false;
	FString MercName;
	int32 GoldAdded = 0;		// 가문 보너스 적용 후 탈출 골드
	int32 MercXP = 0;
	int32 OldLevel = 1;
	int32 NewLevel = 1;
	int32 HallNumber = 0;		// 전사 시 1부터 시작하는 명예의 전당 항목
	TArray<FText> ItemNames;	// 탈출: 창고로 보낸 아이템
	int32 AutoSoldGold = 0;		// 창고 초과분 즉시 판매 (design §8.4)
	int32 ItemsLost = 0;		// 사망: 원정 아이템 + 장착 아이템
	int32 GraveZone = 0;		// 사망: 묘비가 남은 구역, 0 = 남지 않음 (design §6.2.3)
	FText RequestName;			// 비어 있음 = 의뢰를 받지 않음 (design §8.5.1)
	int32 RequestProgress = 0;
	int32 RequestTarget = 0;
	bool bRequestDone = false;
	int32 RequestGold = 0;
	int32 RequestXP = 0;
	int32 RequestPoints = 0;
	TArray<FText> NewTitles;	// design §8.7: 이번 원정으로 새로 해금된 칭호
};
