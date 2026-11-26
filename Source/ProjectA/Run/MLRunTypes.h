// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Guild/MLGuildTypes.h"
#include "MLRunTypes.generated.h"

class AMLEnemyActor;

// design §6.2.1. M6-1은 모든 종류를 Normal로 진행.
UENUM(BlueprintType)
enum class EMLRoomType : uint8
{
	Normal,
	Elite,
	Gold,
	Rest,
	Shop,
	Boss
};

UENUM()
enum class EMLRunState : uint8
{
	Combat,     // 방 진행 중
	Collecting, // 방 클리어, 보석이 영웅에게 날아오는 중; 마지막 보석이 도착하면 포털이 열림
	Choosing,   // 포털 (+ 탈출 문) 열림
	Transition, // 방 사이 페이드
	Finished    // 탈출 또는 사망
};

USTRUCT(BlueprintType)
struct FMLZoneSpawnConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<AMLEnemyActor> EnemyClass;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.05"))
	float SpawnInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "1"))
	int32 SpawnPerTick = 2;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "1", ClampMax = "200"))
	int32 MaxAlive = 120;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.1"))
	float HealthMult = 1.f;

	// 이 구역의 적이 영웅에게 주는 모든 피해 배율 (1.0 / 1.5 / 2.2), 용병 등급 보정과 곱함
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.1"))
	float DamageMult = 1.f;

	// design §6.2: 이만큼 스폰됐고 살아 있는 적이 없으면 방 종료
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "1"))
	int32 EnemiesPerRoom = 80;
	
	// design §6.3: 이 구역의 금화 가치 배율 (1.0 / 1.6 / 2.5)
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0"))
	float GoldMult = 1.f;
	
	// design §6.2: 일반 방에 섞이는 엘리트 (k번째는 처치 할당량의 EliteKillStep × k에서)
	UPROPERTY(EditAnywhere, Category = "Spawn|Elite", meta = (ClampMin = "0", ClampMax = "3"))
	int32 ElitesPerRoom = 0;

	// design §6.2.1 정예 방: 처치해야 할 엘리트
	UPROPERTY(EditAnywhere, Category = "Spawn|Elite", meta = (ClampMin = "1"))
	int32 EliteRoomElites = 3;
};

// design §6 HUD 피해 리포트 (M9-0): 소스 하나, 오버킬 제외
struct FMLDamageLine
{
	FText Name;
	float Damage = 0.f;
};

// 결과 화면 데이터
struct FMLRunSummary
{
	bool bExtracted = false;
	int32 Zone = 1;
	int32 Room = 1;
	float TimeSeconds = 0.f;
	int32 Kills = 0;
	int32 RunLevel = 1;
	int32 Gold = 0; // 미확보 원정 골드: 탈출하면 유지, 사망하면 잃음
	bool bBossKilled = false; // design §6.2: 보스 처치 = 강제 탈출
	FMLRunReward Reward; // design §11: 길드 결과 (용병 경험치 / 전사)
	TArray<FMLDamageLine> DamageLines; // 소스별 원정 합계, 큰 순서
	float CombatSeconds = 0.f;         // 전투 방만, 일시정지 제외
};

namespace MLRun
{
	inline FText GetRoomTypeName(EMLRoomType Type)
	{
		switch (Type)
		{
		case EMLRoomType::Elite: return NSLOCTEXT("ML", "RoomElite",  "정예");
		case EMLRoomType::Gold:  return NSLOCTEXT("ML", "RoomGold",   "황금");
		case EMLRoomType::Rest:  return NSLOCTEXT("ML", "RoomRest",   "휴식터");
		case EMLRoomType::Shop:  return NSLOCTEXT("ML", "RoomShop",   "상점");
		case EMLRoomType::Boss:  return NSLOCTEXT("ML", "RoomBoss",   "보스");
		default:                 return NSLOCTEXT("ML", "RoomNormal", "일반");
		}
	}
}
