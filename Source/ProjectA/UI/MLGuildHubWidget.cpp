// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGuildHubWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "Guild/MLGuildSettings.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MLConfirmWidget.h"
#include "UI/MLContractWidget.h"
#include "UI/MLGuildUIText.h"
#include "UI/MLMercWidget.h"
#include "UI/MLStashWidget.h"
#include "UI/MLFamilyWidget.h"
#include "UI/MLHallWidget.h"
#include "UI/MLTitleWidget.h"

void UMLGuildHubWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	MenuStyle = ContractTabButton->GetStyle();
	MenuTextColor = ContractTabText->GetColorAndOpacity();
	MenuFont = ContractTabText->GetFont();
	
	ContractTabButton->OnClicked.AddDynamic(this, &UMLGuildHubWidget::HandleContractTab);
	MercTabButton->OnClicked.AddDynamic(this, &UMLGuildHubWidget::HandleMercTab);
	StashTabButton->OnClicked.AddDynamic(this, &UMLGuildHubWidget::HandleStashTab);
	FamilyTabButton->OnClicked.AddDynamic(this, &UMLGuildHubWidget::HandleFamilyTab);
	HallTabButton->OnClicked.AddDynamic(this, &UMLGuildHubWidget::HandleHallTab);
	SortieButton->OnClicked.AddDynamic(this, &UMLGuildHubWidget::HandleSortie);

	ContractPanel->OnContractRequested.BindUObject(this, &UMLGuildHubWidget::HandleContractRequested);
	MercPanel->OnRetireRequested.BindUObject(this, &UMLGuildHubWidget::HandleRetireRequested);
	MercPanel->OnSellRequested.BindUObject(this, &UMLGuildHubWidget::HandleSellRequested);
	MercPanel->OnTitleRequested.BindUObject(this, &UMLGuildHubWidget::HandleTitleRequested);
	StashPanel->OnSellItemRequested.BindUObject(this, &UMLGuildHubWidget::HandleSellItemRequested);
	StashPanel->OnBuyRequested.BindUObject(this, &UMLGuildHubWidget::HandleBuyRequested);
	StashPanel->OnUpgradeShopRequested.BindUObject(this, &UMLGuildHubWidget::HandleUpgradeShopRequested);
	ConfirmPanel->OnChoice.BindUObject(this, &UMLGuildHubWidget::HandleConfirmChoice);
}

void UMLGuildHubWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save) return;

	GuildChangedHandle = Guild->OnGuildChanged.AddUObject(this, &UMLGuildHubWidget::Refresh);
	ConfirmPanel->Close();
	TitlePanel->Close();
	ShowPanel(Save->bHasMerc ? static_cast<UWidget*>(MercPanel.Get()) : ContractPanel.Get());
	Refresh();
	
	// design §6.6 (c): 비정상 종료된 원정이 되돌려졌다고 플레이어에게 한 번 알림
	if (Guild->ConsumeRollbackNotice())
	{
		Pending = EMLGuildPending::Notice;
		ConfirmPanel->Open(NSLOCTEXT("ML", "HubRollbackTitle", "원정 비정상 종료"),
			NSLOCTEXT("ML", "HubRollbackBody", "원정이 비정상 종료되어 획득물이 사라졌습니다.\n<Dim>용병과 장착 장비는 출정 전 상태 그대로입니다. 받은 의뢰는 취소되었습니다.</>"),
			{ NSLOCTEXT("ML", "HubRollbackOk", "확인") });
	}
}

void UMLGuildHubWidget::NativeDestruct()
{
	if (UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this))
	{
		Guild->OnGuildChanged.Remove(GuildChangedHandle);
	}
	Super::NativeDestruct();
}

void UMLGuildHubWidget::Refresh()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save) return;

	ResourceText->SetText(FText::Format(NSLOCTEXT("ML", "HubResources", "골드 {0} · 명성 {1} · 가문 포인트 {2}"),
		FText::AsNumber(Save->Gold), FText::AsNumber(Save->Renown), FText::AsNumber(Save->FamilyPoints)));

	SortieButton->SetIsEnabled(Save->bHasMerc);
	SortieHintText->SetText(Save->bHasMerc
		? FText::Format(NSLOCTEXT("ML", "HubSortieReady", "{0} 출정 준비 완료"), FText::FromString(Save->Merc.Name))
		: NSLOCTEXT("ML", "HubSortieNoMerc", "출정하려면 용병을 계약하세요"));

	ContractPanel->Refresh();
	MercPanel->Refresh();
	StashPanel->Refresh();
	FamilyPanel->Refresh();
	HallPanel->Refresh();
	TitlePanel->Refresh(); // 열려 있을 때만 갱신 (칭호 장착 직후 강조 이동)
}

