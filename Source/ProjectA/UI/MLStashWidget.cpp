// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLStashWidget.h"

#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Guild/MLGuildSettings.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLSaveGame.h"
#include "UI/MLGuildUIText.h"

void UMLStashWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EquipButton->OnClicked.AddDynamic(this, &UMLStashWidget::HandleEquip);
	SellButton->OnClicked.AddDynamic(this, &UMLStashWidget::HandleSell);
	BuyButton->OnClicked.AddDynamic(this, &UMLStashWidget::HandleBuy);
}

void UMLStashWidget::Refresh()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save) return;

	const FMLMercData* Merc = Save->bHasMerc ? &Save->Merc : nullptr;
	const int32 Capacity = GetDefault<UMLGuildSettings>()->StashCapacity;
	const bool bStashFull = Save->Stash.Num() >= Capacity;
	CountText->SetText(FText::Format(bStashFull
		? NSLOCTEXT("ML", "StashCountFull", "창고 {0}/{1} · 가득 참: 장비를 해제할 수 없습니다")
		: NSLOCTEXT("ML", "StashCount", "창고 {0}/{1}"),
		FText::AsNumber(Save->Stash.Num()), FText::AsNumber(Capacity)));

	auto TierTint = [this](int32 Tier) { return TierTints.IsValidIndex(Tier - 1) ? &TierTints[Tier - 1] : nullptr; };

	// [보관함] [상점]: 선택한 탭이 보일 화면과 동작 버튼을 정함
	if (UMLListEntryWidget::SyncPool(this, ViewTabClass, ViewTabList, ViewTabs, 2))
	{
		const FText Names[] = { NSLOCTEXT("ML", "StashViewStash", "보관함"), NSLOCTEXT("ML", "StashViewShop", "상점") };
		for (int32 t = 0; t < 2; ++t)
		{
			const bool bSelected = (t == 1) == bShopMode;
			ViewTabs[t]->Setup(t, bSelected ? FText::Format(NSLOCTEXT("ML", "StashViewSelected", "<Selected>{0}</>"), Names[t]) : Names[t], true);
			ViewTabs[t]->SetSelected(bSelected);
			if (!ViewTabs[t]->OnEntryClicked.IsBound()) ViewTabs[t]->OnEntryClicked.BindUObject(this, &UMLStashWidget::HandleViewTab);
		}
	}
	StashView->SetVisibility(bShopMode ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	ShopView->SetVisibility(bShopMode ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	EquipButton->SetVisibility(bShopMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	SellButton->SetVisibility(bShopMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	BuyButton->SetVisibility(bShopMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// design §8.4: 가득 찬 창고로 장착 해제하면 아이템이 자동 판매되므로 여기서 막음
	constexpr int32 SlotCount = static_cast<int32>(EMLItemSlot::Count);
	if (UMLListEntryWidget::SyncPool(this, SlotClass, EquipList, SlotEntries, SlotCount))
	{
		for (int32 s = 0; s < SlotCount; ++s)
		{
			const EMLItemSlot ItemSlot = static_cast<EMLItemSlot>(s);
			const FName* ItemId = Merc ? Merc->Equipped.Find(ItemSlot) : nullptr;
			const FMLItemRow* Item = ItemId ? Guild->FindItem(*ItemId) : nullptr;
			const FText Label = FText::Format(NSLOCTEXT("ML", "StashSlot", "<Dim>{0}</>\n{1}"), MLGuildUI::SlotText(ItemSlot),
				Item ? FText::Format(NSLOCTEXT("ML", "StashSlotItem", "<Name>{0}</>"), Item->Name)
					 : NSLOCTEXT("ML", "StashSlotEmpty", "<Dim>비어 있음</>"));
			SlotEntries[s]->Setup(s, Label, Item && !bStashFull);
			SlotEntries[s]->SetSelected(false, Item ? TierTint(Item->Tier) : nullptr);
			if (!SlotEntries[s]->OnEntryClicked.IsBound()) SlotEntries[s]->OnEntryClicked.BindUObject(this, &UMLStashWidget::HandleSlot);
		}
	}

	if (!Save->Stash.IsValidIndex(SelectedIndex) || Save->Stash[SelectedIndex] != SelectedItem)
	{
		SelectedIndex = INDEX_NONE;
	}

	// 슬롯마다 열 하나; 각 카드는 창고 인덱스를 유지하므로 선택 / 장착 / 판매는 그대로.
	// shortcut: DT_Items에 없는 창고 행은 표시하지 않음 (데이터 오류, ML.Guild.Status가 로그로 남김)
	UPanelWidget* Lists[] = { WeaponList.Get(), ArmorList.Get(), TrinketList.Get() };
	TArray<TObjectPtr<UMLListEntryWidget>>* Pools[] = { &WeaponEntries, &ArmorEntries, &TrinketEntries };
	for (int32 s = 0; s < SlotCount; ++s)
	{
		TArray<int32> Indices;
		for (int32 i = 0; i < Save->Stash.Num(); ++i)
		{
			const FMLItemRow* Item = Guild->FindItem(Save->Stash[i]);
			if (Item && static_cast<int32>(Item->Slot) == s) Indices.Add(i);
		}

		TArray<TObjectPtr<UMLListEntryWidget>>& Pool = *Pools[s];
		if (!UMLListEntryWidget::SyncPool(this, ItemClass, Lists[s], Pool, Indices.Num())) continue;
		for (int32 n = 0; n < Indices.Num(); ++n)
		{
			const int32 StashIndex = Indices[n];
			const FMLItemRow* Item = Guild->FindItem(Save->Stash[StashIndex]);
			Pool[n]->Setup(StashIndex, MLGuildUI::ItemLine(*Item, Merc), true);
			Pool[n]->SetSelected(StashIndex == SelectedIndex, TierTint(Item->Tier));
			if (!Pool[n]->OnEntryClicked.IsBound()) Pool[n]->OnEntryClicked.BindUObject(this, &UMLStashWidget::HandleItem);
		}
	}

	// design §8.4 길드 상점: 원정마다 진열 갱신; 구매한 진열은 목록에서 빠짐
	if (!Save->ShopOffers.IsValidIndex(SelectedOffer) || Save->ShopOffers[SelectedOffer] != SelectedOfferItem)
	{
		SelectedOffer = INDEX_NONE;
	}
	if (UMLListEntryWidget::SyncPool(this, ItemClass, ShopList, ShopEntries, Save->ShopOffers.Num()))
	{
		for (int32 i = 0; i < Save->ShopOffers.Num(); ++i)
		{
			const FMLItemRow* Item = Guild->FindItem(Save->ShopOffers[i]);
			const FText Label = Item
				? FText::Format(NSLOCTEXT("ML", "ShopCard", "{0}\n<Gold>{1}G</>"), MLGuildUI::ItemLine(*Item, Merc), FText::AsNumber(Item->Price))
				: FText::FromName(Save->ShopOffers[i]);
			ShopEntries[i]->Setup(i, Label, Item != nullptr);
			ShopEntries[i]->SetSelected(i == SelectedOffer, Item ? TierTint(Item->Tier) : nullptr);
			if (!ShopEntries[i]->OnEntryClicked.IsBound()) ShopEntries[i]->OnEntryClicked.BindUObject(this, &UMLStashWidget::HandleOffer);
		}
	}

	if (bShopMode)
	{
		const FMLItemRow* Offer = SelectedOffer != INDEX_NONE ? Guild->FindItem(SelectedOfferItem) : nullptr;
		const bool bCanPay = Offer && Save->Gold >= Offer->Price;
		FText Detail = Save->ShopOffers.Num() > 0
			? NSLOCTEXT("ML", "ShopPick", "<Dim>진열된 아이템을 누르면 정보와 비교가 나옵니다.</>\n<Dim>진열은 원정이 끝날 때마다 바뀝니다.</>")
			: NSLOCTEXT("ML", "ShopSoldOut", "<Dim>진열된 아이템을 모두 구매했습니다.</>\n<Dim>원정이 끝나면 새 물건이 들어옵니다.</>");
		if (Offer)
		{
			Detail = MLGuildUI::ItemDetail(*Guild, *Offer, Merc);
			if (bStashFull)
			{
				Detail = FText::Format(NSLOCTEXT("ML", "ShopStashFull", "{0}\n\n<Dim>창고가 가득 차서 구매할 수 없습니다.</>"), Detail);
			}
			else if (!bCanPay)
			{
				Detail = FText::Format(NSLOCTEXT("ML", "ShopNoGold", "{0}\n\n<Dim>골드가 부족합니다 (보유 {1}G).</>"), Detail, FText::AsNumber(Save->Gold));
			}
		}
		DetailText->SetText(Detail);
		BuyButton->SetIsEnabled(bCanPay && !bStashFull);
		BuyText->SetText(Offer
			? FText::Format(NSLOCTEXT("ML", "ShopBuy", "구매 {0}G"), FText::AsNumber(Offer->Price))
			: NSLOCTEXT("ML", "ShopBuyNone", "구매"));
		return;
	}

	const FMLItemRow* Selected = SelectedIndex != INDEX_NONE ? Guild->FindItem(SelectedItem) : nullptr;
	DetailText->SetText(Selected
		? MLGuildUI::ItemDetail(*Guild, *Selected, Merc)
		: NSLOCTEXT("ML", "StashPick", "<Dim>창고의 아이템을 누르면 정보와 비교가 나옵니다.</>\n<Dim>장착한 장비를 누르면 해제합니다.</>"));

	// design §7.4: 필요 등급 미만 = 장착 불가 (상세 텍스트에 이유 표시)
	EquipButton->SetIsEnabled(Selected && Merc && Merc->Grade >= Selected->RequiredGrade);
	SellButton->SetIsEnabled(Selected != nullptr);
	SellText->SetText(Selected
		? FText::Format(NSLOCTEXT("ML", "StashSell", "판매 {0}G"), FText::AsNumber(Guild->GetItemSellPrice(*Selected)))
		: NSLOCTEXT("ML", "StashSellNone", "판매"));
}

void UMLStashWidget::HandleItem(int32 StashIndex)
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save || !Save->Stash.IsValidIndex(StashIndex)) return;

	SelectedIndex = StashIndex;
	SelectedItem = Save->Stash[StashIndex];
	Refresh();
}

void UMLStashWidget::HandleSlot(int32 SlotIndex)
{
	if (UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this))
	{
		Guild->Unequip(static_cast<EMLItemSlot>(SlotIndex)); // Save() → OnGuildChanged → 허브 갱신
	}
}

void UMLStashWidget::HandleEquip()
{
	UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	if (Guild && SelectedIndex != INDEX_NONE)
	{
		Guild->Equip(SelectedIndex); // 교체하면 이전 아이템이 창고 끝으로 감; 이어지는 Refresh가 선택을 해제
	}
}

void UMLStashWidget::HandleSell()
{
	if (SelectedIndex != INDEX_NONE)
	{
		OnSellItemRequested.ExecuteIfBound(SelectedIndex);
	}
}

void UMLStashWidget::HandleBuy()
{
	if (SelectedOffer != INDEX_NONE)
	{
		OnBuyRequested.ExecuteIfBound(SelectedOffer);
	}
}

void UMLStashWidget::HandleViewTab(int32 TabIndex)
{
	bShopMode = TabIndex == 1;
	Refresh();
}

void UMLStashWidget::HandleOffer(int32 OfferIndex)
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save || !Save->ShopOffers.IsValidIndex(OfferIndex)) return;

	SelectedOffer = OfferIndex;
	SelectedOfferItem = Save->ShopOffers[OfferIndex];
	Refresh();
}
