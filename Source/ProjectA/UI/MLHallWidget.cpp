// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLHallWidget.h"

#include "Components/TextBlock.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLSaveGame.h"
#include "UI/MLGuildUIText.h"

void UMLHallWidget::Refresh()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save) return;

	const TArray<FMLHallEntry>& Hall = Save->HallOfFame;
	EmptyText->SetVisibility(Hall.Num() == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!UMLListEntryWidget::SyncPool(this, RowClass, HallList, HallRows, Hall.Num())) return;

	for (int32 n = 0; n < Hall.Num(); ++n)
	{
		const int32 Index = Hall.Num() - 1 - n; // 최신순; 번호는 전당 순서 그대로 (#1 = 가장 먼저 떠남)
		const FMLHallEntry& Entry = Hall[Index];
		const FMLMercData& Merc = Entry.Merc;
		const FMLGrave* Grave = Save->Graves.FindByPredicate([&Merc](const FMLGrave& G) { return G.MercId == Merc.Id; }); // design §6.2.3
		const FText Reason = Entry.Reason == EMLLeaveReason::Retired ? NSLOCTEXT("ML", "HallRetired", "<Talent>은퇴</>")
			: Entry.Reason == EMLLeaveReason::Sold ? NSLOCTEXT("ML", "HallSold", "<Gold>판매</>")
			: Grave ? FText::Format(NSLOCTEXT("ML", "HallFallenGrave", "<Dim>전사</> · <Talent>묘비 {0}구역</> <Dim>({1}회 남음)</>"),
				FText::AsNumber(Grave->Zone), FText::AsNumber(Grave->RunsLeft))
			: NSLOCTEXT("ML", "HallFallen", "<Dim>전사</>");

		const FText Label = FText::Format(NSLOCTEXT("ML", "HallRow",
			"<Dim>#{0}</>  <Name>{1}</>  <Dim>{2}등급 · Lv {3} · 주무기 {4}</>   {5}\n<Dim>탈출</> {6} · <Dim>보스</> {7} · <Dim>최고</> {8}구역 · <Dim>벌어온 골드</> <Gold>{9}</> · <Dim>재능</> <Talent>{10}</> · <Dim>{11}</>"),
			FText::AsNumber(Index + 1), FText::FromString(Merc.Name), MLGuildUI::GradeText(Merc.Grade), FText::AsNumber(Merc.Level),
			MLGuildUI::WeaponText(Merc.WeaponType), Reason,
			FText::AsNumber(Merc.Extractions), FText::AsNumber(Merc.BossKills), FText::AsNumber(Merc.BestZone),
			FText::AsNumber(Merc.GoldEarned), MLGuildUI::TalentsText(*Guild, Merc.Talents),
			FText::FromString(Entry.Date.ToString(TEXT("%m/%d %H:%M"))));
		HallRows[n]->Setup(Index, Label, true); // 읽기 전용: OnEntryClicked는 바인딩하지 않음
	}
}
