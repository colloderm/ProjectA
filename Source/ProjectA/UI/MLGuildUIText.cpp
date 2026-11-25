// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGuildUIText.h"

#include "Components/Widget.h"
#include "Engine/GameInstance.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLGuildSettings.h"

namespace MLGuildUI
{
	FText StatLabel(int32 Index)
	{
		switch (Index)
		{
		case 0:  return NSLOCTEXT("ML", "StatHP", "체력");
		case 1:  return NSLOCTEXT("ML", "StatATK", "공격");
		case 2:  return NSLOCTEXT("ML", "StatDEF", "방어");
		case 3:  return NSLOCTEXT("ML", "StatSPD", "속도");
		case 4:  return NSLOCTEXT("ML", "StatCRIT", "치명");
		case 5:  return NSLOCTEXT("ML", "StatDODGE", "회피");
		default: return NSLOCTEXT("ML", "StatSPECIAL", "특수");
		}
	}
	
	UMLGuildSubsystem* GetGuild(const UWidget* Widget)
	{
		const UGameInstance* GI = Widget ? Widget->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UMLGuildSubsystem>() : nullptr;
	}

	FText GradeText(EMLGrade Grade)
	{
		return StaticEnum<EMLGrade>()->GetDisplayNameTextByValue(static_cast<int64>(Grade));
	}

	FText WeaponText(EMLAttackType Type)
	{
		switch (Type)
		{
		case EMLAttackType::Sword: return NSLOCTEXT("ML", "WeaponSword", "검");
		case EMLAttackType::Bow:   return NSLOCTEXT("ML", "WeaponBow", "활");
		case EMLAttackType::Staff: return NSLOCTEXT("ML", "WeaponStaff", "지팡이");
		default:                   return NSLOCTEXT("ML", "WeaponDagger", "단검");
		}
	}

	FText TalentName(const UMLGuildSubsystem& Guild, FName TalentId)
	{
		const FMLTalentRow* Talent = Guild.FindTalent(TalentId);
		return Talent ? Talent->Name : FText::FromName(TalentId);
	}

	FText TalentsText(const UMLGuildSubsystem& Guild, const TArray<FName>& Talents)
	{
		TArray<FText> Names;
		for (const FName& Id : Talents)
		{
			Names.Add(TalentName(Guild, Id));
		}
		return Names.Num() > 0 ? FText::Join(FText::FromString(TEXT(", ")), Names) : NSLOCTEXT("ML", "TalentsNone", "없음");
	}
	
	FText TalentLine(const UMLGuildSubsystem& Guild, FName TalentId)
	{
		const FMLTalentRow* Talent = Guild.FindTalent(TalentId);
		return FText::Format(NSLOCTEXT("ML", "TalentLine", "<Talent>{0}</>\n<Desc>{1}</>"),
			TalentName(Guild, TalentId), Talent ? Talent->Description : FText::GetEmpty());
	}

	FText TitleName(const UMLGuildSubsystem& Guild, FName TitleId)
	{
		const FMLTitleRow* Title = Guild.FindTitle(TitleId);
		if (!Title) return NSLOCTEXT("ML", "TitleNone", "없음");

		static const TCHAR* Tags[] = { TEXT("TitleCommon"), TEXT("TitleRare"), TEXT("TitleEpic"), TEXT("TitleLegend") };
		const int32 TagIndex = FMath::Clamp(static_cast<int32>(Title->Rarity), 0, 3);
		return FText::Format(NSLOCTEXT("ML", "TitleTagged", "<{0}>{1}</>"), FText::FromString(Tags[TagIndex]), Title->Name);
	}

	FText TitleLine(const UMLGuildSubsystem& Guild, FName TitleId)
	{
		const FMLTitleRow* Title = Guild.FindTitle(TitleId);
		if (!Title) return TitleName(Guild, TitleId);

		TArray<FText> Parts;
		for (int32 i = 0; i < FMLStatBlock::Num; ++i)
		{
			if (Title->BonusPercent[i] != 0.f)
			{
				Parts.Add(FText::Format(NSLOCTEXT("ML", "TitleBonusPct", "{0} +{1}%"), StatLabel(i), FText::AsNumber(Title->BonusPercent[i])));
			}
			if (Title->Flat[i] != 0.f)
			{
				Parts.Add(FText::Format(NSLOCTEXT("ML", "TitleBonusFlat", "{0} +{1}"), StatLabel(i), FText::AsNumber(Title->Flat[i])));
			}
		}
		if (Title->GoldPercent != 0.f) Parts.Add(FText::Format(NSLOCTEXT("ML", "TitleBonusGold", "탈출 골드 +{0}%"), FText::AsNumber(Title->GoldPercent)));
		if (Title->XPPercent != 0.f) Parts.Add(FText::Format(NSLOCTEXT("ML", "TitleBonusXP", "경험치 +{0}%"), FText::AsNumber(Title->XPPercent)));

		return FText::Format(NSLOCTEXT("ML", "TitleLine", "{0}  <Dim>{1}</>"), TitleName(Guild, TitleId), FText::Join(FText::FromString(TEXT(", ")), Parts));
	}

