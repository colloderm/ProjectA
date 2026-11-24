// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Run/MLPassiveTypes.h"
#include "ActiveGameplayEffectHandle.h"
#include "MLRunProgressComponent.generated.h"

class APlayerController;
class UDataTable;
class UMLLevelUpWidget;
class UMLRunHUDWidget;
class UMLAutoSkill;
class UMLShopWidget;

DECLARE_MULTICAST_DELEGATE(FMLOnRunProgressChanged);

UCLASS(ClassGroup=(ML), meta=(BlueprintSpawnableComponent))
class PROJECTA_API UMLRunProgressComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMLRunProgressComponent();

	// 기본 어트리뷰트 설정 후 AMLHeroCharacter::PossessedBy에서 호출.
	void InitForPlayer(APlayerController* PC);
	void AddXP(float Amount);
	void AddGold(int32 Amount);
	int32 GetRunGold() const { return RunGold; }
	
	// design §6.3: 지금 아이템 하나를 추첨 (Step 1..4); 탈출 전까지 미확보
	void AddChest(int32 Step);
	const TArray<FName>& GetRunItems() const { return RunItems; }
	int32 GetRunItemTierTotal() const { return RunItemTierTotal; } // 탐욕 값 소스

	int32 GetRunLevel() const { return RunLevel; }
	float GetRunXP() const { return RunXP; }
	float GetXPToNext() const { return 5.f + 8.f * (RunLevel - 1); } // design §6.4

	// 레벨업 위젯 API
	int32 GetChoiceCount() const { return Choices.Num(); }
	FText GetChoiceText(int32 ChoiceIndex) const;
	int32 GetChoiceTier(int32 ChoiceIndex) const; // 0 = 기술 아님 (패시브 / 회복), 1~3 = 기술 단계
	bool CanReroll() const { return !bRerollUsed && !bRestMode; }
	void SelectChoice(int32 ChoiceIndex);
	void Reroll();
	
	// design §6.2.1 휴식터: 회복 / 진화 / 강화를 레벨업 위젯에 표시. 선택 후 OnRestFinished 발생.
	void OpenRestChoices();
	bool IsRestOpen() const { return bRestMode; }
	bool IsChoiceEnabled(int32 ChoiceIndex) const { return Choices.IsValidIndex(ChoiceIndex) && Choices[ChoiceIndex].bEnabled; }
	
	// design §6.2.1 상점. PriceMult = 구역 골드 배율. 플레이어가 나가면 OnShopFinished 발생.
	void OpenShop(float PriceMult);
	void CloseShop();
	int32 GetShopItemCount() const { return ShopItems.Num(); }
	FText GetShopItemText(int32 Index) const;
	bool IsShopItemEnabled(int32 Index) const;
	void TradeShopItem(int32 Index); // 구매 (골드 감소) 또는 판매 (골드 증가)
	bool IsShopSellItem(int32 Index) const;

	// HUD 조회 API
	const TArray<TObjectPtr<UMLAutoSkill>>& GetOwnedSkills() const { return OwnedSkills; }
	const TMap<FName, int32>& GetPassiveLevels() const { return PassiveLevels; } // 상점 제거로 순서가 바뀔 수 있음
	const FMLPassiveRow* FindPassiveRow(FName RowName) const;

	FMLOnRunProgressChanged OnProgressChanged; // 경험치 / 레벨 / 골드
	FMLOnRunProgressChanged OnSlotsChanged;    // 기술이나 패시브 획득 / 레벨업
	
	FMLOnRunProgressChanged OnRestFinished;
	FMLOnRunProgressChanged OnShopFinished;

	static constexpr int32 MaxPassiveSlots = 5;
	static constexpr int32 MaxSkillSlots = 5;

