// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Guild/MLGuildTypes.h"
#include "MLSaveGame.generated.h"

// design §11: 저장 슬롯 하나 "Guild"
UCLASS()
class PROJECTA_API UMLSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Gold = 0;
	UPROPERTY() int32 Renown = 0;						// design §8.3: 누적 은퇴 포인트, 소비되지 않음
	UPROPERTY() int32 FamilyPoints = 0;					// design §8.3: 소비 가능, 은퇴 시 명성과 함께 획득
	UPROPERTY() TArray<FName> FamilyTalents;			// 유산 재능 (DT_Talents 행), 최대 UMLGuildSettings::MaxFamilyTalents
	UPROPERTY() TMap<FName, int32> FamilyUpgrades;		// DT_FamilyUpgrades 행 → 레벨
	UPROPERTY() int32 RerollCount = 0;					// design §8.1: 원정마다 초기화
	UPROPERTY() TArray<FName> Stash;					// design §8.4: DT_Items 행, 최대 UMLGuildSettings::StashCapacity
	UPROPERTY() TArray<FName> ShopOffers;				// design §8.4: 길드 상점 진열 (DT_Items 행), 원정마다 갱신
	UPROPERTY() TArray<FName> RequestOffers;			// design §8.5.1: 의뢰판의 DT_Requests 행, 원정마다 갱신
	UPROPERTY() FName ActiveRequest;					// 출정 시 받음; EndRun에서 판정 후 비움 (또는 비정상 종료 복구 시)
	UPROPERTY() bool bHasMerc = false;
	UPROPERTY() FMLMercData Merc;
	UPROPERTY() TArray<FMLMercData> Candidates;
	UPROPERTY() TArray<FMLHallEntry> HallOfFame;
	UPROPERTY() bool bRunInProgress = false;			// design §6.6 비정상 종료 복구 플래그
	UPROPERTY() int32 NextMercId = 1;
};