	FText TitleProgressLine(const UMLGuildSubsystem& Guild, FName TitleId)
	{
		const FMLTitleRow* Title = Guild.FindTitle(TitleId);
		if (!Title) return FText::GetEmpty();

		FText StatName;
		switch (Title->Stat)
		{
		case EMLTitleStat::Kills:       StatName = NSLOCTEXT("ML", "TitleStatKills", "처치"); break;
		case EMLTitleStat::EliteKills:  StatName = NSLOCTEXT("ML", "TitleStatElites", "엘리트 처치"); break;
		case EMLTitleStat::BossKills:   StatName = NSLOCTEXT("ML", "TitleStatBoss", "보스 처치"); break;
		case EMLTitleStat::Extractions: StatName = NSLOCTEXT("ML", "TitleStatExtract", "탈출"); break;
		case EMLTitleStat::Deaths:      StatName = NSLOCTEXT("ML", "TitleStatDeaths", "용병 전사"); break;
		default:                        StatName = NSLOCTEXT("ML", "TitleStatGold", "원정 골드"); break;
		}
		// 미획득 탭 항목: 1줄 = 등급 색 이름 + 보너스, 2줄 = 진행도
		return FText::Format(NSLOCTEXT("ML", "TitleProgress", "{0}\n<Dim>{1} / {2} {3}</>"), TitleLine(Guild, TitleId),
			FText::AsNumber(Guild.GetTitleProgress(TitleId)), FText::AsNumber(Title->Target), StatName);
	}

	FText DescribeMerc(const UMLGuildSubsystem& Guild, const FMLMercData& Merc)
	{
		const FMLGradeRow* Row = Guild.FindGrade(Merc.Grade);
		const int32 Need = Guild.GetXPToNextLevel(Merc);
		const FText XPText = Need > 0
			? FText::Format(NSLOCTEXT("ML", "MercXP", "경험치 {0}/{1}"), FText::AsNumber(Merc.XP), FText::AsNumber(Need))
			: NSLOCTEXT("ML", "MercXPMax", "최대 레벨");
		const FMLStatBlock S = Guild.ComputeStats(Merc);
		auto Stat = [&S](int32 i) { return FText::AsNumber(FMath::RoundToInt(S[i])); };

		// RichText 태그는 줄바꿈을 넘지 못함 (마크업 파서가 줄 단위로 동작)
		return FText::Format(NSLOCTEXT("ML", "MercDescribe",
			"<Name>{0}</>\n<Dim>등급</> {1}    <Dim>주무기</> {2}\n<Dim>Lv</> {3}/{4}    <Dim>{5}</>\n<Dim>체력</> {6}  <Dim>공격</> {7}  <Dim>방어</> {8}  <Dim>속도</> {9}\n<Dim>치명</> {10}%  <Dim>회피</> {11}%  <Dim>특수</> {12}%\n<Dim>재능</> <Talent>{13}</>"),
			FText::FromString(Merc.Name), GradeText(Merc.Grade), WeaponText(Merc.WeaponType),
			FText::AsNumber(Merc.Level), FText::AsNumber(Row ? Row->MaxLevel : Merc.Level), XPText,
			Stat(0), Stat(1), Stat(2), Stat(3), Stat(4), Stat(5), Stat(6),
			TalentsText(Guild, Merc.Talents));
	}
	
	FText StatsText(const FMLStatBlock& Stats)
	{
		TArray<FText> Parts;
		for (int32 i = 0; i < FMLStatBlock::Num; ++i)
		{
			const int32 Value = FMath::RoundToInt(Stats[i]);
			if (Value == 0) continue;
			Parts.Add(FText::Format(NSLOCTEXT("ML", "StatPart", "{0} {1}{2}{3}"), StatLabel(i),
				Value > 0 ? FText::FromString(TEXT("+")) : FText::GetEmpty(), FText::AsNumber(Value),
				FMLStatBlock::IsPercentStat(i) ? FText::FromString(TEXT("%")) : FText::GetEmpty()));
		}
		return FText::Join(FText::FromString(TEXT(", ")), Parts);
	}
	
	FText SlotText(EMLItemSlot ItemSlot)
	{
		switch (ItemSlot)
		{
		case EMLItemSlot::Armor:   return NSLOCTEXT("ML", "SlotArmor", "방어구");
		case EMLItemSlot::Trinket: return NSLOCTEXT("ML", "SlotTrinket", "장신구");
		default:                   return NSLOCTEXT("ML", "SlotWeapon", "무기");
		}
	}