void UMLGuildHubWidget::ShowPanel(UWidget* Panel)
{
	ContentSwitcher->SetActiveWidget(Panel);
	SetMenuSelected(ContractTabButton, ContractTabText, Panel == ContractPanel.Get());
	SetMenuSelected(MercTabButton, MercTabText, Panel == MercPanel.Get());
	SetMenuSelected(StashTabButton, StashTabText, Panel == StashPanel.Get());
	SetMenuSelected(FamilyTabButton, FamilyTabText, Panel == FamilyPanel.Get());
	SetMenuSelected(HallTabButton, HallTabText, Panel == HallPanel.Get());
}

void UMLGuildHubWidget::SetMenuSelected(UButton* Button, UTextBlock* Label, bool bSelected)
{
	FButtonStyle Style = MenuStyle;
	FSlateFontInfo Font = MenuFont;
	if (bSelected)
	{
		Style.Normal.TintColor = MenuSelectedTint;
		Style.Hovered.TintColor = MenuSelectedTint;
		Style.Pressed.TintColor = MenuSelectedTint;
		Font.TypefaceFontName = TEXT("Bold");
	}
	Button->SetStyle(Style);
	Label->SetColorAndOpacity(bSelected ? MenuSelectedTextColor : MenuTextColor);
	Label->SetFont(Font);
}

void UMLGuildHubWidget::HandleContractTab()
{
	ShowPanel(ContractPanel.Get());
}

void UMLGuildHubWidget::HandleMercTab()
{
	ShowPanel(MercPanel.Get());
}

void UMLGuildHubWidget::HandleStashTab()
{
	ShowPanel(StashPanel.Get());
}

void UMLGuildHubWidget::HandleFamilyTab()
{
	ShowPanel(FamilyPanel.Get());
}

void UMLGuildHubWidget::HandleHallTab()
{
	ShowPanel(HallPanel.Get());
}

void UMLGuildHubWidget::HandleSortie()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save || !Save->bHasMerc || GetDefault<UMLGuildSettings>()->RunMap.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Hub: sortie refused (no merc or RunMap not set in Project Settings > Mercenary Legacy)"));
		return;
	}

	// design §8.5.1: 출정 확인 창이 의뢰판을 겸함 (진열 + "없음"); 취소하면 허브에 남음
	const FMLMercData& Merc = Save->Merc;
	TArray<FText> EquipNames;
	for (int32 s = 0; s < static_cast<int32>(EMLItemSlot::Count); ++s)
	{
		const FName* ItemId = Merc.Equipped.Find(static_cast<EMLItemSlot>(s));
		const FMLItemRow* Item = ItemId ? Guild->FindItem(*ItemId) : nullptr;
		EquipNames.Add(Item ? Item->Name : NSLOCTEXT("ML", "HubSortieNoItem", "없음"));
	}

	TArray<FText> Choices;
	for (const FName& Id : Save->RequestOffers)
	{
		const FMLRequestRow* Request = Guild->FindRequest(Id);
		if (!Request)
		{
			Choices.Add(FText::FromName(Id)); // 선택지 인덱스 == 진열 인덱스 유지
			continue;
		}
		const FText Points = Request->RewardPoints > 0
			? FText::Format(NSLOCTEXT("ML", "HubRequestPoints", "  <Talent>가문 포인트 +{0}</>"), FText::AsNumber(Request->RewardPoints))
			: FText::GetEmpty();
		Choices.Add(FText::Format(NSLOCTEXT("ML", "HubRequestChoice", "<Name>{0}</>\n<Gold>{1}G</>  <Talent>경험치 +{2}</>{3}"),
			Request->Name, FText::AsNumber(Request->RewardGold), FText::AsNumber(Request->RewardXP), Points));
	}
	Choices.Add(NSLOCTEXT("ML", "HubSortieNoRequest", "<Dim>의뢰 없이 출정</>"));

	Pending = EMLGuildPending::Sortie;
	ConfirmPanel->Open(NSLOCTEXT("ML", "HubSortieTitle", "출정 준비"),
		FText::Format(NSLOCTEXT("ML", "HubSortieBody", "<Name>{0}</>  <Dim>{1}등급 · Lv {2} · 주무기 {3}</>\n<Dim>장비</> {4} · {5} · {6}\n\n<Dim>길드 의뢰를 하나 고르세요. 탈출에 성공해야 보상을 받습니다.</>"),
			FText::FromString(Merc.Name), MLGuildUI::GradeText(Merc.Grade), FText::AsNumber(Merc.Level), MLGuildUI::WeaponText(Merc.WeaponType),
			EquipNames[0], EquipNames[1], EquipNames[2]),
		Choices);
}

void UMLGuildHubWidget::StartSortie()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, GetDefault<UMLGuildSettings>()->RunMap);
}

