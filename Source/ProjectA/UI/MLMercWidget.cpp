// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLMercWidget.h"

#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLSaveGame.h"
#include "UI/MLGuildUIText.h"

void UMLMercWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RetireButton->OnClicked.AddDynamic(this, &UMLMercWidget::HandleRetire);
	SellButton->OnClicked.AddDynamic(this, &UMLMercWidget::HandleSell);
	TitleButton->OnClicked.AddDynamic(this, &UMLMercWidget::HandleTitle);
}

void UMLMercWidget::Refresh()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	const bool bHasMerc = Save && Save->bHasMerc;
	RetireButton->SetIsEnabled(bHasMerc && Save->Merc.Talents.Num() > 0); // design §8.3: 은퇴하려면 고를 유산 재능이 필요
	SellButton->SetIsEnabled(bHasMerc);
	TitleButton->SetIsEnabled(bHasMerc);
	
	// design §7.5 훈련: 용병별, 구매할 때마다 비용 증가; 즉시 구매 (확인 없음)
	TrainingIds = bHasMerc ? Guild->GetTrainingIds() : TArray<FName>();
	if (UMLListEntryWidget::SyncPool(this, TrainingClass, TrainingList, TrainingEntries, TrainingIds.Num()))
	{
		for (int32 i = 0; i < TrainingIds.Num(); ++i)
		{
			const FMLTrainingRow* Row = Guild->FindTraining(TrainingIds[i]);
			const int32 Cost = Guild->GetTrainingCost(Save->Merc, TrainingIds[i]);
			const FText Label = FText::Format(NSLOCTEXT("ML", "TrainRow", "<Name>{0}</>  <Dim>{1} · {2}회</>\n<Gold>{3}G</>"),
				Row ? Row->Name : FText::FromName(TrainingIds[i]), Row ? MLGuildUI::StatsText(Row->Amount) : FText::GetEmpty(),
				FText::AsNumber(Save->Merc.TrainingBuys.FindRef(TrainingIds[i])), FText::AsNumber(Cost));
			TrainingEntries[i]->Setup(i, Label, Row && Save->Gold >= Cost);
			if (!TrainingEntries[i]->OnEntryClicked.IsBound()) TrainingEntries[i]->OnEntryClicked.BindUObject(this, &UMLMercWidget::HandleTraining);
		}
	}
	
	if (!bHasMerc)
	{
		MercText->SetText(NSLOCTEXT("ML", "MercNone", "보유한 용병이 없습니다.\n계약소에서 용병을 계약하세요."));
		return;
	}

	const FMLMercData& Merc = Save->Merc;
	TArray<FText> EquipNames;
	for (int32 s = 0; s < static_cast<int32>(EMLItemSlot::Count); ++s)
	{
		const FName* ItemId = Merc.Equipped.Find(static_cast<EMLItemSlot>(s));
		const FMLItemRow* Item = ItemId ? Guild->FindItem(*ItemId) : nullptr;
		EquipNames.Add(Item ? Item->Name : NSLOCTEXT("ML", "MercEquipEmpty", "없음"));
	}

	MercText->SetText(FText::Format(NSLOCTEXT("ML", "MercDetail",
		"{0}\n<Dim>칭호</>  {9}\n\n<Dim>장비</>  무기 {1} · 방어구 {2} · 장신구 {3}\n<Dim>기록</>  탈출 {4} · 보스 처치 {5} · 최고 {6}구역\n<Dim>은퇴 시</> <Talent>가문 포인트 +{7}</>    <Dim>판매가</> <Gold>{8}G</>"),
		MLGuildUI::DescribeMerc(*Guild, Merc), EquipNames[0], EquipNames[1], EquipNames[2],
		FText::AsNumber(Merc.Extractions), FText::AsNumber(Merc.BossKills), FText::AsNumber(Merc.BestZone),
		FText::AsNumber(Guild->GetRetirePoints(Merc)), FText::AsNumber(Guild->GetSellPrice(Merc)),
		MLGuildUI::TitleLine(*Guild, Merc.Title)));
}

void UMLMercWidget::HandleTitle()
{
	OnTitleRequested.ExecuteIfBound();
}

void UMLMercWidget::HandleRetire()
{
	OnRetireRequested.ExecuteIfBound();
}

void UMLMercWidget::HandleSell()
{
	OnSellRequested.ExecuteIfBound();
}

void UMLMercWidget::HandleTraining(int32 Index)
{
	UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	if (Guild && TrainingIds.IsValidIndex(Index))
	{
		Guild->Train(TrainingIds[Index]); // FName 값 전달이라 Train 안의 Refresh가 TrainingIds를 안전하게 다시 만들 수 있음
	}
}