	FText ItemLine(const FMLItemRow& Item, const FMLMercData* Merc)
	{
		const FText Head = FText::Format(NSLOCTEXT("ML", "ItemCard", "<Name>{0}</>\n<Dim>{1} · {2}티어</>"),
			Item.Name, SlotText(Item.Slot), FText::AsNumber(Item.Tier));
		return Merc && Merc->Grade < Item.RequiredGrade
			? FText::Format(NSLOCTEXT("ML", "ItemCardLocked", "{0}\n<Dim>요구 등급 {1}</>"), Head, GradeText(Item.RequiredGrade))
			: Head;
	}

	FText ItemDetail(const UMLGuildSubsystem& Guild, const FMLItemRow& Item, const FMLMercData* Merc)
	{
		const FName* EquippedId = Merc ? Merc->Equipped.Find(Item.Slot) : nullptr;
		const FMLItemRow* Current = EquippedId ? Guild.FindItem(*EquippedId) : nullptr;

		TArray<FText> Lines;
		Lines.Add(FText::Format(NSLOCTEXT("ML", "ItemHead", "<Name>{0}</>\n<Dim>{1} · {2}티어 · 요구 등급 {3}</>"),
			Item.Name, SlotText(Item.Slot), FText::AsNumber(Item.Tier), GradeText(Item.RequiredGrade)));
		Lines.Add(FText::GetEmpty()); // 빈 줄: 스탯 / 무기 종류 / 설명이 한 덩어리로 읽힘

		// design §9 #5 비교: 이 아이템 vs 같은 슬롯에 장착한 아이템 (빈 슬롯 = 0)
		for (int32 i = 0; i < FMLStatBlock::Num; ++i)
		{
			const int32 New = FMath::RoundToInt(Item.Flat[i]);
			const int32 Old = Current ? FMath::RoundToInt(Current->Flat[i]) : 0;
			if (New == 0 && Old == 0) continue;

			const FText Unit = FMLStatBlock::IsPercentStat(i) ? FText::FromString(TEXT("%")) : FText::GetEmpty();
			FText Diff = FText::GetEmpty();
			if (Merc)
			{
				const int32 Delta = New - Old;
				const FText DeltaText = Delta > 0
					? FText::Format(NSLOCTEXT("ML", "ItemUp", "<Talent>(+{0})</>"), FText::AsNumber(Delta))
					: Delta < 0
						? FText::Format(NSLOCTEXT("ML", "ItemDown", "<Dim>({0})</>"), FText::AsNumber(Delta))
						: NSLOCTEXT("ML", "ItemSame", "<Dim>(같음)</>");
				Diff = FText::Format(NSLOCTEXT("ML", "ItemDiff", "   <Dim>현재 +{0}{1}</>  {2}"), FText::AsNumber(Old), Unit, DeltaText);
			}
			Lines.Add(FText::Format(NSLOCTEXT("ML", "ItemStat", "<Dim>{0}</> +{1}{2}{3}"), StatLabel(i), FText::AsNumber(New), Unit, Diff));
		}

		if (Item.Slot == EMLItemSlot::Weapon)
		{
			const FText Type = WeaponText(Item.WeaponType);
			if (!Merc)
			{
				Lines.Add(FText::Format(NSLOCTEXT("ML", "ItemWeapon", "<Dim>공격 방식</> {0}"), Type));
			}
			else if (Item.WeaponType == Merc->WeaponType)
			{
				const int32 Bonus = FMath::RoundToInt((GetDefault<UMLGuildSettings>()->WeaponMatchBonus - 1.f) * 100.f);
				Lines.Add(FText::Format(NSLOCTEXT("ML", "ItemWeaponMatch", "<Dim>공격 방식</> {0}   <Talent>주무기 일치 · 기본 공격 +{1}%</>"),
					Type, FText::AsNumber(Bonus)));
			}
			else
			{
				Lines.Add(FText::Format(NSLOCTEXT("ML", "ItemWeaponOther", "<Dim>공격 방식</> {0}   <Dim>주무기({1})와 다름</>"),
					Type, WeaponText(Merc->WeaponType)));
			}
		}

		if (!Item.Description.IsEmpty())
		{
			Lines.Add(FText::Format(NSLOCTEXT("ML", "ItemDesc", "<Desc>{0}</>"), Item.Description));
		}

		FText Blocked = FText::GetEmpty();
		if (!Merc)
		{
			Blocked = NSLOCTEXT("ML", "ItemNoMerc", "<Dim>보유한 용병이 없어 장착할 수 없습니다.</>");
		}
		else if (Merc->Grade < Item.RequiredGrade)
		{
			Blocked = FText::Format(NSLOCTEXT("ML", "ItemGradeLocked", "<Dim>{0}등급 용병은 장착할 수 없습니다 (요구 등급 {1}).</>"),
				GradeText(Merc->Grade), GradeText(Item.RequiredGrade));
		}
		if (!Blocked.IsEmpty())
		{
			Lines.Add(FText::GetEmpty()); // 장착할 수 없는 이유 앞의 빈 줄
			Lines.Add(Blocked);
		}
		return FText::Join(FText::FromString(TEXT("\n")), Lines);
	}
}