void UMLGuildHubWidget::HandleContractRequested(EMLGrade Grade, int32 IndexInGrade)
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save) return;

	const FMLMercData* Candidate = nullptr;
	int32 Seen = 0;
	for (const FMLMercData& C : Save->Candidates)
	{
		if (C.Grade == Grade && Seen++ == IndexInGrade)
		{
			Candidate = &C;
			break;
		}
	}
	const FMLGradeRow* Row = Guild->FindGrade(Grade);
	if (!Candidate || !Row) return;

	Pending = EMLGuildPending::Contract;
	PendingGrade = Grade;
	PendingIndex = IndexInGrade;
	ConfirmPanel->Open(NSLOCTEXT("ML", "HubContractTitle", "용병 계약"),
		FText::Format(NSLOCTEXT("ML", "HubContractBody", "<Name>{0}</> ({1}등급 · 주무기 {2})을(를) <Gold>{3}G</>에 계약합니다.\n<Dim>보유 골드</> {4}G → <Gold>{5}G</>"),
			FText::FromString(Candidate->Name), MLGuildUI::GradeText(Grade), MLGuildUI::WeaponText(Candidate->WeaponType),
			FText::AsNumber(Row->ContractGold), FText::AsNumber(Save->Gold), FText::AsNumber(Save->Gold - Row->ContractGold)),
		{ NSLOCTEXT("ML", "HubContractOk", "계약한다") });
}

void UMLGuildHubWidget::HandleSellRequested()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save || !Save->bHasMerc) return;

	Pending = EMLGuildPending::Sell;
	ConfirmPanel->Open(NSLOCTEXT("ML", "HubSellTitle", "용병 판매"),
		FText::Format(NSLOCTEXT("ML", "HubSellBody", "<Name>{0}</>을(를) <Gold>{1}G</>에 판매합니다.\n<Dim>가문 포인트와 유산 재능은 얻지 못합니다.</>\n<Dim>장착 장비는 보관함으로 돌아갑니다.</>"),
			FText::FromString(Save->Merc.Name), FText::AsNumber(Guild->GetSellPrice(Save->Merc))),
		{ NSLOCTEXT("ML", "HubSellOk", "판매한다") });
}

void UMLGuildHubWidget::HandleSellItemRequested(int32 StashIndex)
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save || !Save->Stash.IsValidIndex(StashIndex)) return;
	const FMLItemRow* Item = Guild->FindItem(Save->Stash[StashIndex]);
	if (!Item) return;

	Pending = EMLGuildPending::SellItem;
	PendingIndex = StashIndex;
	ConfirmPanel->Open(NSLOCTEXT("ML", "HubSellItemTitle", "아이템 판매"),
		FText::Format(NSLOCTEXT("ML", "HubSellItemBody", "<Name>{0}</>을(를) <Gold>{1}G</>에 판매합니다.\n<Dim>판매한 아이템은 되찾을 수 없습니다.</>"),
			Item->Name, FText::AsNumber(Guild->GetItemSellPrice(*Item))),
		{ NSLOCTEXT("ML", "HubSellItemOk", "판매한다") });
}

void UMLGuildHubWidget::HandleBuyRequested(int32 OfferIndex)
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save || !Save->ShopOffers.IsValidIndex(OfferIndex)) return;
	const FMLItemRow* Item = Guild->FindItem(Save->ShopOffers[OfferIndex]);
	if (!Item) return;

	Pending = EMLGuildPending::BuyItem;
	PendingIndex = OfferIndex;
	ConfirmPanel->Open(NSLOCTEXT("ML", "HubBuyTitle", "아이템 구매"),
		FText::Format(NSLOCTEXT("ML", "HubBuyBody", "<Name>{0}</>을(를) <Gold>{1}G</>에 구매합니다."),
			Item->Name, FText::AsNumber(Item->Price)),
		{ NSLOCTEXT("ML", "HubBuyOk", "구매한다") });
}

void UMLGuildHubWidget::HandleUpgradeShopRequested()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	const int32 Cost = Guild ? Guild->GetShopUpgradeCost() : 0;
	if (!Save || Cost <= 0) return;

	// design §8.4: 등급 N = N~N+1티어
	const int32 NextGrade = Save->ShopGrade + 1;
	Pending = EMLGuildPending::UpgradeShop;
	ConfirmPanel->Open(NSLOCTEXT("ML", "HubUpgradeShopTitle", "상점 확장"),
		FText::Format(NSLOCTEXT("ML", "HubUpgradeShopBody", "상점을 <Name>{0}등급</>으로 올립니다. <Gold>{1}G</>\n{2}~{3}티어 장비가 진열됩니다. <Dim>(1칸은 항상 1티어)</>\n<Dim>지금 진열은 사라지고 바로 새로 바뀝니다.</>"),
			FText::AsNumber(NextGrade), FText::AsNumber(Cost), FText::AsNumber(NextGrade), FText::AsNumber(NextGrade + 1)),
		{ NSLOCTEXT("ML", "HubUpgradeShopOk", "확장한다") });
}

