// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLFamilyWidget.h"

#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Guild/MLGuildSettings.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLSaveGame.h"
#include "UI/MLGuildUIText.h"

namespace
{
	// "공격 +6%, 탈출 골드 +10%" Level 기준 (가문 보너스는 모두 퍼센트)
	FText UpgradeEffect(const FMLFamilyUpgradeRow& Row, int32 Level)
	{
		TArray<FText> Parts;
		for (int32 i = 0; i < FMLStatBlock::Num; ++i)
		{
			const int32 Value = FMath::RoundToInt(Row.StatPercentPerLevel[i] * Level);
			if (Value != 0)
			{
				Parts.Add(FText::Format(NSLOCTEXT("ML", "FamilyStat", "{0} +{1}%"), MLGuildUI::StatLabel(i), FText::AsNumber(Value)));
			}
		}
		const int32 GoldPercent = FMath::RoundToInt(Row.GoldPercentPerLevel * Level);
		if (GoldPercent != 0)
		{
			Parts.Add(FText::Format(NSLOCTEXT("ML", "FamilyGold", "탈출 골드 +{0}%"), FText::AsNumber(GoldPercent)));
		}
		const int32 XPPercent = FMath::RoundToInt(Row.XPPercentPerLevel * Level);
		if (XPPercent != 0)
		{
			Parts.Add(FText::Format(NSLOCTEXT("ML", "FamilyXP", "용병 경험치 +{0}%"), FText::AsNumber(XPPercent)));
		}
		return Parts.Num() > 0 ? FText::Join(FText::FromString(TEXT(", ")), Parts) : NSLOCTEXT("ML", "FamilyNoEffect", "효과 없음");
	}
}

void UMLFamilyWidget::Refresh()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save) return;

	PointsText->SetText(FText::Format(NSLOCTEXT("ML", "FamilyPoints", "가문 포인트 {0} · 명성 {1}"),
		FText::AsNumber(Save->FamilyPoints), FText::AsNumber(Save->Renown)));

	// design §8.3: 계약 등급은 명성으로 해금 (소비되지 않음)
	FText Unlock = NSLOCTEXT("ML", "FamilyAllUnlocked", "모든 등급의 계약이 해금되었습니다.");
	for (int32 g = 0; g < static_cast<int32>(EMLGrade::Count); ++g)
	{
		const FMLGradeRow* Row = Guild->FindGrade(static_cast<EMLGrade>(g));
		if (Row && Save->Renown < Row->UnlockRenown)
		{
			Unlock = FText::Format(NSLOCTEXT("ML", "FamilyNextUnlock", "다음 해금: {0}등급 계약 (명성 {1} / {2})"),
				MLGuildUI::GradeText(static_cast<EMLGrade>(g)), FText::AsNumber(Save->Renown), FText::AsNumber(Row->UnlockRenown));
			break;
		}
	}
	UnlockText->SetText(Unlock);

	// design §8.3 가문 강화: 가문 포인트, Cost × CostGrowth^레벨, 행마다 상한; 즉시 구매 (확인 없음)
	UpgradeIds = Guild->GetFamilyUpgradeIds();
	if (UMLListEntryWidget::SyncPool(this, UpgradeClass, UpgradeList, UpgradeEntries, UpgradeIds.Num()))
	{
		for (int32 i = 0; i < UpgradeIds.Num(); ++i)
		{
			if (!UpgradeEntries[i]->OnEntryClicked.IsBound()) UpgradeEntries[i]->OnEntryClicked.BindUObject(this, &UMLFamilyWidget::HandleUpgrade);

			const FMLFamilyUpgradeRow* Row = Guild->FindFamilyUpgrade(UpgradeIds[i]);
			if (!Row)
			{
				UpgradeEntries[i]->Setup(i, FText::FromName(UpgradeIds[i]), false);
				continue;
			}
			const int32 Level = Save->FamilyUpgrades.FindRef(UpgradeIds[i]);
			const int32 Cost = Guild->GetFamilyUpgradeCost(UpgradeIds[i]);
			const bool bMax = Level >= Row->MaxLevel;
			const FText Label = bMax
				? FText::Format(NSLOCTEXT("ML", "FamilyUpgradeMax", "<Name>{0}</>  <Dim>Lv {1}/{1} · {2} (최대)</>"),
					Row->Name, FText::AsNumber(Row->MaxLevel), UpgradeEffect(*Row, Level))
				: FText::Format(NSLOCTEXT("ML", "FamilyUpgrade", "<Name>{0}</>  <Dim>Lv {1}/{2} · {3} → {4}</>\n<Talent>{5}P</>"),
					Row->Name, FText::AsNumber(Level), FText::AsNumber(Row->MaxLevel),
					UpgradeEffect(*Row, Level), UpgradeEffect(*Row, Level + 1), FText::AsNumber(Cost));
			UpgradeEntries[i]->Setup(i, Label, !bMax && Save->FamilyPoints >= Cost);
		}
	}

	// design §8.3 유산 재능: 은퇴 시 선택, 계약 후보에서 가중치만 올림
	const UMLGuildSettings* Settings = GetDefault<UMLGuildSettings>();
	TArray<FText> Lines;
	for (int32 i = 0; i < Settings->MaxFamilyTalents; ++i)
	{
		Lines.Add(Save->FamilyTalents.IsValidIndex(i)
			? MLGuildUI::TalentLine(*Guild, Save->FamilyTalents[i])
			: NSLOCTEXT("ML", "FamilyTalentEmpty", "<Dim>비어 있음</>"));
	}
	Lines.Add(FText::GetEmpty());
	Lines.Add(FText::Format(NSLOCTEXT("ML", "FamilyTalentInfo", "<Dim>{0}/{1}칸 · 가문 재능은 계약 후보에 {2}배 자주 나옵니다.</>"),
		FText::AsNumber(Save->FamilyTalents.Num()), FText::AsNumber(Settings->MaxFamilyTalents), FText::AsNumber(Settings->FamilyTalentWeightMult)));
	Lines.Add(NSLOCTEXT("ML", "FamilyTalentHow", "<Dim>용병을 은퇴시키면 그 용병의 재능 하나를 남길 수 있습니다.</>"));
	TalentText->SetText(FText::Join(FText::FromString(TEXT("\n")), Lines));
}

void UMLFamilyWidget::HandleUpgrade(int32 Index)
{
	UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	if (Guild && UpgradeIds.IsValidIndex(Index))
	{
		Guild->BuyFamilyUpgrade(UpgradeIds[Index]); // FName 값 전달: 안의 Refresh가 UpgradeIds를 다시 만들 수 있음
	}
}
