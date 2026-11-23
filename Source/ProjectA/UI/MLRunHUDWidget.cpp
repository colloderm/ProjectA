// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLRunHUDWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Run/MLRunGameMode.h"
#include "Run/MLPassiveTypes.h"
#include "Run/MLRunProgressComponent.h"
#include "Skills/MLAutoSkill.h"
#include "UI/MLSlotIconWidget.h"
#include "UI/MLDamageRowWidget.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UMLRunHUDWidget::InitHUD(UMLRunProgressComponent* InRunProgress)
{
	BoundRunProgress = InRunProgress;
	if (!InRunProgress) return;

	// AddUObject는 약한 참조: 위젯이 먼저 수거돼도 안전.
	InRunProgress->OnProgressChanged.AddUObject(this, &UMLRunHUDWidget::RefreshXP);
	InRunProgress->OnSlotsChanged.AddUObject(this, &UMLRunHUDWidget::RefreshSlots);
	InRunProgress->OnProgressChanged.AddUObject(this, &UMLRunHUDWidget::RefreshGold);

	if (SlotIconClass)
	{
		BuildRow(SkillRow, UMLRunProgressComponent::MaxSkillSlots, SkillSlots);
		BuildRow(PassiveRow, UMLRunProgressComponent::MaxPassiveSlots, PassiveSlots);
	}
	
	if (AMLRunGameMode* RunMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMLRunGameMode>() : nullptr)
	{
		BoundRunMode = RunMode;
		RunMode->OnRoomStateChanged.AddUObject(this, &UMLRunHUDWidget::RefreshRoom);
		RunMode->OnRoomStateChanged.AddUObject(this, &UMLRunHUDWidget::RefreshDamagePanel);
	}
	PortalArrow->SetVisibility(ESlateVisibility::Collapsed);
	GoblinArrow->SetVisibility(ESlateVisibility::Collapsed);
	BossPanel->SetVisibility(ESlateVisibility::Collapsed);
	DamagePanel->SetVisibility(ESlateVisibility::Collapsed);
	ObjectiveText->SetVisibility(ESlateVisibility::Collapsed);

	RefreshRoom();
	RefreshXP();
	RefreshSlots();
	RefreshGold();
}

void UMLRunHUDWidget::BuildRow(UHorizontalBox* Row, int32 SlotCount, TArray<TObjectPtr<UMLSlotIconWidget>>& OutSlots)
{
	Row->ClearChildren();
	OutSlots.Reset();

	for (int32 i = 0; i < SlotCount; ++i)
	{
		UMLSlotIconWidget* SlotIcon = CreateWidget<UMLSlotIconWidget>(this, SlotIconClass);
		if (!SlotIcon) continue;

		if (UHorizontalBoxSlot* RowSlot = Row->AddChildToHorizontalBox(SlotIcon))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, SlotSpacing, 0.f));
		}
		OutSlots.Add(SlotIcon);
	}
}

void UMLRunHUDWidget::RefreshXP()
{
	const UMLRunProgressComponent* RunProgress = BoundRunProgress.Get();
	if (!RunProgress) return;

	XPBar->SetPercent(RunProgress->GetRunXP() / RunProgress->GetXPToNext());
	RunLevelText->SetText(FText::Format(NSLOCTEXT("ML", "RunLevel", "Lv {0}"), FText::AsNumber(RunProgress->GetRunLevel())));
}

void UMLRunHUDWidget::RefreshSlots()
{
	const UMLRunProgressComponent* RunProgress = BoundRunProgress.Get();
	if (!RunProgress) return;

	const TArray<TObjectPtr<UMLAutoSkill>>& Skills = RunProgress->GetOwnedSkills();
	for (int32 i = 0; i < SkillSlots.Num(); ++i)
	{
		const UMLAutoSkill* Skill = Skills.IsValidIndex(i) ? Skills[i].Get() : nullptr;
		if (Skill)
		{
			const int32 Tier = Skill->GetSkillTier();
			const FLinearColor& TierColor = Tier >= 3 ? Tier3SlotColor : (Tier == 2 ? Tier2SlotColor : SkillSlotColor);
			SkillSlots[i]->SetFilled(Skill->GetSkillIcon(), Skill->GetSkillName(), Skill->GetSkillLevel(), TierColor);
		}
		else
		{
			SkillSlots[i]->SetEmpty(EmptySlotColor);
		}
	}

	int32 PassiveIndex = 0;
	for (const TPair<FName, int32>& Pair : RunProgress->GetPassiveLevels())
	{
		if (!PassiveSlots.IsValidIndex(PassiveIndex)) break;

		const FMLPassiveRow* PassiveData = RunProgress->FindPassiveRow(Pair.Key);
		PassiveSlots[PassiveIndex++]->SetFilled(
			PassiveData ? PassiveData->Icon.Get() : nullptr,
			PassiveData ? PassiveData->DisplayName : FText::FromName(Pair.Key),
			Pair.Value, PassiveSlotColor);
	}
	for (; PassiveIndex < PassiveSlots.Num(); ++PassiveIndex)
	{
		PassiveSlots[PassiveIndex]->SetEmpty(EmptySlotColor);
	}
}

