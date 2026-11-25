// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Guild/MLGuildTypes.h"
#include "MLGuildSubsystem.generated.h"

class UDataTable;
class UMLSaveGame;

// 모든 Save() 후에 발생: 길드 UI 갱신 (design §9)
DECLARE_MULTICAST_DELEGATE(FMLOnGuildChanged);

// design §11 길드 계층: 저장을 소유. 원정은 시작할 때 스탯을 한 번 읽고 끝날 때 한 번 보고.
UCLASS()
class PROJECTA_API UMLGuildSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// design §8.1: Index = 해당 등급의 n번째 후보. 용병 보유 중, 잘못된 인덱스, 골드 부족이면 실패.
	bool Contract(EMLGrade Grade, int32 Index);

	// AMLRunGameMode::InitGame: 원정 중 플래그 설정 (design §6.6) 후 저장.
	void BeginRun();
	// 탈출은 골드 + 용병 경험치 지급, 사망은 용병을 명예의 전당으로 옮김. 항상 저장.
	FMLRunReward EndRun(const FMLRunReport& Report);

	// 용병이 없으면 false.
	bool GetActiveStats(FMLStatBlock& OutStats) const;
	// design §6.5: 보유 용병 등급의 적 HP 배율 (용병이 없으면 1)
	float GetEnemyScale() const;

	// design §7.2 순서: 레벨 → 재능 → 훈련 (M7-2) → 장비 (M7-3) → 가문 (M7-2) → 반올림/상한
	FMLStatBlock ComputeStats(const FMLMercData& Merc) const;
	int32 GetXPToNextLevel(const FMLMercData& Merc) const;
	
	// design §8.2 / §8.3: 유산 재능 = 용병 재능의 TalentIndex; 가문 목록이 가득 차면 ReplaceSlot 필요
	bool Retire(int32 TalentIndex, int32 ReplaceSlot = INDEX_NONE);
	bool Sell();
	bool Train(FName TrainingId);              // design §7.5
	bool BuyShopItem(int32 OfferIndex);        // design §8.4: 창고가 가득 차면 거절 (구매품은 절대 자동 판매하지 않음)
	bool AcceptRequest(int32 OfferIndex);      // design §8.5.1: INDEX_NONE = 의뢰 없이 출정
	// design §6.6 (c): Initialize가 비정상 종료된 원정을 되돌린 뒤 한 번만 true (허브가 안내 표시)
	bool ConsumeRollbackNotice();
	bool BuyFamilyUpgrade(FName UpgradeId);    // design §8.3
	bool RerollCandidates();                   // design §8.1
	int32 GetRetirePoints(const FMLMercData& Merc) const;
	int32 GetSellPrice(const FMLMercData& Merc) const;
	
	// design §7.4 / §8.4 장비. 원정 중에는 거절.
	bool Equip(int32 StashIndex);           // 슬롯의 현재 아이템과 교체; RequiredGrade 확인
	bool Unequip(EMLItemSlot Slot);
	bool SellItem(int32 StashIndex);
	// design §6.3: Step 1..3 = 구역, 4 = 보스 테이블. 보유 용병의 등급 사용. ItemTable이 없으면 NAME_None.
	FName RollItem(int32 Step, int32& OutTier) const;
	// design §7.4: 원정에 쓸 DT_Weapons 행 (장착 무기, 없으면 주무기); 용병이 없으면 NAME_None
	FName GetActiveWeaponRow(float& OutDamageScale) const;
	
	// design §9 길드 UI: 읽기 전용 조회. 변경은 여전히 위의 동작들을 거침.
	FMLOnGuildChanged OnGuildChanged;
	const UMLSaveGame* GetSaveData() const;
	const FMLGradeRow* FindGrade(EMLGrade Grade) const;
	const FMLTalentRow* FindTalent(FName TalentId) const;
	const FMLItemRow* FindItem(FName ItemId) const;
	int32 GetRerollCost() const;
	int32 GetItemSellPrice(const FMLItemRow& Item) const; // design §8.4
	const FMLTrainingRow* FindTraining(FName TrainingId) const;
	TArray<FName> GetTrainingIds() const;
	int32 GetTrainingCost(const FMLMercData& Merc, FName TrainingId) const; // design §7.5: Cost × CostGrowth^구매 횟수
	const FMLFamilyUpgradeRow* FindFamilyUpgrade(FName UpgradeId) const;
	TArray<FName> GetFamilyUpgradeIds() const;
	int32 GetFamilyUpgradeCost(FName UpgradeId) const; // design §8.3: Cost × CostGrowth^현재 레벨
	const FMLRequestRow* FindRequest(FName RequestId) const;

	// design §8.7 칭호 (M9-2): 해금은 길드 누적 기록으로 매번 계산 (따로 저장하지 않음)
	const FMLTitleRow* FindTitle(FName TitleId) const;
	TArray<FName> GetTitleIds() const;
	bool IsTitleUnlocked(FName TitleId) const;
	int32 GetTitleProgress(FName TitleId) const; // 조건 기록의 현재 누적 수치
	bool EquipTitle(FName TitleId);              // None = 해제. 원정 중이거나 미해금이면 거절
	bool GetActiveTitle(FText& OutName, EMLTitleRarity& OutRarity) const; // 원정 표시용: 장착 칭호가 없으면 false

	// 개발 콘솔 (M8 길드 UI 전까지)
	void LogStatus() const;
	void ResetSave();
	void AddGold(int32 Amount);
	void AddPoints(int32 Amount); // 가문 포인트 + 명성 합계
	void AddItem(FName ItemId);

private:
	FMLMercData MakeMerc(EMLGrade Grade);
	void RefreshCandidates();
	void RefreshShop();
	void RefreshRequests();
	int32 AddMissingCandidates(); // 해금됐지만 후보가 없는 등급에 새 후보를 채움; 추가한 수를 반환
	bool CanAct(const TCHAR* Action) const; // 원정 중에는 길드 동작이 거절됨
	int32 MoveMercToHall(EMLLeaveReason Reason); // 1부터 시작하는 전당 번호를 반환
	void GetFamilyBonuses(FMLStatBlock& OutStatPercent, float& OutGoldPercent, float& OutXPPercent) const;
	int32 AddToStash(const TArray<FName>& ItemIds); // 넘친 것은 즉시 판매; 그 골드를 반환
	int32 ReturnEquipmentToStash();                 // 은퇴 / 판매; 자동 판매된 골드를 반환
	void GainMercXP(FMLMercData& Merc, int32 Amount) const;
	FString TalentsToString(const TArray<FName>& Talents) const;
	const FMLTitleRow* GetMercTitle(const FMLMercData& Merc) const; // 장착 + 해금된 칭호만, 아니면 nullptr
	void SeedGuildStats();
	void CreateNewSave();
	void Save();

	UPROPERTY()
	TObjectPtr<UMLSaveGame> SaveData;

	UPROPERTY()
	TObjectPtr<UDataTable> GradeTable;

	UPROPERTY()
	TObjectPtr<UDataTable> TalentTable;
	
	UPROPERTY()
	TObjectPtr<UDataTable> FamilyUpgradeTable;

	UPROPERTY()
	TObjectPtr<UDataTable> TrainingTable;
	
	UPROPERTY()
	TObjectPtr<UDataTable> ItemTable;
	
	UPROPERTY()
	TObjectPtr<UDataTable> RequestTable;

	UPROPERTY()
	TObjectPtr<UDataTable> TitleTable;

	bool bRollbackNotice = false;
};
