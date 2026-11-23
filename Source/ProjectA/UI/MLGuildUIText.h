// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Guild/MLGuildTypes.h"

class UMLGuildSubsystem;
class UWidget;

// 길드 화면 공용 텍스트 (design §9). shortcut: NSLOCTEXT 안의 한글 리터럴, 현지화 시 String Table로 (M10)
namespace MLGuildUI
{
	UMLGuildSubsystem* GetGuild(const UWidget* Widget);
	FText GradeText(EMLGrade Grade);
	FText WeaponText(EMLAttackType Type);
	FText TalentName(const UMLGuildSubsystem& Guild, FName TalentId);
	FText TalentsText(const UMLGuildSubsystem& Guild, const TArray<FName>& Talents);
	// RichText: "<Talent>이름</>" + 새 줄 "<Desc>설명</>"
	FText TalentLine(const UMLGuildSubsystem& Guild, FName TalentId);
	// 이름 · 등급 · 주무기 / 레벨 · 경험치 / 최종 스탯 / 재능
	FText DescribeMerc(const UMLGuildSubsystem& Guild, const FMLMercData& Merc);
	FText StatLabel(int32 Index); // 체력 / 공격 / 방어 / 속도 / 치명 / 회피 / 특수
	// "공격 +4, 치명 +2%" (0이 아닌 스탯만)
	FText StatsText(const FMLStatBlock& Stats);
	FText SlotText(EMLItemSlot ItemSlot);
	// RichText 창고 카드: 이름 / 슬롯 · 티어 / 용병이 장착할 수 없으면 "요구 등급 X"
	FText ItemLine(const FMLItemRow& Item, const FMLMercData* Merc);
	// RichText 상세: 헤더, 같은 장착 슬롯 아이템과의 스탯 비교, 무기 종류 일치, 설명, 장착 불가 이유
	FText ItemDetail(const UMLGuildSubsystem& Guild, const FMLItemRow& Item, const FMLMercData* Merc);
}
