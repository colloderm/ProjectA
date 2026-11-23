// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MLListEntryWidget.h"
#include "MLStashWidget.generated.h"

class UButton;
class UPanelWidget;
class URichTextBlock;
class UTextBlock;
class UWidget;

// design §8.4 / §9 #5 창고: 장착 슬롯 (클릭 = 해제), 창고 카드 (클릭 = 선택), 상세 + 비교, 장착 / 판매.
UCLASS(Abstract)
class PROJECTA_API UMLStashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

	FMLOnEntryClicked OnSellItemRequested; // 창고 인덱스; 허브가 확인 창을 엶
	FMLOnEntryClicked OnBuyRequested; // 상점 진열 인덱스; 허브가 확인 창을 엶

protected:
	virtual void NativeOnInitialized() override;

	// "창고 N/40"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> EquipList;

	// design §8.4 슬롯별로 나눈 창고, 각각 스크롤 열 하나 (EMLItemSlot과 같은 순서)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> WeaponList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ArmorList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> TrinketList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> DetailText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EquipButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SellButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SellText;
	
	// [보관함] [상점] 전환 (WBP_GuildTab 항목)과 그것이 바꾸는 두 화면
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ViewTabList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StashView;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ShopView;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ShopList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BuyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BuyText;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> ViewTabClass;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> SlotClass;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> ItemClass;
	
	// 아이템 티어 1..4별 카드 / 장착 슬롯 배경 (선택된 카드는 항목의 SelectedTint 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TArray<FLinearColor> TierTints = {
		FLinearColor(0.08f, 0.08f, 0.09f), FLinearColor(0.05f, 0.14f, 0.07f),
		FLinearColor(0.05f, 0.09f, 0.20f), FLinearColor(0.15f, 0.06f, 0.20f) };

private:
	UFUNCTION()
	void HandleEquip();

	UFUNCTION()
	void HandleSell();
	
	UFUNCTION()
	void HandleBuy();

	void HandleViewTab(int32 TabIndex);
	void HandleOffer(int32 OfferIndex);
	void HandleSlot(int32 SlotIndex);
	void HandleItem(int32 StashIndex);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> SlotEntries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> WeaponEntries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> ArmorEntries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> TrinketEntries;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> ViewTabs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> ShopEntries;

	bool bShopMode = false;
	int32 SelectedIndex = INDEX_NONE;
	FName SelectedItem; // 이 인덱스를 옮기는 창고 변경 (장착 교체, 판매)은 선택을 해제
	int32 SelectedOffer = INDEX_NONE;
	FName SelectedOfferItem; // 구매하면 진열이 빠지므로 선택이 해제됨
};