private:
	void OpenLevelUp();
	bool ShowChoiceWidget();
	FText GetRestChoiceText(const FMLLevelUpChoice& Choice) const;
	void ApplyRestChoice(const FMLLevelUpChoice& Picked);
	UMLAutoSkill* FindEvolvableSkill() const;
	void EvolveSkill(UMLAutoSkill* OldSkill);
	void GatherCandidates(TArray<FMLLevelUpChoice>& OutCandidates) const;
	bool CanCombo(TSubclassOf<UMLAutoSkill> ComboClass) const;
	void ApplyCombo(TSubclassOf<UMLAutoSkill> ComboClass);
	void ApplyLevelUpChoice(const FMLLevelUpChoice& Picked);
	void SetMenuMode(bool bMenu);
	void RebuildShopItems();
	bool IsOfferStillValid(const FMLLevelUpChoice& Offer) const;
	FText GetTargetName(const FMLLevelUpChoice& Target) const;
	void RemoveSkill(TSubclassOf<UMLAutoSkill> SkillClass);
	void RemovePassive(FName RowName);
	void CloseLevelUp();
	void RefreshOpenWidget();
	void RollChoices();
	void ApplyPassive(FName RowName);
	void HealPercent(float Percent);
	void ApplySkill(TSubclassOf<UMLAutoSkill> SkillClass);
	UMLAutoSkill* FindOwnedSkill(TSubclassOf<UMLAutoSkill> SkillClass) const;
	APlayerController* GetOwningPC() const;

	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TObjectPtr<UDataTable> PassiveTable;
	
	// 레벨업에 제안되는 BP_Skill_* 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TArray<TSubclassOf<UMLAutoSkill>> AutoSkillPool;

	// 2단계 조합 BP (design §6.4.1, M9-1). 재료는 각 BP의 RecipeInputs
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TArray<TSubclassOf<UMLAutoSkill>> ComboSkillPool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLAutoSkill>> OwnedSkills;

	// 조합 재료로 쓴 1단계 기술: 빈 슬롯을 채울 새 기술 후보에서 제외 (이번 원정 동안)
	UPROPERTY(Transient)
	TSet<TSubclassOf<UMLAutoSkill>> ConsumedSkills;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMLLevelUpWidget> LevelUpWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMLRunHUDWidget> HUDWidgetClass;

	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMLShopWidget> ShopWidgetClass;

	// design §6.2.1 구역 배율 적용 전 상점 가격
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 ShopBasePrice = 40;   // Lv1 진열

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 ShopLevelPrice = 15;  // + 추가 레벨당

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 ShopPotionPrice = 25;

	// design §6.2.1 판매: 받는 골드 = 레벨 x 이 값 x 구역 배율
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 ShopSellPerLevel = 10;

	// 2단계 판매 레벨 = 이 값 + 현재 레벨 (재료 두 개 Lv3 = 6)
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 ShopSellTier2Base = 6;

	// 3단계(Lv1 = 최대) 판매 레벨
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 ShopSellTier3Level = 10;

	UPROPERTY(Transient)
	TObjectPtr<UMLLevelUpWidget> LevelUpWidget;

	UPROPERTY(Transient)
	TObjectPtr<UMLRunHUDWidget> HUDWidget;
	
	UPROPERTY(Transient)
	TObjectPtr<UMLShopWidget> ShopWidget;

	TMap<FName, int32> PassiveLevels; // DT_Passives 행 -> 보유 레벨
	TArray<FMLLevelUpChoice> Choices;
	
	TMap<FName, TArray<FActiveGameplayEffectHandle>> PassiveEffects; // 패시브 레벨마다 GE 하나, 상점 제거용으로 보관
	TArray<FMLShopItem> ShopStock; // 진열 + 회복약, 판매됨 표시 유지
	TArray<FMLShopItem> ShopItems; // ShopStock 뒤에 보유 기술 / 패시브마다 제거 행 하나
	float ShopPriceMult = 1.f;

	int32 RunLevel = 1;
	float RunXP = 0.f;
	int32 RunGold = 0; // design §6.2: 탈출 전까지 미확보
	TArray<FName> RunItems;     // DT_Items 행, 미확보
	int32 RunItemTierTotal = 0;
	int32 PendingLevelUps = 0;
	bool bLevelUpOpen = false;
	bool bRerollUsed = false;
	bool bRestMode = false;

	static constexpr int32 ChoicesPerLevel = 3;
	static constexpr int32 ShopOfferCount = 3;
	static constexpr float FallbackHealPercent = 0.3f;
	static constexpr float RestHealPercent = 0.5f;
};
