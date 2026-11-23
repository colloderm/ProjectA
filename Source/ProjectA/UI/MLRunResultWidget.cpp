// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLRunResultWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Run/MLRunTypes.h"
#include "GameFramework/PlayerController.h"
#include "Guild/MLGuildSettings.h"
#include "UI/MLDamageRowWidget.h"

void UMLRunResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RestartButton->OnClicked.AddDynamic(this, &UMLRunResultWidget::HandleRestart);
}

void UMLRunResultWidget::Setup(const FMLRunSummary& Summary)
{
	TitleText->SetText(Summary.bBossKilled
		? NSLOCTEXT("ML", "ResultBoss", "보스 처치 · 탈출 성공")
		: Summary.bExtracted
			? NSLOCTEXT("ML", "ResultExtracted", "탈출 성공")
			: NSLOCTEXT("ML", "ResultDead", "전사"));

	// design §6.2: 원정 골드는 탈출할 때만 유지
	const FText GoldLine = Summary.bExtracted
		? FText::Format(NSLOCTEXT("ML", "ResultGoldGained", "획득 골드: +{0}"), FText::AsNumber(Summary.Gold))
		: FText::Format(NSLOCTEXT("ML", "ResultGoldLost", "잃은 골드: {0}"), FText::AsNumber(Summary.Gold));

	const int32 TotalSeconds = FMath::FloorToInt(Summary.TimeSeconds);
	const FText TimeText = FText::FromString(FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60));
	// design §11: 길드 결과 줄
	FText MercLine = FText::GetEmpty();
	const FMLRunReward& Reward = Summary.Reward;
	if (Reward.bFallen)
	{
		MercLine = FText::Format(NSLOCTEXT("ML", "ResultMercFallen", "\n{0} 전사 — 명예의 전당 #{1}"),
			FText::FromString(Reward.MercName), FText::AsNumber(Reward.HallNumber));
	}
	else if (Reward.bHasMerc)
	{
		MercLine = FText::Format(NSLOCTEXT("ML", "ResultMercXP", "\n{0} 용병 경험치 +{1} · Lv {2} → {3}"),
			FText::FromString(Reward.MercName), FText::AsNumber(Reward.MercXP), FText::AsNumber(Reward.OldLevel), FText::AsNumber(Reward.NewLevel));
	}
	
	// design §6.3: 아이템은 탈출할 때만 창고에 들어감
	FText ItemLine = FText::GetEmpty();
	if (Summary.bExtracted && Reward.ItemNames.Num() > 0)
	{
		const FText Names = FText::Join(FText::FromString(TEXT(", ")), Reward.ItemNames);
		ItemLine = Reward.AutoSoldGold > 0
			? FText::Format(NSLOCTEXT("ML", "ResultItemsSold", "\n장비 획득: {0} (창고 가득: +{1}G 자동 판매)"), Names, FText::AsNumber(Reward.AutoSoldGold))
			: FText::Format(NSLOCTEXT("ML", "ResultItems", "\n장비 획득: {0}"), Names);
	}
	else if (Reward.ItemsLost > 0)
	{
		ItemLine = FText::Format(NSLOCTEXT("ML", "ResultItemsLost", "\n장비 {0}개 소실"), FText::AsNumber(Reward.ItemsLost));
	}
	
	// design §8.5.1: 의뢰를 받았을 때만 의뢰 줄 표시
	FText RequestLine = FText::GetEmpty();
	if (!Reward.RequestName.IsEmpty())
	{
		const FText PointsText = Reward.RequestPoints > 0
			? FText::Format(NSLOCTEXT("ML", "ResultRequestPoints", " · 가문 포인트 +{0}"), FText::AsNumber(Reward.RequestPoints))
			: FText::GetEmpty();
		RequestLine = Reward.bRequestDone
			? FText::Format(NSLOCTEXT("ML", "ResultRequestDone", "\n의뢰 완료: {0} · +{1}G · 경험치 +{2}{3}"),
				Reward.RequestName, FText::AsNumber(Reward.RequestGold), FText::AsNumber(Reward.RequestXP), PointsText)
			: !Summary.bExtracted
				? FText::Format(NSLOCTEXT("ML", "ResultRequestNoExtract", "\n의뢰 실패: {0} (탈출해야 인정)"), Reward.RequestName)
				: FText::Format(NSLOCTEXT("ML", "ResultRequestFailed", "\n의뢰 실패: {0} ({1}/{2})"),
					Reward.RequestName, FText::AsNumber(Reward.RequestProgress), FText::AsNumber(Reward.RequestTarget));
	}

	DetailText->SetText(FText::Format(
		NSLOCTEXT("ML", "ResultDetail", "도달: {0}구역 · 방 {1}\n생존 시간: {2}\n처치: {3}\n판 레벨: {4}\n{5}{6}{7}{8}"),
		FText::AsNumber(Summary.Zone), FText::AsNumber(Summary.Room), TimeText,
		FText::AsNumber(Summary.Kills), FText::AsNumber(Summary.RunLevel), GoldLine, MercLine, ItemLine, RequestLine));
	
	// design §6 HUD 피해 리포트: 원정 합계
	const int32 CombatSecs = FMath::RoundToInt(Summary.CombatSeconds);
	RunDamageTitle->SetText(Summary.DamageLines.Num() > 0
		? FText::Format(NSLOCTEXT("ML", "ResultDamageTitle", "원정 피해 합계 · 전투 {0}"),
			FText::FromString(FString::Printf(TEXT("%d:%02d"), CombatSecs / 60, CombatSecs % 60)))
		: NSLOCTEXT("ML", "ResultDamageNone", "원정 피해 기록 없음"));
	UMLDamageRowWidget::FillList(this, RunDamageList, DamageRowClass, Summary.DamageLines, Summary.CombatSeconds);

	RestartButton->SetUserFocus(GetOwningPlayer()); // 게임패드는 여기서 시작
}

void UMLRunResultWidget::HandleRestart()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		// FinishRun이 UI 전용으로 바꿨음; 이게 없으면 다시 불러온 맵이 키보드 포커스를 되찾지 못함.
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	// design §9: 원정은 길드 허브로 돌아가며 끝남. GuildMap이 없으면 원정 맵을 다시 불러옴 (L_Test 직접 테스트).
	UGameplayStatics::SetGamePaused(this, false);
	const TSoftObjectPtr<UWorld>& GuildMap = GetDefault<UMLGuildSettings>()->GuildMap;
	if (!GuildMap.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, GuildMap);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
	}
}
