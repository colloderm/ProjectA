// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MLGuildSettings.generated.h"

class UDataTable;
class UWorld;

// 프로젝트 설정 > Game > Mercenary Legacy
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mercenary Legacy"))
class PROJECTA_API UMLGuildSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// 행 구조체 FMLGradeRow
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> GradeTable;

	// 행 구조체 FMLTalentRow
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> TalentTable;
	
	// 행 구조체 FMLFamilyUpgradeRow
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> FamilyUpgradeTable;

	// 행 구조체 FMLTrainingRow
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> TrainingTable;
	
	// 행 구조체 FMLItemRow
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> ItemTable;
	
	// 행 구조체 FMLRequestRow (선택: 없으면 출정 창에 의뢰가 나오지 않음)
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> RequestTable;

	// design §7.1.1 용병 경험치, 탈출 시에만 지급
	UPROPERTY(Config, EditAnywhere, Category = "Merc XP")
	int32 KillXP = 1;

	UPROPERTY(Config, EditAnywhere, Category = "Merc XP")
	int32 EliteKillXP = 10;

	// 인덱스 = 도달 구역 - 1
	UPROPERTY(Config, EditAnywhere, Category = "Merc XP")
	TArray<int32> ZoneBonusXP = { 50, 150, 350 };

	UPROPERTY(Config, EditAnywhere, Category = "Merc XP")
	int32 BossBonusXP = 800;

	// 성공한 원정의 평균 경험치. 모든 등급의 레벨 곡선이 이 값에 맞춰 늘어남 (Run end 로그로 조정).
	UPROPERTY(Config, EditAnywhere, Category = "Merc XP", meta = (ClampMin = "100"))
	float ExpectedRunXP = 4000.f;

	// design §8.1
	UPROPERTY(Config, EditAnywhere, Category = "Contract", meta = (ClampMin = "1"))
	int32 CandidatesPerGrade = 3;
	
	// design §8.1 후보 다시 뽑기: RerollBaseGold × RerollGrowth^횟수, 횟수는 원정마다 초기화
	UPROPERTY(Config, EditAnywhere, Category = "Contract", meta = (ClampMin = "0"))
	int32 RerollBaseGold = 100;

	UPROPERTY(Config, EditAnywhere, Category = "Contract", meta = (ClampMin = "1"))
	float RerollGrowth = 1.5f;

	// design §8.3
	UPROPERTY(Config, EditAnywhere, Category = "Family", meta = (ClampMin = "1"))
	int32 MaxFamilyTalents = 5;

	UPROPERTY(Config, EditAnywhere, Category = "Family", meta = (ClampMin = "1"))
	float FamilyTalentWeightMult = 3.f;

	// design §8.2: 최대 레벨에서 은퇴 포인트 × 이 값
	UPROPERTY(Config, EditAnywhere, Category = "Family", meta = (ClampMin = "1"))
	float MaxLevelRetireMult = 1.5f;
	
	// design §8.4
	UPROPERTY(Config, EditAnywhere, Category = "Items", meta = (ClampMin = "1"))
	int32 StashCapacity = 40;

	UPROPERTY(Config, EditAnywhere, Category = "Items", meta = (ClampMin = "0", ClampMax = "1"))
	float ItemSellRate = 0.3f;

	// design §7.4: 장착 무기가 용병 주무기와 같으면 기본 공격 피해 × 이 값
	UPROPERTY(Config, EditAnywhere, Category = "Items", meta = (ClampMin = "1"))
	float WeaponMatchBonus = 1.15f;
	
	// design §8.4 길드 상점: 원정마다 진열 갱신 (서로 다른 아이템, 티어 ≤ ShopMaxTier)
	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "1"))
	int32 ShopOfferCount = 4;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "1", ClampMax = "4"))
	int32 ShopMaxTier = 2;
	
	// design §8.5.1 의뢰판: Weight로 서로 다른 의뢰 진열, 원정마다 갱신
	UPROPERTY(Config, EditAnywhere, Category = "Request", meta = (ClampMin = "1"))
	int32 RequestOfferCount = 3;
	
	// design §9 화면 흐름: 길드 허브 ↔ 원정 맵
	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> GuildMap;

	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> RunMap;
};