void UMLRunHUDWidget::RefreshRoom()
{
	const AMLRunGameMode* RunMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMLRunGameMode>() : nullptr;
	if (!RunMode) return;

	const FText Zone = FText::AsNumber(RunMode->GetZone());
	const FText Room = FText::AsNumber(RunMode->GetRoomInZone());
	const FText Rooms = FText::AsNumber(RunMode->GetRoomsInZone());

	// 황금 방 강조 문구: 슬라임 처치·시간 종료로 방이 끝나면 Combat이 아니게 되어 바로 사라짐
	const bool bObjective = RunMode->GetRunState() == EMLRunState::Combat && RunMode->GetCurrentRoomType() == EMLRoomType::Gold;
	if (bObjective != bObjectiveShown)
	{
		bObjectiveShown = bObjective;
		if (bObjective) ObjectiveText->SetText(NSLOCTEXT("ML", "HUDGoldObjective", "황금 슬라임을 잡으세요!"));
		ObjectiveText->SetVisibility(bObjective ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	FText NewText;
	switch (RunMode->GetRunState())
	{
	case EMLRunState::Combat:
		{
			const EMLRoomType RoomType = RunMode->GetCurrentRoomType();
			const int32 Secs = FMath::Max(0, RunMode->GetRoomSecondsLeft());
			const FText Time = FText::FromString(FString::Printf(TEXT("%d:%02d"), Secs / 60, Secs % 60));
			if (RoomType == EMLRoomType::Gold)
			{
				NewText = FText::Format(NSLOCTEXT("ML", "HUDRoomGold", "{0}구역 · 방 {1}/{2} · 황금 슬라임 {3}"), Zone, Room, Rooms, Time);
			}
			else if (RoomType == EMLRoomType::Elite)
			{
				NewText = FText::Format(NSLOCTEXT("ML", "HUDRoomElite", "{0}구역 · 방 {1}/{2} · 남은 엘리트 {3} · {4}"),
					Zone, Room, Rooms, FText::AsNumber(RunMode->GetElitesLeft()), Time);
			}
			else if (RoomType == EMLRoomType::Boss)
			{
				const int32 Fight = RunMode->GetBossFightSeconds();
				NewText = RunMode->IsBossDefeated()
					? NSLOCTEXT("ML", "HUDBossDown", "보스 처치!")
					: FText::Format(NSLOCTEXT("ML", "HUDBoss", "보스전 · {0}"), FText::FromString(FString::Printf(TEXT("%d:%02d"), Fight / 60, Fight % 60)));
			}
			else
			{
				NewText = FText::Format(NSLOCTEXT("ML", "HUDRoomKills", "{0}구역 · 방 {1}/{2} · 남은 적 {3}"),
					Zone, Room, Rooms, FText::AsNumber(RunMode->GetEnemiesLeft()));
			}
			break;
		}
	case EMLRunState::Collecting:
		{
			const EMLRoomType RoomType = RunMode->GetCurrentRoomType();
			if (RoomType != EMLRoomType::Rest && RoomType != EMLRoomType::Shop) return; // 보석이 날아오는 동안 "남은 적 0" 유지
			NewText = FText::Format(NSLOCTEXT("ML", "HUDRoomSafe", "{0}구역 · 방 {1}/{2} · {3}"), Zone, Room, Rooms, MLRun::GetRoomTypeName(RoomType));
			break;
		}
	case EMLRunState::Choosing:
		NewText = RunMode->IsExitDoorOpen()
			? NSLOCTEXT("ML", "HUDChooseDoor", "다음 방을 고르세요 · 탈출 문 열림")
			: NSLOCTEXT("ML", "HUDChoose", "다음 방을 고르세요");
		break;
	default:
		return; // 수집 / 페이드 / 결과 중에는 마지막 문구 유지
	}
	RoomText->SetText(NewText);
}

void UMLRunHUDWidget::RefreshGold()
{
	const UMLRunProgressComponent* RunProgress = BoundRunProgress.Get();
	if (!RunProgress) return;

	const AMLRunGameMode* RunMode = BoundRunMode.Get();
	GoldText->SetText(FText::Format(NSLOCTEXT("ML", "HUDGold", "골드 {0} · 탐욕 {1}/{2} · 장비 {3}"),
		FText::AsNumber(RunProgress->GetRunGold()),
		FText::AsNumber(RunMode ? RunMode->GetGreedLevel() : 0),
		FText::AsNumber(RunMode ? RunMode->GetMaxGreedLevel() : 0),
		FText::AsNumber(RunProgress->GetRunItems().Num())));
}

void UMLRunHUDWidget::RefreshDamagePanel()
{
	// 처치할 때마다도 브로드캐스트됨: 패널이 나타나거나 사라져야 할 때만 처리
	const AMLRunGameMode* RunMode = BoundRunMode.Get();
	const EMLRunState State = RunMode ? RunMode->GetRunState() : EMLRunState::Transition;
	const bool bShow = RunMode && (State == EMLRunState::Collecting || State == EMLRunState::Choosing)
		&& RunMode->GetRoomDamageLines().Num() > 0;
	if (bShow == bDamagePanelShown) return;

	bDamagePanelShown = bShow;
	if (bShow)
	{
		const int32 Secs = FMath::RoundToInt(RunMode->GetRoomCombatSeconds());
		DamageTitle->SetText(FText::Format(NSLOCTEXT("ML", "HUDDamageTitle", "방 피해 · 전투 {0}"),
			FText::FromString(FString::Printf(TEXT("%d:%02d"), Secs / 60, Secs % 60))));
		UMLDamageRowWidget::FillList(this, DamageList, DamageRowClass, RunMode->GetRoomDamageLines(), RunMode->GetRoomCombatSeconds());
	}
	DamagePanel->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMLRunHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AMLRunGameMode* RunMode = BoundRunMode.Get();
	const bool bChoosing = RunMode && RunMode->GetRunState() == EMLRunState::Choosing;
	UpdateArrow(PortalArrow, bChoosing, bChoosing ? RunMode->GetPortalGroupCenter() : FVector::ZeroVector, bPortalArrowShown);

	FVector TargetLoc = FVector::ZeroVector;
	const bool bTarget = RunMode && (RunMode->GetGoblinLocation(TargetLoc) || RunMode->GetEliteLocation(TargetLoc));
	UpdateArrow(GoblinArrow, bTarget, TargetLoc, bGoblinArrowShown);
	
	float BossPercent = 0.f;
	FText BossName;
	const bool bBoss = RunMode && RunMode->GetBossHealth(BossPercent, BossName);
	if (bBoss != bBossPanelShown)
	{
		bBossPanelShown = bBoss;
		BossPanel->SetVisibility(bBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bBoss) BossNameText->SetText(BossName);
	}
	if (bBoss) BossBar->SetPercent(BossPercent);
}

void UMLRunHUDWidget::UpdateArrow(UWidget* Arrow, bool bActive, const FVector& Target, bool& bShown)
{
	const APlayerController* PC = GetOwningPlayer();
	const APawn* Hero = GetOwningPlayerPawn();
	const float Dist = (bActive && Hero) ? FVector::Dist2D(Hero->GetActorLocation(), Target) : 0.f;
	const float Threshold = bShown ? ArrowShowDistance - 100.f : ArrowShowDistance; // 히스테리시스: 경계에서 깜빡이지 않게
	const bool bShow = bActive && PC && Hero && Dist > Threshold;

	if (bShow != bShown)
	{
		bShown = bShow;
		Arrow->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bShow) return;

	// 화살표 슬롯은 화면 중앙에 있음; 영웅의 화면 위치로 옮긴 뒤 방향을 따라 바깥으로.
	FVector2D HeroPos, TargetPos;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Hero->GetActorLocation(), HeroPos, false)) return;
	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Target, TargetPos, false); // 화면 밖이어도 쓸 수 있는 방향이 나옴
	const FVector2D Dir = (TargetPos - HeroPos).GetSafeNormal();
	const FVector2D ViewCenter = UWidgetLayoutLibrary::GetViewportSize(this) / UWidgetLayoutLibrary::GetViewportScale(this) * 0.5f;

	Arrow->SetRenderTranslation(HeroPos - ViewCenter + Dir * ArrowRadius);
	Arrow->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)));
}