void UMLGuildHubWidget::HandleTitleRequested()
{
	TitlePanel->Open(); // design §8.7: [획득] [미획득] 탭 전용 창
}

void UMLGuildHubWidget::HandleRetireRequested()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save || !Save->bHasMerc) return;

	// design §8.3: 1단계에서 유산 재능 선택 (설명 표시, 이미 등록된 것은 표시)
	TArray<FText> Choices;
	for (const FName& Id : Save->Merc.Talents)
	{
		const FText Line = MLGuildUI::TalentLine(*Guild, Id);
		Choices.Add(Save->FamilyTalents.Contains(Id)
			? FText::Format(NSLOCTEXT("ML", "HubRetireTalentOwned", "{0}\n<Dim>이미 가문 재능 · 가문 재능 변화 없음</>"), Line)
			: Line);
	}

	Pending = EMLGuildPending::RetireTalent;
	const int32 Points = Guild->GetRetirePoints(Save->Merc);
	ConfirmPanel->Open(NSLOCTEXT("ML", "HubRetireTitle", "용병 은퇴"),
		FText::Format(NSLOCTEXT("ML", "HubRetireBody", "<Name>{0}</>을(를) 은퇴시킵니다.\n<Talent>가문 포인트 +{1}</> · <Talent>명성 +{1}</>\n<Dim>장착 장비는 보관함으로 돌아갑니다.</>\n\n가문에 남길 유산 재능을 누르면 은퇴가 확정됩니다."),
			FText::FromString(Save->Merc.Name), FText::AsNumber(Points)),
		Choices);
}

void UMLGuildHubWidget::HandleConfirmChoice(int32 Choice)
{
	UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	const EMLGuildPending Action = Pending;
	Pending = EMLGuildPending::None;
	if (!Save)
	{
		ConfirmPanel->Close();
		return;
	}

	bool bOk = false;
	switch (Action)
	{
	case EMLGuildPending::Contract:
		bOk = Guild->Contract(PendingGrade, PendingIndex);
		if (bOk) ShowPanel(MercPanel.Get());
		break;
	case EMLGuildPending::Sell:
		bOk = Guild->Sell();
		if (bOk) ShowPanel(ContractPanel.Get());
		break;
	case EMLGuildPending::RetireTalent:
	{
		const FName Legacy = Save->Merc.Talents.IsValidIndex(Choice) ? Save->Merc.Talents[Choice] : NAME_None;
		const int32 MaxFamily = GetDefault<UMLGuildSettings>()->MaxFamilyTalents;
		if (!Legacy.IsNone() && !Save->FamilyTalents.Contains(Legacy) && Save->FamilyTalents.Num() >= MaxFamily)
		{
			// design §8.3: 가문 목록이 가득 참 → 같은 창의 2단계에서 교체할 슬롯 선택
			Pending = EMLGuildPending::RetireReplace;
			PendingIndex = Choice;
			TArray<FText> Slots;
			for (const FName& Id : Save->FamilyTalents)
			{
				Slots.Add(MLGuildUI::TalentLine(*Guild, Id));
			}
			ConfirmPanel->Open(NSLOCTEXT("ML", "HubRetireReplaceTitle", "유산 재능 교체"),
				FText::Format(NSLOCTEXT("ML", "HubRetireReplaceBody", "가문 재능이 가득 찼습니다 ({0}/{1}).\n<Talent>{2}</>와(과) 바꿀 가문 재능을 고르세요."),
					FText::AsNumber(Save->FamilyTalents.Num()), FText::AsNumber(MaxFamily), MLGuildUI::TalentName(*Guild, Legacy)),
				Slots);
			return;
		}
		PendingIndex = Choice;
		bOk = Guild->Retire(Choice);
		if (bOk) ShowPanel(ContractPanel.Get());
		break;
	}
	case EMLGuildPending::SellItem:
		bOk = Guild->SellItem(PendingIndex);
		break;
	case EMLGuildPending::BuyItem:
		bOk = Guild->BuyShopItem(PendingIndex);
		break;
	case EMLGuildPending::UpgradeShop:
		bOk = Guild->UpgradeShop();
		break;
	case EMLGuildPending::Sortie:
		Guild->AcceptRequest(Save->RequestOffers.IsValidIndex(Choice) ? Choice : INDEX_NONE);
		ConfirmPanel->Close();
		StartSortie();
		return;
	case EMLGuildPending::Notice:
		break;
	case EMLGuildPending::RetireReplace:
		bOk = Guild->Retire(PendingIndex, Choice);
		if (bOk) ShowPanel(ContractPanel.Get());
		break;

	default:
		break;
	}

	ConfirmPanel->Close();
}
